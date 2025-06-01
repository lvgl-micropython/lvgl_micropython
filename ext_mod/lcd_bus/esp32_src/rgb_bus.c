// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED

    // local includes
    #include "lcd_types.h"
    #include "lcd_bus_task.h"
    #include "modlcd_bus.h"
    #include "rgb_bus.h"
    #include "bus_trans_done.h"

    // esp-idf includes
    #include "hal/lcd_hal.h"
    #include "esp_pm.h"
    #include "esp_intr_alloc.h"
    #include "esp_heap_caps.h"

    #include "esp_lcd_panel_io.h"
    #include "esp_lcd_panel_ops.h"
    #include "esp_lcd_panel_interface.h"
    #include "esp_lcd_panel_rgb.h"
    #include "esp_task.h"

    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"
    #include "py/runtime.h"

    // stdlib includes
    #include <string.h>


    typedef struct {
        esp_lcd_panel_t base;
        int panel_id;
        lcd_hal_context_t hal;
        size_t data_width;
        size_t fb_bits_per_pixel;
        size_t num_fbs;
        size_t output_bits_per_pixel;
        size_t sram_trans_align;
        size_t psram_trans_align;
        int disp_gpio_num;
        intr_handle_t intr;
        esp_pm_lock_handle_t pm_lock;
        size_t num_dma_nodes;
        uint8_t *fbs[3];
        uint8_t cur_fb_index;
        uint8_t bb_fb_index;
    } rgb_panel_t;


    mp_lcd_err_t rgb_del(mp_lcd_bus_obj_t *self_in);
    mp_lcd_err_t rgb_init(mp_lcd_bus_obj_t *self_in, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,
                          uint8_t cmd_bits, uint8_t param_bits);


    static bool rgb_bus_trans_done_cb(esp_lcd_panel_handle_t panel,
                                    const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
    {
        LCD_UNUSED(edata);
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)user_ctx;
        rgb_panel_t *rgb_panel = __containerof(panel, rgb_panel_t, base);
        uint8_t *curr_buf = rgb_panel->fbs[rgb_panel->cur_fb_index];

        if (curr_buf != self->bufs.active && !lcd_bus_event_isset_from_isr(&self->tx_data.swap_bufs)) {
            uint8_t *idle_fb = self->bufs.idle;
            self->bufs.idle = self->bufs.active;
            self->bufs.active = idle_fb;
            lcd_bus_event_set_from_isr(self->tx_data.swap_bufs);
        }

        return false;
    }

    static void rgb_send_func(mp_lcd_bus_obj_t *self_in, int cmd, uint8_t *params, size_t params_len)
    {
        LCD_UNUSED(cmd);
        LCD_UNUSED(params_len);
    }

    static void rgb_flush_func(mp_lcd_bus_obj_t *self_in, rotation_data_t *r_data, rotation_data_t *original_r_data,
                               uint8_t *idle_fb, uint8_t last_update)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *) self_in;

        if (self->callback != mp_const_none) {
            cb_isr(self->callback);
        }

        if (last_update) {
            mp_lcd_err_t ret = esp_lcd_panel_draw_bitmap(self->panel_handle, 0, 0,
                                original_r_data->dst_width, original_r_data->dst_height,
                                idle_fb);

            if (ret != 0) {
                mp_printf(&mp_plat_print, "esp_lcd_panel_draw_bitmap error (%d)\n", ret);
            } else {
                lcd_bus_event_clear(self->tx_data.swap_bufs);
                lcd_bus_event_wait(self->tx_data.swap_bufs);

                size_t size = (size_t)(original_r_data->dst_width * original_r_data->dst_height * original_r_data->bytes_per_pixel);
                memcpy(self->bufs.idle, self->bufs.active, size);
            }
        }
    }

    static bool rgb_init_func(mp_lcd_bus_obj_t *self_in)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *) self_in;

        esp_lcd_rgb_panel_event_callbacks_t callbacks = { .on_vsync = rgb_bus_trans_done_cb };

        self->init.err = esp_lcd_new_rgb_panel(&self->panel_io_config, &self->panel_handle);
        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_rgb_panel)");
            return false;
        }

        self->init.err = esp_lcd_rgb_panel_register_event_callbacks(self->panel_handle, &callbacks, self);
        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_rgb_panel_register_event_callbacks)");
            return false;
        }

        self->init.err = esp_lcd_panel_reset(self->panel_handle);
        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_panel_reset)");
            return false;
        }

        self->init.err = esp_lcd_panel_init(self->panel_handle);
        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_panel_init)");
            return false;
        }

        rgb_panel_t *rgb_panel = __containerof((esp_lcd_panel_t *)self->panel_handle, rgb_panel_t, base);

        self->bufs.active = rgb_panel->fbs[0];
        self->bufs.idle = rgb_panel->fbs[1];

        return true;
    }

    mp_obj_t mp_lcd_rgb_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        enum { ARG_hsync, ARG_vsync, ARG_de, ARG_pclk, ARG_data_pins, ARG_freq, ARG_hsync_front_porch, ARG_hsync_back_porch,
               ARG_hsync_pulse_width, ARG_hsync_idle_low, ARG_vsync_front_porch, ARG_vsync_back_porch, ARG_vsync_pulse_width,
               ARG_vsync_idle_low, ARG_de_idle_high, ARG_pclk_idle_high, ARG_pclk_active_low, ARG_refresh_on_demand };

        const mp_arg_t allowed_args[] = {
            { MP_QSTR_hsync,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_vsync,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_de,                 MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_pclk,               MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data_pins,          MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_freq,               MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 8000000 } },
            { MP_QSTR_hsync_front_porch,  MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0       } },
            { MP_QSTR_hsync_back_porch,   MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0       } },
            { MP_QSTR_hsync_pulse_width,  MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 1       } },
            { MP_QSTR_hsync_idle_low,     MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } },
            { MP_QSTR_vsync_front_porch,  MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0       } },
            { MP_QSTR_vsync_back_porch,   MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0       } },
            { MP_QSTR_vsync_pulse_width,  MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 1       } },
            { MP_QSTR_vsync_idle_low,     MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } },
            { MP_QSTR_de_idle_high,       MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } },
            { MP_QSTR_pclk_idle_high,     MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } },
            { MP_QSTR_pclk_active_low,    MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } },
            { MP_QSTR_refresh_on_demand,  MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } }
        };

        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        // create new object
        mp_lcd_rgb_bus_obj_t *self = m_new_obj(mp_lcd_rgb_bus_obj_t);
        self->base.type = &mp_lcd_rgb_bus_type;

        self->callback = mp_const_none;
        self->bus_config.pclk_hz = (uint32_t)args[ARG_freq].u_int;
        self->bus_config.hsync_pulse_width = (uint32_t)args[ARG_hsync_pulse_width].u_int;
        self->bus_config.hsync_back_porch = (uint32_t)args[ARG_hsync_back_porch].u_int;
        self->bus_config.hsync_front_porch = (uint32_t)args[ARG_hsync_front_porch].u_int;
        self->bus_config.vsync_pulse_width = (uint32_t)args[ARG_vsync_pulse_width].u_int;
        self->bus_config.vsync_back_porch = (uint32_t)args[ARG_vsync_back_porch].u_int;
        self->bus_config.vsync_front_porch = (uint32_t)args[ARG_vsync_front_porch].u_int;
        self->bus_config.flags.hsync_idle_low = (uint32_t)args[ARG_hsync_idle_low].u_bool;
        self->bus_config.flags.vsync_idle_low = (uint32_t)args[ARG_vsync_idle_low].u_bool;
        self->bus_config.flags.de_idle_high = (uint32_t)args[ARG_de_idle_high].u_bool;
        self->bus_config.flags.pclk_active_neg = (uint32_t)args[ARG_pclk_active_low].u_bool;
        self->bus_config.flags.pclk_idle_high = (uint32_t)args[ARG_pclk_idle_high].u_bool;

        self->panel_io_config.clk_src = LCD_CLK_SRC_PLL160M;
        self->panel_io_config.timings = self->bus_config;
        self->panel_io_config.hsync_gpio_num = (int)args[ARG_hsync].u_int;
        self->panel_io_config.vsync_gpio_num = (int)args[ARG_vsync].u_int;
        self->panel_io_config.de_gpio_num = (int)args[ARG_de].u_int;
        self->panel_io_config.pclk_gpio_num = (int)args[ARG_pclk].u_int;


        mp_obj_tuple_t *tuple = MP_OBJ_TO_PTR(args[ARG_data_pins].u_obj);
        size_t i = 0;

        if (tuple->len != 8 && tuple->len != 16) {
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("8 or 16 data pins are required"));
        }

        for (; i < tuple->len; i++) {
            self->panel_io_config.data_gpio_nums[i] = (int)mp_obj_get_int(tuple->items[i]);
        }

        for (i++; i < 16; i++) {
            self->panel_io_config.data_gpio_nums[i] = -1;
        }

        self->panel_io_config.data_width = tuple->len;
        self->num_lanes = (uint8_t)tuple->len;

        self->panel_io_config.disp_gpio_num = -1;   // -1 means no GPIO is assigned to this function
        self->panel_io_config.sram_trans_align = 8;
        self->panel_io_config.psram_trans_align = 64;
        self->panel_io_config.flags.refresh_on_demand = (uint32_t)args[ARG_refresh_on_demand].u_bool;
        self->panel_io_config.flags.fb_in_psram = 0;
        self->panel_io_config.flags.double_fb = 0;

        LCD_DEBUG_PRINT("pclk_hz=%lu\n", self->bus_config.pclk_hz)
        LCD_DEBUG_PRINT("hsync_pulse_width=%lu\n", self->bus_config.hsync_pulse_width)
        LCD_DEBUG_PRINT("hsync_back_porch=%lu\n", self->bus_config.hsync_back_porch)
        LCD_DEBUG_PRINT("hsync_front_porch=%lu\n", self->bus_config.hsync_front_porch)
        LCD_DEBUG_PRINT("vsync_pulse_width=%lu\n", self->bus_config.vsync_pulse_width)
        LCD_DEBUG_PRINT("vsync_back_porch=%lu\n", self->bus_config.vsync_back_porch)
        LCD_DEBUG_PRINT("vsync_front_porch=%lu\n", self->bus_config.vsync_front_porch)
        LCD_DEBUG_PRINT("hsync_idle_low=%d\n", self->bus_config.flags.hsync_idle_low)
        LCD_DEBUG_PRINT("vsync_idle_low=%d\n", self->bus_config.flags.vsync_idle_low)
        LCD_DEBUG_PRINT("de_idle_high=%d\n", self->bus_config.flags.de_idle_high)
        LCD_DEBUG_PRINT("pclk_active_neg=%d\n", self->bus_config.flags.pclk_active_neg)
        LCD_DEBUG_PRINT("pclk_idle_high=%d\n", self->bus_config.flags.pclk_idle_high)
        LCD_DEBUG_PRINT("clk_src=%d\n", self->panel_io_config.clk_src)
        LCD_DEBUG_PRINT("hsync_gpio_num=%d\n", self->panel_io_config.hsync_gpio_num)
        LCD_DEBUG_PRINT("vsync_gpio_num=%d\n", self->panel_io_config.vsync_gpio_num)
        LCD_DEBUG_PRINT("de_gpio_num=%d\n", self->panel_io_config.de_gpio_num)
        LCD_DEBUG_PRINT("pclk_gpio_num=%d\n", self->panel_io_config.pclk_gpio_num)
        LCD_DEBUG_PRINT("data_gpio_nums[0]=%d\n", self->panel_io_config.data_gpio_nums[0])
        LCD_DEBUG_PRINT("data_gpio_nums[1]=%d\n", self->panel_io_config.data_gpio_nums[1])
        LCD_DEBUG_PRINT("data_gpio_nums[2]=%d\n", self->panel_io_config.data_gpio_nums[2])
        LCD_DEBUG_PRINT("data_gpio_nums[3]=%d\n", self->panel_io_config.data_gpio_nums[3])
        LCD_DEBUG_PRINT("data_gpio_nums[4]=%d\n", self->panel_io_config.data_gpio_nums[4])
        LCD_DEBUG_PRINT("data_gpio_nums[5]=%d\n", self->panel_io_config.data_gpio_nums[5])
        LCD_DEBUG_PRINT("data_gpio_nums[6]=%d\n", self->panel_io_config.data_gpio_nums[6])
        LCD_DEBUG_PRINT("data_gpio_nums[7]=%d\n", self->panel_io_config.data_gpio_nums[7])
        LCD_DEBUG_PRINT("data_gpio_nums[8]=%d\n", self->panel_io_config.data_gpio_nums[8])
        LCD_DEBUG_PRINT("data_gpio_nums[9]=%d\n", self->panel_io_config.data_gpio_nums[9])
        LCD_DEBUG_PRINT("data_gpio_nums[10]=%d\n", self->panel_io_config.data_gpio_nums[10])
        LCD_DEBUG_PRINT("data_gpio_nums[11]=%d\n", self->panel_io_config.data_gpio_nums[11])
        LCD_DEBUG_PRINT("data_gpio_nums[12]=%d\n", self->panel_io_config.data_gpio_nums[12])
        LCD_DEBUG_PRINT("data_gpio_nums[13]=%d\n", self->panel_io_config.data_gpio_nums[13])
        LCD_DEBUG_PRINT("data_gpio_nums[14]=%d\n", self->panel_io_config.data_gpio_nums[14])
        LCD_DEBUG_PRINT("data_gpio_nums[15]=%d\n", self->panel_io_config.data_gpio_nums[15])
        LCD_DEBUG_PRINT("sram_trans_align=%d\n", self->panel_io_config.sram_trans_align)
        LCD_DEBUG_PRINT("psram_trans_align=%d\n", self->panel_io_config.psram_trans_align)
        LCD_DEBUG_PRINT("refresh_on_demand=%d\n", self->panel_io_config.flags.refresh_on_demand)
        LCD_DEBUG_PRINT("fb_in_psram=%d\n", self->panel_io_config.flags.fb_in_psram)
        LCD_DEBUG_PRINT("double_fb=%d\n", self->panel_io_config.flags.double_fb)
        LCD_DEBUG_PRINT("data_width=%d\n", self->panel_io_config.data_width)

        self->internal_cb_funcs.deinit = &rgb_del;
        self->internal_cb_funcs.init = &rgb_init;

        self->tx_data.flush_func = rgb_flush_func;
        self->init.init_func = rgb_init_func;

        self->tx_cmds.send_func = &rgb_send_func;

        return MP_OBJ_FROM_PTR(self);
    }

    mp_lcd_err_t rgb_del(mp_lcd_bus_obj_t *self_in)
    {
        LCD_DEBUG_PRINT("rgb_del(self)\n")

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)self_in;

        if (self->panel_handle != NULL) {
            mp_lcd_err_t ret = esp_lcd_panel_del(self->panel_handle);

            if (ret != 0) {
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_del)"), ret);
            }
            self->panel_handle = NULL;

            return ret;
        } else {
            return LCD_FAIL;
        }
    }


    mp_lcd_err_t rgb_init(mp_lcd_bus_obj_t *self_in, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,
                          uint8_t cmd_bits, uint8_t param_bits)
    {
        LCD_UNUSED(cmd_bits);
        LCD_UNUSED(param_bits);

        LCD_DEBUG_PRINT("rgb_init(self, width=%i, height=%i, bpp=%d, buffer_size=%lu)\n", width, height, bpp, buffer_size)

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)self_in;

        LCD_DEBUG_PRINT("rgb565_byte_swap=%d\n", self->r_data.swap)

        if (self->r_data.swap) {
            /*
            We change the pins aound when the bus width is 16 and wanting to
            swap bytes. This does the same thing as moving the bytes around in
            the buffer but without having to iterate over the entire buffer to
            swap the bytes around. Swapping the bytes around on larger displays
            has a serious performance impact on the speed. Moving the pins
            eliminates the need to do that.
            */
            if (self->panel_io_config.data_width == 16) {
                int temp_pin;

                for (uint8_t i = 0; i < 8; i++) {
                    temp_pin = self->panel_io_config.data_gpio_nums[i];
                    self->panel_io_config.data_gpio_nums[i] = self->panel_io_config.data_gpio_nums[i + 8];
                    self->panel_io_config.data_gpio_nums[i + 8] = temp_pin;
                }

                self->r_data.swap = 0;
            } else {
                self->r_data.swap = 1;
            }
        }

        self->panel_io_config.timings.h_res = (uint32_t)width;
        self->panel_io_config.timings.v_res = (uint32_t)height;
        self->panel_io_config.bits_per_pixel = (size_t)bpp;

        self->r_data.dst_height = (uint16_t)height;
        self->r_data.dst_width = (uint16_t)width;

        self->panel_io_config.flags.fb_in_psram = 1;
        self->panel_io_config.flags.double_fb = 1;

        LCD_DEBUG_PRINT("h_res=%lu\n", self->panel_io_config.timings.h_res)
        LCD_DEBUG_PRINT("v_res=%lu\n", self->panel_io_config.timings.v_res)
        LCD_DEBUG_PRINT("bits_per_pixel=%d\n", self->panel_io_config.bits_per_pixel)


        xTaskCreatePinnedToCore(
                lcd_bus_task, "rgb_task", LCD_DEFAULT_STACK_SIZE / sizeof(StackType_t),
                self, ESP_TASK_PRIO_MAX - 1, (TaskHandle_t *)&self->task.handle, 0);

        lcd_bus_lock_acquire(self->init.lock);
        lcd_bus_lock_release(self->init.lock);
        lcd_bus_lock_delete(self->init.lock);

        return self->init.err;
    }

    MP_DEFINE_CONST_OBJ_TYPE(
        mp_lcd_rgb_bus_type,
        MP_QSTR_RGBBus,
        MP_TYPE_FLAG_NONE,
        make_new, mp_lcd_rgb_bus_make_new,
        locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
    );

#else
    #include "../common_src/rgb_bus.c"

#endif /*SOC_LCD_RGB_SUPPORTED*/
