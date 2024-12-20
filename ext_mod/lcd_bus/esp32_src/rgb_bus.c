#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED

    // local includes
    #include "lcd_types.h"
    #include "modlcd_bus.h"
    #include "rgb_bus.h"
    #include "bus_task.h"
    #include "rotation.h"

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
    #include "rom/ets_sys.h"
    #include "esp_system.h"
    #include "esp_cpu.h"

    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"
    #include "py/runtime.h"
    #include "py/objarray.h"
    #include "py/binary.h"
    #include "py/objint.h"
    #include "py/objstr.h"
    #include "py/objtype.h"
    #include "py/objexcept.h"
    #include "py/gc.h"
    #include "py/stackctrl.h"

    // stdlib includes
    #include <string.h>

    #define DEFAULT_STACK_SIZE    (5 * 1024)

    mp_lcd_err_t rgb_del(mp_obj_t obj);
    mp_lcd_err_t rgb_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits, bool sw_rotation);
    mp_lcd_err_t rgb_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
    mp_lcd_err_t rgb_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t rgb_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size, bool is_flush, bool last_flush_cmd);
    mp_lcd_err_t rgb_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update);
    mp_obj_t rgb_allocate_framebuffer(mp_obj_t obj, uint32_t size, uint32_t caps);
    mp_obj_t rgb_free_framebuffer(mp_obj_t obj, mp_obj_t buf);

    typedef struct {
        esp_lcd_panel_t base;  // Base class of generic lcd panel
        int panel_id;          // LCD panel ID
        lcd_hal_context_t hal; // Hal layer object
        size_t data_width;     // Number of data lines
        size_t fb_bits_per_pixel; // Frame buffer color depth, in bpp
        size_t num_fbs;           // Number of frame buffers
        size_t output_bits_per_pixel; // Color depth seen from the output data line. Default to fb_bits_per_pixel, but can be changed by YUV-RGB conversion
        size_t sram_trans_align;  // Alignment for framebuffer that allocated in SRAM
        size_t psram_trans_align; // Alignment for framebuffer that allocated in PSRAM
        int disp_gpio_num;     // Display control GPIO, which is used to perform action like "disp_off"
        intr_handle_t intr;    // LCD peripheral interrupt handle
        esp_pm_lock_handle_t pm_lock; // Power management lock
        size_t num_dma_nodes;  // Number of DMA descriptors that used to carry the frame buffer
        uint8_t *fbs[3]; // Frame buffers
        uint8_t cur_fb_index;  // Current frame buffer index
        uint8_t bb_fb_index;  // Current frame buffer index which used by bounce buffer
    } rgb_panel_t;


    static bool rgb_bus_trans_done_cb(esp_lcd_panel_handle_t panel,
                                    const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
    {
        LCD_UNUSED(edata);
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)user_ctx;
        rgb_panel_t *rgb_panel = __containerof(panel, rgb_panel_t, base);
        void *curr_buf = (void *)rgb_panel->fbs[rgb_panel->cur_fb_index];

        if (curr_buf != self->rotate_task->active_fb && !bus_event_isset_from_isr(&self->rotate_task->swap_bufs)) {
            void *idle_fb = self->rotate_task->idle_fb;
            self->rotate_task->idle_fb = self->rotate_task->active_fb;
            self->rotate_task->active_fb = idle_fb;
            bus_event_set_from_isr(&self->rotate_task->swap_bufs);
        }

        return false;
    }

    static void rgb_bus_rotate_task(void *self_in) {
        LCD_DEBUG_PRINT("rgb_bus_rotate_task - STARTED\n")

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)self_in;

        rotation_t *rotation = self->rotation;
        rotation_task_t *task = &rotation->task;
        rotation_buffer_t *buf = &rotation->buf;
        init_err_t *init = &rotation->init;
        rotation_data_t *data = &rotation->data;


        esp_lcd_rgb_panel_event_callbacks_t callbacks = { .on_vsync = rgb_bus_trans_done_cb };

        init->err = esp_lcd_new_rgb_panel(&self->panel_io_config, &self->panel_handle);
        if (init->err != 0) {
            init->err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_rgb_panel)");
            bus_lock_release(&task->init_lock);
            return;
        }

        init->err = esp_lcd_rgb_panel_register_event_callbacks(self->panel_handle, &callbacks, self);
        if (init->err != 0) {
            init->err_msg = MP_ERROR_TEXT("%d(esp_lcd_rgb_panel_register_event_callbacks)");
            bus_lock_release(&task->init_lock);
            return;
        }

        init->err = esp_lcd_panel_reset(self->panel_handle);
        if (init->err != 0) {
            init->err_msg = MP_ERROR_TEXT("%d(esp_lcd_panel_reset)");
            bus_lock_release(&task->init_lock);
            return;
        }

        init->err = esp_lcd_panel_init(self->panel_handle);
        if (init->err != 0) {
            init->err_msg = MP_ERROR_TEXT("%d(esp_lcd_panel_init)");
            bus_lock_release(&task->init_lock);
            return;
        }

        rgb_panel_t *rgb_panel = __containerof((esp_lcd_panel_t *)self->panel_handle, rgb_panel_t, base);

        buf->active = (void *)rgb_panel->fbs[0];
        buf->idle = (void *)rgb_panel->fbs[1];

        void *idle_fb;
        bool last_update;

        bus_lock_acquire(&task->lock, -1);

        init->err = LCD_OK;
        bus_lock_release(&task->init_lock);

        bool exit = bus_event_isset(&task->exit);
        while (!exit) {
            bus_lock_acquire(&task->lock, -1);

            if (buf->partial == NULL) break;
            last_update = (bool)data->last_update;


        #if LCD_RGB_OPTIMUM_FB_SIZE
            rotation->optimum.flush_count += 1;
        #endif

            idle_fb = buf->idle;

            rotate(buf->partial, idle_fb, data);

            bus_lock_release(&task->tx_color_lock);

            if (self->callback != mp_const_none) {
                volatile uint32_t sp = (uint32_t)esp_cpu_get_sp();

                void *old_state = mp_thread_get_state();

                mp_state_thread_t ts;
                mp_thread_set_state(&ts);
                mp_stack_set_top((void*)sp);
                mp_stack_set_limit(CONFIG_FREERTOS_IDLE_TASK_STACKSIZE - 1024);
                mp_locals_set(mp_state_ctx.thread.dict_locals);
                mp_globals_set(mp_state_ctx.thread.dict_globals);

                mp_sched_lock();
                gc_lock();

                nlr_buf_t nlr;
                if (nlr_push(&nlr) == 0) {
                    mp_call_function_n_kw(self->callback, 0, 0, NULL);
                    nlr_pop();
                } else {
                    ets_printf("Uncaught exception in IRQ callback handler!\n");
                    mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
                }

                gc_unlock();
                mp_sched_unlock();

                mp_thread_set_state(old_state);
            }

            if (last_update) {

            #if LCD_RGB_OPTIMUM_FB_SIZE
                if (rotation->optimum.curr_index == 254) {
                    rotation->optimum.curr_index = 0;
                } else {
                    rotation->optimum.curr_index += 1;
                }
                if (rotation->optimum.sample_count < 255) {
                    rotation->optimum.sample_count += 1;
                }
                rotation->optimum.samples[rotation->optimum.curr_index] = rotation->optimum.flush_count;
                rotation->optimum.flush_count = 0;

                bus_lock_release(&rotation->optimum.lock);
                bus_lock_acquire(&rotation->optimum.lock, -1);
            #endif

                mp_lcd_err_t ret = esp_lcd_panel_draw_bitmap(
                    self->panel_handle,
                    0,
                    0,
                    data->width - 1,
                    data->height - 1,
                    idle_fb
                );

                if (ret != 0) {
                    mp_printf(&mp_plat_print, "esp_lcd_panel_draw_bitmap error (%d)\n", ret);
                } else {
                    bus_event_clear(&task->swap_bufs);
                    bus_event_wait(&task->swap_bufs);
                    memcpy(buf->idle, buf->active, data->width * data->height * data->bytes_per_pixel);
                }
            }

            exit = bus_event_isset(&task->exit);
        }

        LCD_DEBUG_PRINT(&mp_plat_print, "rgb_bus_copy_task - STOPPED\n")
    }

    mp_obj_t mp_lcd_rgb_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        enum {
            ARG_hsync,
            ARG_vsync,
            ARG_de,
            ARG_pclk,
            ARG_data0,
            ARG_data1,
            ARG_data2,
            ARG_data3,
            ARG_data4,
            ARG_data5,
            ARG_data6,
            ARG_data7,
            ARG_data8,
            ARG_data9,
            ARG_data10,
            ARG_data11,
            ARG_data12,
            ARG_data13,
            ARG_data14,
            ARG_data15,
            ARG_freq,
            ARG_hsync_front_porch,
            ARG_hsync_back_porch,
            ARG_hsync_pulse_width,
            ARG_hsync_idle_low,
            ARG_vsync_front_porch,
            ARG_vsync_back_porch,
            ARG_vsync_pulse_width,
            ARG_vsync_idle_low,
            ARG_de_idle_high,
            ARG_pclk_idle_high,
            ARG_pclk_active_low,
            ARG_refresh_on_demand,
        };

        const mp_arg_t allowed_args[] = {
            { MP_QSTR_hsync,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_vsync,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_de,                 MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_pclk,               MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data0,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data1,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data2,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data3,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data4,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data5,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data6,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data7,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data8,              MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data9,              MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data10,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data11,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data12,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data13,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data14,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data15,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
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
            { MP_QSTR_refresh_on_demand,  MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } },
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
        self->panel_io_config.data_gpio_nums[0] = (int)args[ARG_data0].u_int;
        self->panel_io_config.data_gpio_nums[1] = (int)args[ARG_data1].u_int;
        self->panel_io_config.data_gpio_nums[2] = (int)args[ARG_data2].u_int;
        self->panel_io_config.data_gpio_nums[3] = (int)args[ARG_data3].u_int;
        self->panel_io_config.data_gpio_nums[4] = (int)args[ARG_data4].u_int;
        self->panel_io_config.data_gpio_nums[5] = (int)args[ARG_data5].u_int;
        self->panel_io_config.data_gpio_nums[6] = (int)args[ARG_data6].u_int;
        self->panel_io_config.data_gpio_nums[7] = (int)args[ARG_data7].u_int;
        self->panel_io_config.data_gpio_nums[8] = (int)args[ARG_data8].u_int;
        self->panel_io_config.data_gpio_nums[9] = (int)args[ARG_data9].u_int;
        self->panel_io_config.data_gpio_nums[10] = (int)args[ARG_data10].u_int;
        self->panel_io_config.data_gpio_nums[11] = (int)args[ARG_data11].u_int;
        self->panel_io_config.data_gpio_nums[12] = (int)args[ARG_data12].u_int;
        self->panel_io_config.data_gpio_nums[13] = (int)args[ARG_data13].u_int;
        self->panel_io_config.data_gpio_nums[14] = (int)args[ARG_data14].u_int;
        self->panel_io_config.data_gpio_nums[15] = (int)args[ARG_data15].u_int;
        self->panel_io_config.disp_gpio_num = -1;   // -1 means no GPIO is assigned to this function
        self->panel_io_config.sram_trans_align = 8;
        self->panel_io_config.psram_trans_align = 64;
        self->panel_io_config.flags.refresh_on_demand = (uint32_t)args[ARG_refresh_on_demand].u_bool;
        self->panel_io_config.flags.fb_in_psram = 0;
        self->panel_io_config.flags.double_fb = 0;

        int i = 0;
        for (; i < 16; i++) {
            if (self->panel_io_config.data_gpio_nums[i] == -1) {
                break;
            }
        }

        self->panel_io_config.data_width = (size_t) i;

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

        self->panel_io_handle.get_lane_count = &rgb_get_lane_count;
        self->panel_io_handle.del = &rgb_del;
        self->panel_io_handle.rx_param = &rgb_rx_param;
        self->panel_io_handle.tx_param = &rgb_tx_param;
        self->panel_io_handle.tx_color = &rgb_tx_color;
        self->panel_io_handle.allocate_framebuffer = &rgb_allocate_framebuffer;
        self->panel_io_handle.free_framebuffer = &rgb_free_framebuffer;
        self->panel_io_handle.init = &rgb_init;

        return MP_OBJ_FROM_PTR(self);
    }

    mp_lcd_err_t rgb_del(mp_obj_t obj)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        LCD_DEBUG_PRINT("rgb_del(self)\n")

        if (self->view1 != NULL || self->view2 != NULL) {
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Framebuffers have not been released"));
            return LCD_FAIL;
        }

        rgb_bus_lock_acquire(&self->rotation->task.tx_color_lock, -1);
        self->rotation->buf.partial = NULL;
        rgb_bus_event_set(&self->rotation->task.exit);
        rgb_bus_lock_release(&self->rotation->task.lock);
        rgb_bus_lock_release(&self->rotation->task.tx_color_lock);

        mp_lcd_err_t ret = esp_lcd_panel_del(self->panel_handle);

        rgb_bus_lock_delete(&self->rotation->task.lock);
        rgb_bus_lock_delete(&self->rotation->task.tx_color_lock);

        rgb_bus_event_clear(&self->rotation->task.swap_bufs);
        rgb_bus_event_delete(&self->rotation->task.swap_bufs);
        rgb_bus_event_delete(&self->rotation->task.exit);

        free(self->rotation);

        return ret;
    }

    mp_lcd_err_t rgb_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        LCD_UNUSED(obj);
        LCD_UNUSED(param);
        LCD_UNUSED(lcd_cmd);
        LCD_UNUSED(param_size);

        LCD_DEBUG_PRINT("rgb_rx_param(self, lcd_cmd=%d, param, param_size=%d)\n", lcd_cmd, param_size)
        return LCD_OK;
    }

    mp_lcd_err_t rgb_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size, bool is_flush, bool last_flush_cmd)
    {
        LCD_UNUSED(obj);
        LCD_UNUSED(param);
        LCD_UNUSED(lcd_cmd);
        LCD_UNUSED(param_size);
        LCD_UNUSED(is_flush);
        LCD_UNUSED(last_flush_cmd);
        LCD_DEBUG_PRINT("rgb_tx_param(self, lcd_cmd=%d, param, param_size=%d)\n", lcd_cmd, param_size)

        return LCD_OK;
    }

    mp_obj_t rgb_free_framebuffer(mp_obj_t obj, mp_obj_t buf)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        if (self->panel_handle != NULL) {
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Unable to free buffer"));
            return mp_const_none;
        }

        mp_obj_array_t *array_buf = (mp_obj_array_t *)MP_OBJ_TO_PTR(buf);
        void *item_buf = array_buf->items;

        if (array_buf == self->view1) {
            heap_caps_free(item_buf);
            self->view1 = NULL;
            LCD_DEBUG_PRINT("rgb_free_framebuffer(self, buf=1)\n")
        } else if (array_buf == self->view2) {
            heap_caps_free(item_buf);
            self->view2 = NULL;
            LCD_DEBUG_PRINT("rgb_free_framebuffer(self, buf=2)\n")
        } else {
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("No matching buffer found"));
        }
        return mp_const_none;
    }

    mp_obj_t rgb_allocate_framebuffer(mp_obj_t obj, uint32_t size, uint32_t caps)
    {
        LCD_DEBUG_PRINT("rgb_allocate_framebuffer(self, size=%lu, caps=%lu)\n", size, caps)

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        void *buf = heap_caps_calloc(1, size, caps);

        if (buf == NULL) {
           mp_raise_msg_varg(
               &mp_type_MemoryError,
               MP_ERROR_TEXT("Not enough memory available (%d)"),
               size
           );
        }

        mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, buf));
        view->typecode |= 0x80; // used to indicate writable buffer

        if (self->view1 == NULL) {
            self->view1 = view;
        } else if (self->view2 == NULL) {
            self->view2 = view;
        } else {
            heap_caps_free(buf);
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("There is a maximum of 2 frame buffers allowed"));
            return mp_const_none;
        }

        return MP_OBJ_FROM_PTR(view);
    }

    mp_lcd_err_t rgb_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits, bool sw_rotation)
    {
        LCD_UNUSED(cmd_bits);
        LCD_UNUSED(param_bits);
        LCD_UNUSED(sw_rotation);

        LCD_DEBUG_PRINT("rgb_init(self, width=%i, height=%i, bpp=%d, buffer_size=%lu, rgb565_byte_swap=%d)\n", width, height, bpp, buffer_size, rgb565_byte_swap)

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        self->rotation = (rotation_t *)malloc(sizeof(rotation_t));

        if (bpp == 16 && rgb565_byte_swap) {
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

                self->rgb565_byte_swap = 0;
            } else {
                self->rgb565_byte_swap = 1;
            }
        } else {
            self->rgb565_byte_swap = 0;
        }

        self->panel_io_config.timings.h_res = (uint32_t)width;
        self->panel_io_config.timings.v_res = (uint32_t)height;
        self->panel_io_config.bits_per_pixel = (size_t)bpp;

        self->rotation->data.width = width;
        self->rotation->data.height = height;
        self->rotation->data.bytes_per_pixel = bpp / 8;

        self->panel_io_config.flags.fb_in_psram = 1;
        self->panel_io_config.flags.double_fb = 1;

    #if LCD_RGB_OPTIMUM_FB_SIZE
        bus_lock_init(&self->rotation->optimum.lock);
        bus_lock_acquire(&self->rotation->optimum.lock, -1);
        self->rotation->optimum.samples = (uint16_t *)malloc(sizeof(uint16_t) * 255);
        self->rotation->optimum.curr_index = 254;
    #endif

        LCD_DEBUG_PRINT("h_res=%lu\n", self->panel_io_config.timings.h_res)
        LCD_DEBUG_PRINT("v_res=%lu\n", self->panel_io_config.timings.v_res)
        LCD_DEBUG_PRINT("bits_per_pixel=%d\n", self->panel_io_config.bits_per_pixel)
        LCD_DEBUG_PRINT("rgb565_byte_swap=%d\n", self->rgb565_byte_swap)

        rotation_task_start(rgb_bus_rotation_task, self->rotation, self);

        if (self->rotation->init_err.code != LCD_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, self->rotation->init_err.msg, self->rotation->init_err.code);
            return self->rotation->init_err.code;
        } else {
            return LCD_OK;
        }
    }


    mp_lcd_err_t rgb_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;
        *lane_count = (uint8_t)self->panel_io_config.data_width;

        LCD_DEBUG_PRINT("rgb_get_lane_count(self)-> %d\n", (uint8_t)self->panel_io_config.data_width)

        return LCD_OK;
    }


    mp_lcd_err_t rgb_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, int rotation, bool last_update)
    {
        LCD_DEBUG_PRINT("rgb_tx_color(self, lcd_cmd=%d, color, color_size=%d, x_start=%d, y_start=%d, x_end=%d, y_end=%d)\n", lcd_cmd, color_size, x_start, y_start, x_end, y_end)
        LCD_UNUSED(color_size);

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;
        
        bus_lock_acquire(&self->rotation->task.tx_color_lock, -1);

        self->rotation->data.last_update = (uint8_t)last_update;
        self->rotation->buf.partial = (void *)color;
        self->rotation->data.x_start = x_start;
        self->rotation->data.y_start = y_start;
        self->rotation->data.x_end = x_end;
        self->rotation->data.y_end = y_end;
        self->rotation->data.rotation = (uint8_t)rotation;

        bus_lock_release(&self->rotation->task.lock);
//        if (self->callback != mp_const_none) {
//            mp_call_function_n_kw(self->callback, 0, 0, NULL);
//        }

        return LCD_OK;
    }

#if LCD_RGB_OPTIMUM_FB_SIZE

    static void rgb_bus_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
    {
        mp_lcd_rgb_bus_obj_t *self = MP_OBJ_TO_PTR(self_in);

        rotation_optimum_t *optimum = &self->rotation->optimum;

        if (attr == MP_QSTR_avg_flushes_per_update) {
            if (dest[0] == MP_OBJ_NULL) {
                uint32_t total = 0;

                bus_lock_acquire(&optimum->lock, -1);
                for (uint8_t i=0;i<optimum->sample_count;i++) {
                    total += (uint32_t)optimum->samples[i];
                }

                uint16_t avg = (uint16_t)(total / (uint32_t)optimum->sample_count);

                bus_lock_release(&optimum->lock);

                dest[0] = mp_obj_new_int_from_uint(avg);
            } else if (dest[1]) {
                uint16_t value = (uint16_t)mp_obj_get_int_truncated(dest[1]);

                if (value == 0) {
                    bus_lock_acquire(&optimum->lock, -1);
                    for (uint8_t i=0;i<optimum->sample_count;i++) {
                        optimum->samples[i] = 0;
                    }

                    optimum->sample_count = 0;
                    optimum->curr_index = 254;
                    bus_lock_release(&optimum->lock);

                    dest[0] = MP_OBJ_NULL;
                }
            }
        } else if (dest[0] == MP_OBJ_NULL) {
            const mp_obj_type_t *type = mp_obj_get_type(self_in);
            while (MP_OBJ_TYPE_HAS_SLOT(type, locals_dict)) {
                // generic method lookup
                // this is a lookup in the object (ie not class or type)
                assert(MP_OBJ_TYPE_GET_SLOT(type, locals_dict)->base.type == &mp_type_dict); // MicroPython restriction, for now
                mp_map_t *locals_map = &MP_OBJ_TYPE_GET_SLOT(type, locals_dict)->map;
                mp_map_elem_t *elem = mp_map_lookup(locals_map, MP_OBJ_NEW_QSTR(attr), MP_MAP_LOOKUP);
                if (elem != NULL) {
                    mp_convert_member_lookup(self_in, type, elem->value, dest);
                    break;
                }
                if (MP_OBJ_TYPE_GET_SLOT_OR_NULL(type, parent) == NULL) break;
                // search parents
                type = MP_OBJ_TYPE_GET_SLOT(type, parent);
            }
        }
    }
#endif

    MP_DEFINE_CONST_OBJ_TYPE(
        mp_lcd_rgb_bus_type,
        MP_QSTR_RGBBus,
        MP_TYPE_FLAG_NONE,
        make_new, mp_lcd_rgb_bus_make_new,
    #if LCD_RGB_OPTIMUM_FB_SIZE
        attr, rgb_bus_attr,
    #endif
        locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
    );

#else
    #include "../common_src/rgb_bus.c"

#endif /*SOC_LCD_RGB_SUPPORTED*/
