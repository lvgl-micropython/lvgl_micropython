#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED

    #include <stdlib.h>
    #include <stdarg.h>
    #include <sys/cdefs.h>
    #include <sys/param.h>
    #include <string.h>
    #include "sdkconfig.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"
    #include "esp_attr.h"
    #include "esp_check.h"
    #include "esp_pm.h"
    #include "esp_lcd_panel_interface.h"
    #include "esp_lcd_panel_ops.h"
    #include "esp_rom_gpio.h"
    #include "soc/soc_caps.h"
    #include "esp_clk_tree.h"
    #include "hal/dma_types.h"
    #include "hal/gpio_hal.h"
    #include "esp_private/gdma.h"
    #include "driver/gpio.h"
    #include "esp_bit_defs.h"
    #include "esp_private/periph_ctrl.h"
    #include "esp_psram.h"
    #include "esp_lcd_common.h"
    #include "soc/lcd_periph.h"
    #include "hal/lcd_hal.h"
    #include "hal/lcd_ll.h"
    #include "hal/gdma_ll.h"
    #include "rom/cache.h"
    #include "esp_cache.h"

    // local includes
    #include "esp_rgb_bus.h"

    static const char *TAG = "rgb_bus";

    static esp_err_t rgb_panel_io_register_callbacks(esp_lcd_panel_io_handle_t io, const esp_lcd_panel_io_callbacks_t *cbs, void *user_ctx);
    static esp_err_t rgb_panel_io_del(esp_lcd_panel_io_handle_t io);
    static esp_err_t rgb_panel_io_tx_color(esp_lcd_panel_io_t *io, int lcd_cmd, const void *color, size_t color_size);
    esp_err_t esp_lcd_new_panel_io_rgb(esp_lcd_rgb_bus_handle_t bus, const esp_lcd_panel_io_rgb_config_t *io_config, esp_lcd_panel_io_handle_t *ret_io);

    static esp_err_t rgb_bus_select_clock_src(esp_lcd_rgb_bus_t *rgb_bus, lcd_clock_source_t clk_src);
    static esp_err_t rgb_bus_create_trans_linkt_clock_src(esp_lcd_rgb_bus_t *rgb_bus);
    static esp_err_t rgb_bus_configure_gpio(esp_lcd_rgb_bus_t *rgb_bus, const esp_lcd_rgb_bus_config_t *panel_config);
    static void rgb_bus_start_transmission(esp_lcd_rgb_bus_t *rgb_bus);
    static void rgb_bus_default_isr_handler(void *args);

    #ifndef traceISR_EXIT_TO_SCHEDULER
        #define traceISR_EXIT_TO_SCHEDULER()
    #endif

    static esp_err_t rgb_bus_alloc_frame_buffers(const esp_lcd_rgb_bus_config_t *bus_config, esp_lcd_rgb_bus_handle_t rgb_bus)
    {
        bool fb_in_psram = false;
        size_t psram_trans_align = bus_config->psram_trans_align ? bus_config->psram_trans_align : 64;
        size_t sram_trans_align = bus_config->sram_trans_align ? bus_config->sram_trans_align : 4;
        rgb_bus->psram_trans_align = psram_trans_align;
        rgb_bus->sram_trans_align = sram_trans_align;

        // alloc frame buffer
        if (rgb_bus->num_fbs > 0) {
            // fb_in_psram is only an option, if there's no PSRAM on board, we fallback to alloc from SRAM
            if (bus_config->flags.fb_in_psram) {
    #if CONFIG_SPIRAM_USE_MALLOC || CONFIG_SPIRAM_USE_CAPS_ALLOC
                if (esp_psram_is_initialized()) {
                    fb_in_psram = true;
                }
    #endif
            }
            for (int i = 0; i < rgb_bus->num_fbs; i++) {
                if (fb_in_psram) {
                    // the low level malloc function will help check the validation of alignment
                    rgb_bus->fbs[i] = heap_caps_aligned_calloc(psram_trans_align, 1, rgb_bus->fb_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
                } else {
                    rgb_bus->fbs[i] = heap_caps_aligned_calloc(sram_trans_align, 1, rgb_bus->fb_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
                }
                ESP_RETURN_ON_FALSE(rgb_bus->fbs[i], ESP_ERR_NO_MEM, TAG, "no mem for frame buffer");
            }
        }

        rgb_bus->cur_fb_index = 0;
        rgb_bus->flags.fb_in_psram = fb_in_psram;

        return ESP_OK;
    }


    static esp_err_t rgb_bus_destory(esp_lcd_rgb_bus_handle_t rgb_bus)
    {
        lcd_ll_enable_clock(rgb_bus->hal.dev, false);
        if (rgb_bus->panel_id >= 0) {
            PERIPH_RCC_RELEASE_ATOMIC(lcd_periph_signals.panels[rgb_bus->panel_id].module, ref_count) {
                if (ref_count == 0) {
                    lcd_ll_enable_bus_clock(rgb_bus->panel_id, false);
                }
            }
            lcd_com_remove_device(LCD_COM_DEVICE_TYPE_RGB, rgb_bus->panel_id);
        }
        if (rgb_bus->dma_chan) {
            gdma_disconnect(rgb_bus->dma_chan);
            gdma_del_channel(rgb_bus->dma_chan);
        }
        if (rgb_bus->intr) {
            esp_intr_free(rgb_bus->intr);
        }
        if (rgb_bus->pm_lock) {
            esp_pm_lock_release(rgb_bus->pm_lock);
            esp_pm_lock_delete(rgb_bus->pm_lock);
        }
        free(rgb_bus);
        return ESP_OK;
    }


    esp_err_t esp_lcd_new_rgb_bus(const esp_lcd_rgb_bus_config_t *bus_config, esp_lcd_rgb_bus_handle_t *ret_bus)
    {
    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        esp_log_level_set(TAG, ESP_LOG_DEBUG);
    #endif
        esp_err_t ret = ESP_OK;
        esp_lcd_rgb_bus_t *rgb_bus = NULL;
        ESP_GOTO_ON_FALSE(bus_config && ret_bus, ESP_ERR_INVALID_ARG, err, TAG, "invalid parameter");
        ESP_GOTO_ON_FALSE(bus_config->data_width == 16 || bus_config->data_width == 8,
                          ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported data width %d", bus_config->data_width);
        ESP_GOTO_ON_FALSE(!(bus_config->flags.double_fb && bus_config->flags.no_fb),
                          ESP_ERR_INVALID_ARG, err, TAG, "double_fb conflicts with no_fb");
        ESP_GOTO_ON_FALSE(!(bus_config->num_fbs > 0 && bus_config->num_fbs != 2 && bus_config->flags.double_fb),
                          ESP_ERR_INVALID_ARG, err, TAG, "num_fbs conflicts with double_fb");
        ESP_GOTO_ON_FALSE(!(bus_config->num_fbs > 0 && bus_config->flags.no_fb),
                          ESP_ERR_INVALID_ARG, err, TAG, "num_fbs conflicts with no_fb");

        // determine number of framebuffers
        size_t num_fbs = 1;
        if (bus_config->flags.no_fb) {
            num_fbs = 0;
        } else if (bus_config->flags.double_fb) {
            num_fbs = 2;
        } else if (bus_config->num_fbs > 0) {
            num_fbs = bus_config->num_fbs;
        }
        ESP_GOTO_ON_FALSE(num_fbs <= RGB_BUS_LCD_PANEL_MAX_FB_NUM, ESP_ERR_INVALID_ARG, err, TAG, "too many frame buffers");

        // bpp defaults to the number of data lines, but for serial RGB interface, they're not equal
        // e.g. for serial RGB 8-bit interface, data lines are 8, whereas the bpp is 24 (RGB888)
        size_t fb_bits_per_pixel = bus_config->data_width;
        if (bus_config->bits_per_pixel) { // override bpp if it's set
            fb_bits_per_pixel = bus_config->bits_per_pixel;
        }
        // calculate buffer size
        size_t fb_size = bus_config->h_res * bus_config->v_res * fb_bits_per_pixel / 8;

        // calculate the number of DMA descriptors
        size_t num_dma_nodes = 0;

        num_dma_nodes = (fb_size + DMA_DESCRIPTOR_BUFFER_MAX_SIZE - 1) / DMA_DESCRIPTOR_BUFFER_MAX_SIZE;

        // DMA descriptors must be placed in internal SRAM (requested by DMA)
        rgb_bus = heap_caps_calloc(1, sizeof(esp_lcd_rgb_bus_t) + num_dma_nodes * sizeof(dma_descriptor_t) * RGB_BUS_LCD_PANEL_DMA_LINKS_REPLICA,
                                     MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
        ESP_GOTO_ON_FALSE(rgb_bus, ESP_ERR_NO_MEM, err, TAG, "no mem for rgb panel");
        rgb_bus->h_res = bus_config->h_res;
        rgb_bus->v_res = bus_config->v_res;
        rgb_bus->num_dma_nodes = num_dma_nodes;
        rgb_bus->num_fbs = num_fbs;
        rgb_bus->fb_size = fb_size;
        rgb_bus->panel_id = -1;
        // register to platform
        int panel_id = lcd_com_register_device(LCD_COM_DEVICE_TYPE_RGB, rgb_bus);
        ESP_GOTO_ON_FALSE(panel_id >= 0, ESP_ERR_NOT_FOUND, err, TAG, "no free rgb panel slot");
        rgb_bus->panel_id = panel_id;

        // enable APB to access LCD registers
        PERIPH_RCC_ACQUIRE_ATOMIC(lcd_periph_signals.panels[panel_id].module, ref_count) {
            if (ref_count == 0) {
                lcd_ll_enable_bus_clock(panel_id, true);
                lcd_ll_reset_register(panel_id);
            }
        }

        // allocate frame buffers + bounce buffers
        ESP_GOTO_ON_ERROR(rgb_bus_alloc_frame_buffers(bus_config, rgb_bus), err, TAG, "alloc frame buffers failed");

        // initialize HAL layer, so we can call LL APIs later
        lcd_hal_init(&rgb_bus->hal, panel_id);
        // enable clock gating
        lcd_ll_enable_clock(rgb_bus->hal.dev, true);
        // set clock source
        ret = rgb_bus_select_clock_src(rgb_bus, bus_config->clk_src);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "set source clock failed");
        // set minimal PCLK divider
        // A limitation in the hardware, if the LCD_PCLK == LCD_CLK, then the PCLK polarity can't be adjustable
        if (!(bus_config->flags.pclk_active_neg || bus_config->flags.pclk_idle_high)) {
            rgb_bus->lcd_clk_flags |= LCD_HAL_PCLK_FLAG_ALLOW_EQUAL_SYSCLK;
        }

        rgb_bus->flags.pclk_idle_high = bus_config->flags.pclk_idle_high;
        rgb_bus->flags.pclk_active_neg = bus_config->flags.pclk_active_neg;

        // install interrupt service, (LCD peripheral shares the interrupt source with Camera by different mask)
        int isr_flags = ESP_INTR_FLAG_INTRDISABLED | ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_LOWMED;
        ret = esp_intr_alloc_intrstatus(lcd_periph_signals.panels[panel_id].irq_id, isr_flags,
                                        (uint32_t)lcd_ll_get_interrupt_status_reg(rgb_bus->hal.dev),
                                        LCD_LL_EVENT_VSYNC_END, rgb_bus_default_isr_handler, rgb_bus, &rgb_bus->intr);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "install interrupt failed");
        lcd_ll_enable_interrupt(rgb_bus->hal.dev, LCD_LL_EVENT_VSYNC_END, false); // disable all interrupts
        lcd_ll_clear_interrupt_status(rgb_bus->hal.dev, UINT32_MAX); // clear pending interrupt

        // install DMA service
        rgb_bus->flags.stream_mode = !bus_config->flags.refresh_on_demand;
        ret = rgb_bus_create_trans_linkt_clock_src(rgb_bus);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "install DMA failed");
        // configure GPIO
        ret = rgb_bus_configure_gpio(rgb_bus, bus_config);
        ESP_GOTO_ON_ERROR(ret, err, TAG, "configure GPIO failed");
        // fill other rgb panel runtime parameters
        memcpy(rgb_bus->data_gpio_nums, bus_config->data_gpio_nums, SOC_LCD_RGB_DATA_WIDTH);
        rgb_bus->data_width = bus_config->data_width;
        rgb_bus->fb_bits_per_pixel = fb_bits_per_pixel;
        rgb_bus->output_bits_per_pixel = fb_bits_per_pixel; // by default, the output bpp is the same as the frame buffer bpp
        rgb_bus->spinlock = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;
        // return base class
        *ret_bus = rgb_bus;

        return ESP_OK;

    err:
        if (rgb_bus) {
            rgb_bus_destory(rgb_bus);
        }
        return ret;
    }


    esp_err_t rgb_panel_io_register_callbacks(esp_lcd_panel_io_handle_t io, const esp_lcd_panel_io_callbacks_t *cbs, void *user_ctx)
    {
        lcd_panel_io_rgb_t *rgb_device = __containerof(io, lcd_panel_io_rgb_t, base);
        esp_lcd_rgb_bus_t *rgb_bus = rgb_device->bus;

        rgb_bus->on_vsync = cbs->on_color_trans_done;
        rgb_bus->user_ctx = user_ctx;
        return ESP_OK;
    }


    esp_err_t rgb_panel_io_del(esp_lcd_panel_io_handle_t io)
    {
        return ESP_OK;
    }


    esp_err_t esp_lcd_del_rgb_bus(esp_lcd_rgb_bus_handle_t rgb_bus) {
        ESP_RETURN_ON_ERROR(rgb_bus_destory(rgb_bus), TAG, "destroy rgb io bus failed");
        return ESP_OK;
    }


    esp_err_t esp_lcd_new_panel_io_rgb(esp_lcd_rgb_bus_handle_t rgb_bus, const esp_lcd_panel_io_rgb_config_t *io_config, esp_lcd_panel_io_handle_t *ret_io)
    {
        esp_err_t ret = ESP_OK;
        // set pixel clock frequency
        rgb_bus->pclk_hz = lcd_hal_cal_pclk_freq(&rgb_bus->hal, rgb_bus->src_clk_hz, io_config->pclk_hz, rgb_bus->lcd_clk_flags);
        // pixel clock phase and polarity
        lcd_ll_set_clock_idle_level(rgb_bus->hal.dev, rgb_bus->flags.pclk_idle_high);
        lcd_ll_set_pixel_clock_edge(rgb_bus->hal.dev, rgb_bus->flags.pclk_active_neg);
        // enable RGB mode and set data width
        lcd_ll_enable_rgb_mode(rgb_bus->hal.dev, true);
        lcd_ll_set_data_width(rgb_bus->hal.dev, rgb_bus->data_width);
        lcd_ll_set_phase_cycles(rgb_bus->hal.dev, 0, 0, 1); // enable data phase only
        // number of data cycles is controlled by DMA buffer size
        lcd_ll_enable_output_always_on(rgb_bus->hal.dev, true);
        // configure HSYNC, VSYNC, DE signal idle state level
        lcd_ll_set_idle_level(rgb_bus->hal.dev, !io_config->flags.hsync_idle_low,
                              !io_config->flags.vsync_idle_low, io_config->flags.de_idle_high);
        // configure blank region timing
        lcd_ll_set_blank_cycles(rgb_bus->hal.dev, 1, 1); // RGB panel always has a front and back blank (porch region)
        lcd_ll_set_horizontal_timing(rgb_bus->hal.dev, io_config->hsync_pulse_width,
                                     io_config->hsync_back_porch, rgb_bus->h_res * rgb_bus->output_bits_per_pixel / rgb_bus->data_width,
                                     io_config->hsync_front_porch);
        lcd_ll_set_vertical_timing(rgb_bus->hal.dev, io_config->vsync_pulse_width,
                                   io_config->vsync_back_porch, rgb_bus->v_res,
                                   io_config->vsync_front_porch);
        // output hsync even in porch region
        lcd_ll_enable_output_hsync_in_porch_region(rgb_bus->hal.dev, true);
        // generate the hsync at the very beginning of line
        lcd_ll_set_hsync_position(rgb_bus->hal.dev, 0);
        // send next frame automatically in stream mode
        lcd_ll_enable_auto_next_frame(rgb_bus->hal.dev, rgb_bus->flags.stream_mode);
        // trigger interrupt on the end of frame
        lcd_ll_enable_interrupt(rgb_bus->hal.dev, LCD_LL_EVENT_VSYNC_END, true);
        // enable intr
        esp_intr_enable(rgb_bus->intr);
        // start transmission
        if (rgb_bus->flags.stream_mode) {
            rgb_bus_start_transmission(rgb_bus);
        }
        ESP_LOGD(TAG, "rgb panel(%d) start, pclk=%"PRIu32"Hz", rgb_bus->panel_id, rgb_bus->pclk_hz);

        lcd_panel_io_rgb_t *rgb_device = (lcd_panel_io_rgb_t *)heap_caps_calloc(1, sizeof(lcd_panel_io_rgb_t), MALLOC_CAP_INTERNAL);
        rgb_device->base.del = rgb_panel_io_del;
        rgb_device->base.register_event_callbacks = rgb_panel_io_register_callbacks;
        rgb_device->base.tx_color = rgb_panel_io_tx_color;

        rgb_device->bus = rgb_bus;

        rgb_bus->panel_io = (esp_lcd_panel_io_t *)rgb_device;
        *ret_io = &(rgb_device->base);
        return ret;
    }

    esp_err_t rgb_panel_io_tx_color(esp_lcd_panel_io_t *io, int lcd_cmd, const void *color, size_t color_size)
    {
        lcd_panel_io_rgb_t *rgb_device = __containerof(io, lcd_panel_io_rgb_t, base);
        esp_lcd_rgb_bus_t *rgb_bus = rgb_device->bus;

        if (color == rgb_bus->fbs[0]) {
            rgb_bus->cur_fb_index = 0;
        } else if (color == rgb_bus->fbs[1]) {
            rgb_bus->cur_fb_index = 1;
        } else if (color == rgb_bus->fbs[2]) {
            rgb_bus->cur_fb_index = 2;
        } else {
            return ESP_OK;
        }

        // round the boundary
        int h_res = rgb_bus->h_res;
        int v_res = rgb_bus->v_res;

        int bytes_per_pixel = rgb_bus->fb_bits_per_pixel / 8;

        uint8_t *fb = rgb_bus->fbs[rgb_bus->cur_fb_index];
        size_t bytes_to_flush = v_res * h_res * bytes_per_pixel;
        uint8_t *flush_ptr = fb;

        // Note that if we use a bounce buffer, the data gets read by the CPU as well so no need to write back
        if (rgb_bus->flags.fb_in_psram) {
            // CPU writes data to PSRAM through DCache, data in PSRAM might not get updated, so write back
            ESP_RETURN_ON_ERROR(esp_cache_msync(flush_ptr, bytes_to_flush, 0), TAG, "flush cache buffer failed");
        }

        if (rgb_bus->flags.stream_mode) {
            // the DMA will convey the new frame buffer next time
            for (int i = 0; i < RGB_BUS_LCD_PANEL_DMA_LINKS_REPLICA; i++) {
                rgb_bus->dma_nodes[rgb_bus->num_dma_nodes * (i + 1) - 1].next = rgb_bus->dma_links[rgb_bus->cur_fb_index];
            }
        }

        return ESP_OK;
    }

    static esp_err_t rgb_bus_configure_gpio(esp_lcd_rgb_bus_t *rgb_bus, const esp_lcd_rgb_bus_config_t *bus_config)
    {
        int panel_id = rgb_bus->panel_id;
        // check validation of GPIO number
        bool valid_gpio = true;
        if (bus_config->de_gpio_num < 0) {
            // Hsync and Vsync are required in HV mode
            valid_gpio = valid_gpio && (bus_config->hsync_gpio_num >= 0) && (bus_config->vsync_gpio_num >= 0);
        }
        for (size_t i = 0; i < bus_config->data_width; i++) {
            valid_gpio = valid_gpio && (bus_config->data_gpio_nums[i] >= 0);
        }
        if (!valid_gpio) {
            return ESP_ERR_INVALID_ARG;
        }
        // connect peripheral signals via GPIO matrix
        for (size_t i = 0; i < bus_config->data_width; i++) {
            gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[bus_config->data_gpio_nums[i]], PIN_FUNC_GPIO);
            gpio_set_direction(bus_config->data_gpio_nums[i], GPIO_MODE_OUTPUT);
            esp_rom_gpio_connect_out_signal(bus_config->data_gpio_nums[i],
                                            lcd_periph_signals.panels[panel_id].data_sigs[i], false, false);
        }
        if (bus_config->hsync_gpio_num >= 0) {
            gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[bus_config->hsync_gpio_num], PIN_FUNC_GPIO);
            gpio_set_direction(bus_config->hsync_gpio_num, GPIO_MODE_OUTPUT);
            esp_rom_gpio_connect_out_signal(bus_config->hsync_gpio_num,
                                            lcd_periph_signals.panels[panel_id].hsync_sig, false, false);
        }
        if (bus_config->vsync_gpio_num >= 0) {
            gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[bus_config->vsync_gpio_num], PIN_FUNC_GPIO);
            gpio_set_direction(bus_config->vsync_gpio_num, GPIO_MODE_OUTPUT);
            esp_rom_gpio_connect_out_signal(bus_config->vsync_gpio_num,
                                            lcd_periph_signals.panels[panel_id].vsync_sig, false, false);
        }
        // PCLK may not be necessary in some cases (i.e. VGA output)
        if (bus_config->pclk_gpio_num >= 0) {
            gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[bus_config->pclk_gpio_num], PIN_FUNC_GPIO);
            gpio_set_direction(bus_config->pclk_gpio_num, GPIO_MODE_OUTPUT);
            esp_rom_gpio_connect_out_signal(bus_config->pclk_gpio_num,
                                            lcd_periph_signals.panels[panel_id].pclk_sig, false, false);
        }
        // DE signal might not be necessary for some RGB LCD
        if (bus_config->de_gpio_num >= 0) {
            gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[bus_config->de_gpio_num], PIN_FUNC_GPIO);
            gpio_set_direction(bus_config->de_gpio_num, GPIO_MODE_OUTPUT);
            esp_rom_gpio_connect_out_signal(bus_config->de_gpio_num,
                                            lcd_periph_signals.panels[panel_id].de_sig, false, false);
        }
        return ESP_OK;
    }

    esp_err_t rgb_bus_select_clock_src(esp_lcd_rgb_bus_t *rgb_bus, lcd_clock_source_t clk_src)
    {
        // get clock source frequency
        uint32_t src_clk_hz = 0;
        ESP_RETURN_ON_ERROR(esp_clk_tree_src_get_freq_hz((soc_module_clk_t)clk_src, ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED, &src_clk_hz),
                            TAG, "get clock source frequency failed");
        rgb_bus->src_clk_hz = src_clk_hz;
        lcd_ll_select_clk_src(rgb_bus->hal.dev, clk_src);

        // create pm lock based on different clock source
        // clock sources like PLL and XTAL will be turned off in light sleep
    #if CONFIG_PM_ENABLE
        ESP_RETURN_ON_ERROR(esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "rgb_panel", &rgb_bus->pm_lock), TAG, "create pm lock failed");
        // hold the lock during the whole lifecycle of RGB panel
        esp_pm_lock_acquire(rgb_bus->pm_lock);
        ESP_LOGD(TAG, "installed pm lock and hold the lock during the whole panel lifecycle");
    #endif

        return ESP_OK;
    }


    // If we restart GDMA, many pixels already have been transferred to the LCD peripheral.
    // Looks like that has 16 pixels of FIFO plus one holding register.
    #define RGB_FIFO_PRESERVE_SIZE_PX (GDMA_LL_L2FIFO_BASE_SIZE + 1)


    static esp_err_t rgb_bus_create_trans_linkt_clock_src(esp_lcd_rgb_bus_t *rgb_bus)
    {
        for (int i = 0; i < RGB_BUS_LCD_PANEL_DMA_LINKS_REPLICA; i++) {
            rgb_bus->dma_links[i] = &rgb_bus->dma_nodes[rgb_bus->num_dma_nodes * i];
        }
        // chain DMA descriptors
        for (int i = 0; i < rgb_bus->num_dma_nodes * RGB_BUS_LCD_PANEL_DMA_LINKS_REPLICA; i++) {
            rgb_bus->dma_nodes[i].dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_CPU;
            rgb_bus->dma_nodes[i].next = &rgb_bus->dma_nodes[i + 1];
        }


        if (rgb_bus->flags.stream_mode) {
            // circle DMA descriptors chain for each frame buffer
            for (int i = 0; i < RGB_BUS_LCD_PANEL_DMA_LINKS_REPLICA; i++) {
                rgb_bus->dma_nodes[rgb_bus->num_dma_nodes * (i + 1) - 1].next = &rgb_bus->dma_nodes[rgb_bus->num_dma_nodes * i];
            }
        } else {
            // one-off DMA descriptors chain
            for (int i = 0; i < RGB_BUS_LCD_PANEL_DMA_LINKS_REPLICA; i++) {
                rgb_bus->dma_nodes[rgb_bus->num_dma_nodes * (i + 1) - 1].next = NULL;
            }
        }
        // mount the frame buffer to the DMA descriptors
        for (size_t i = 0; i < rgb_bus->num_fbs; i++) {
            lcd_com_mount_dma_data(rgb_bus->dma_links[i], rgb_bus->fbs[i], rgb_bus->fb_size);
        }

        // On restart, the data sent to the LCD peripheral needs to start LCD_FIFO_PRESERVE_SIZE_PX pixels after the FB start
        // so we use a dedicated DMA node to restart the DMA transaction
        memcpy(&rgb_bus->dma_restart_node, &rgb_bus->dma_nodes[0], sizeof(rgb_bus->dma_restart_node));
        int restart_skip_bytes = RGB_FIFO_PRESERVE_SIZE_PX * sizeof(uint16_t);
        uint8_t *p = (uint8_t *)rgb_bus->dma_restart_node.buffer;
        rgb_bus->dma_restart_node.buffer = &p[restart_skip_bytes];
        rgb_bus->dma_restart_node.dw0.length -= restart_skip_bytes;
        rgb_bus->dma_restart_node.dw0.size -= restart_skip_bytes;

        // alloc DMA channel and connect to LCD peripheral
        gdma_channel_alloc_config_t dma_chan_config = {
            .direction = GDMA_CHANNEL_DIRECTION_TX,
        };
    #if SOC_GDMA_TRIG_PERIPH_LCD0_BUS == SOC_GDMA_BUS_AHB
        ESP_RETURN_ON_ERROR(gdma_new_ahb_channel(&dma_chan_config, &rgb_bus->dma_chan), TAG, "alloc DMA channel failed");
    #elif SOC_GDMA_TRIG_PERIPH_LCD0_BUS == SOC_GDMA_BUS_AXI
        ESP_RETURN_ON_ERROR(gdma_new_axi_channel(&dma_chan_config, &rgb_bus->dma_chan), TAG, "alloc DMA channel failed");
    #endif
        gdma_connect(rgb_bus->dma_chan, GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_LCD, 0));
        gdma_transfer_ability_t ability = {
            .psram_trans_align = rgb_bus->psram_trans_align,
            .sram_trans_align = rgb_bus->sram_trans_align,
        };
        gdma_set_transfer_ability(rgb_bus->dma_chan, &ability);

        return ESP_OK;
    }

    // reset the GDMA channel every VBlank to stop permanent desyncs from happening.
    // Note that this fix can lead to single-frame desyncs itself, as in: if this interrupt
    // is late enough, the display will shift as the LCD controller already read out the
    // first data bytes, and resetting DMA will re-send those. However, the single-frame
    // desync this leads to is preferable to the permanent desync that could otherwise
    // happen. It's also not super-likely as this interrupt has the entirety of the VBlank
    // time to reset DMA.

    static void rgb_bus_start_transmission(esp_lcd_rgb_bus_t *rgb_bus)
    {
        // reset FIFO of DMA and LCD, incase there remains old frame data
        gdma_reset(rgb_bus->dma_chan);
        lcd_ll_stop(rgb_bus->hal.dev);
        lcd_ll_fifo_reset(rgb_bus->hal.dev);

        // the start of DMA should be prior to the start of LCD engine
        gdma_start(rgb_bus->dma_chan, (intptr_t)rgb_bus->dma_links[rgb_bus->cur_fb_index]);
        // delay 1us is sufficient for DMA to pass data to LCD FIFO
        // in fact, this is only needed when LCD pixel clock is set too high
        esp_rom_delay_us(1);
        // start LCD engine
        lcd_ll_start(rgb_bus->hal.dev);
    }


    IRAM_ATTR static void lcd_rgb_panel_try_update_pclk(esp_lcd_rgb_bus_t *rgb_bus)
    {
        portENTER_CRITICAL_ISR(&rgb_bus->spinlock);
        if (unlikely(rgb_bus->flags.need_update_pclk)) {
            rgb_bus->flags.need_update_pclk = false;
            rgb_bus->pclk_hz = lcd_hal_cal_pclk_freq(&rgb_bus->hal, rgb_bus->src_clk_hz, rgb_bus->pclk_hz, rgb_bus->lcd_clk_flags);
        }
        portEXIT_CRITICAL_ISR(&rgb_bus->spinlock);
    }


    IRAM_ATTR static void rgb_bus_default_isr_handler(void *args)
    {
        esp_lcd_rgb_bus_t *rgb_bus = (esp_lcd_rgb_bus_t *)args;
        bool need_yield = false;

        uint32_t intr_status = lcd_ll_get_interrupt_status(rgb_bus->hal.dev);
        lcd_ll_clear_interrupt_status(rgb_bus->hal.dev, intr_status);
        if (intr_status & LCD_LL_EVENT_VSYNC_END) {
            // call user registered callback
            if (rgb_bus->on_vsync) {
                if (rgb_bus->fbs[rgb_bus->cur_fb_index] != rgb_bus->cur_buf) {
                    rgb_bus->cur_buf = rgb_bus->fbs[rgb_bus->cur_fb_index];
                    rgb_bus->swapped_framebuf = true;
                }
                if (rgb_bus->on_vsync(rgb_bus->panel_io, NULL, rgb_bus->user_ctx)) {
                    need_yield = true;
                }
            }

            // check whether to update the PCLK frequency, it should be safe to update the PCLK frequency in the VSYNC interrupt
            lcd_rgb_panel_try_update_pclk(rgb_bus);

        }
        if (need_yield) {
            portYIELD_FROM_ISR();
        }
    }
#endif

