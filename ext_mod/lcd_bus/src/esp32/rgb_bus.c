// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED

    // local includes
    #include "common/lcd_types.h"
    #include "common/modlcd_bus.h"
    #include "esp32/rgb_bus.h"

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
    #include "py/objarray.h"
    #include "py/binary.h"
    #include "py/objint.h"
    #include "py/objstr.h"
    #include "py/objtype.h"
    #include "py/objexcept.h"

    #include "rgb565_dither.h"

    // stdlib includes
    #include <string.h>

    #define DEFAULT_STACK_SIZE    (5 * 1024)

    mp_lcd_err_t rgb_del(mp_obj_t obj);
    mp_lcd_err_t rgb_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
    mp_lcd_err_t rgb_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
    mp_lcd_err_t rgb_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t rgb_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t rgb_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update);

    static uint8_t rgb_bus_count = 0;
    static mp_lcd_rgb_bus_obj_t **rgb_bus_objs;


    void mp_lcd_rgb_bus_deinit_all(void)
    {
        // we need to copy the existing array to a new one so the order doesn't
        // get all mucked up when objects get removed.
        mp_lcd_rgb_bus_obj_t *objs[rgb_bus_count];

        for (uint8_t i=0;i<rgb_bus_count;i++) {
            objs[i] = rgb_bus_objs[i];
        }

        for (uint8_t i=0;i<rgb_bus_count;i++) {
            rgb_del(MP_OBJ_FROM_PTR(objs[i]));
        }
    }


    static bool rgb_trans_done_cb(esp_lcd_panel_handle_t panel,
                                const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
    {
        LCD_UNUSED(edata);
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)user_ctx;
        rgb_panel_t *rgb_panel = __containerof(panel, rgb_panel_t, base);
        uint8_t *curr_buf = rgb_panel->fbs[rgb_panel->cur_fb_index];

        if (curr_buf != self->active_fb && !mp_lcdbus_event_isset_from_isr(&self->swap_bufs)) {
            uint8_t *idle_fb = self->idle_fb;
            self->idle_fb = self->active_fb;
            self->active_fb = idle_fb;
            mp_lcdbus_event_set_from_isr(&self->swap_bufs);
        }

        return false;
    }


    static bool rgb_init_cb(void *self_in)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)self_in;

        esp_lcd_rgb_panel_event_callbacks_t callbacks = { .on_vsync = rgb_trans_done_cb };

        self->sw_rot->init.err = esp_lcd_new_rgb_panel(self->panel_io_config, &self->panel_handle);
        if (self->sw_rot->init.err != 0) {
            self->sw_rot->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_rgb_panel)");
            return false;
        }

        self->sw_rot->init.err = esp_lcd_rgb_panel_register_event_callbacks(self->panel_handle, &callbacks, self);
        if (self->sw_rot->init.err != 0) {
            self->sw_rot->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_rgb_panel_register_event_callbacks)");
            return false;
        }

        self->sw_rot->init.err = esp_lcd_panel_reset(self->panel_handle);
        if (self->sw_rot->init.err != 0) {
            self->sw_rot->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_panel_reset)");
            return false;
        }

        self->sw_rot->init.err = esp_lcd_panel_init(self->panel_handle);
        if (self->sw_rot->init.err != 0) {
            self->sw_rot->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_panel_init)");
            return false;
        }

        rgb_panel_t *rgb_panel = __containerof((esp_lcd_panel_t *)self->panel_handle, rgb_panel_t, base);

        self->sw_rot->buffers.active = rgb_panel->fbs[0];
        self->sw_rot->buffers.idle = rgb_panel->fbs[1];

        return true;
    }


    static void rgb_flush_cb(void *self_in, uint8_t last_update, int cmd, uint8_t *idle_fb)
    {
        LCD_UNUSED(cmd);

        if (last_update) {

            mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)self_in;

            mp_lcd_err_t ret = esp_lcd_panel_draw_bitmap(
                self->panel_handle,
                0,
                0,
                self->sw_rot->data.dst_width - 1,
                self->sw_rot->data.dst_height - 1,
                idle_fb
            );

            if (ret != 0) {
                mp_printf(&mp_plat_print, "esp_lcd_panel_draw_bitmap error (%d)\n", ret);
            } else {
                mp_lcd_event_clear(&self->sw_rot->handles.swap_bufs);
                mp_lcd_event_wait(&self->sw_rot->handles.swap_bufs);
                memcpy(self->sw_rot->buffers.idle, self->sw_rot->buffers.active,
                       self->sw_rot->data.dst_width * self->sw_rot->data.dst_height * bytes_per_pixel);
            }
        }
    }


    static mp_obj_t rgb_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
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
            ARG_refresh_on_demand
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
            { MP_QSTR_refresh_on_demand,  MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } }
        };

        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        // create new object
        mp_lcd_rgb_bus_obj_t *self = m_new_obj(mp_lcd_rgb_bus_obj_t);
        self->base.type = &mp_lcd_rgb_bus_type;

        self->callback = mp_const_none;

        self->sw_rot = malloc(sizeof(mp_lcd_sw_rotation_t));
        self->panel_io_config = malloc(sizeof(esp_lcd_rgb_panel_config_t));

        esp_lcd_rgb_panel_config_t *panel_io_config = self->panel_io_config;

        esp_lcd_rgb_timing_t timings = {
            .pclk_hz = (uint32_t)args[ARG_freq].u_int,
            .hsync_pulse_width = (uint32_t)args[ARG_hsync_pulse_width].u_int,
            .hsync_back_porch = (uint32_t)args[ARG_hsync_back_porch].u_int,
            .hsync_front_porch = (uint32_t)args[ARG_hsync_front_porch].u_int,
            .vsync_pulse_width = (uint32_t)args[ARG_vsync_pulse_width].u_int,
            .vsync_back_porch = (uint32_t)args[ARG_vsync_back_porch].u_int,
            .vsync_front_porch = (uint32_t)args[ARG_vsync_front_porch].u_int,
            .flags.hsync_idle_low = (uint32_t)args[ARG_hsync_idle_low].u_bool,
            .flags.vsync_idle_low = (uint32_t)args[ARG_vsync_idle_low].u_bool,
            .flags.de_idle_high = (uint32_t)args[ARG_de_idle_high].u_bool,
            .flags.pclk_active_neg = (uint32_t)args[ARG_pclk_active_low].u_bool,
            .flags.pclk_idle_high = (uint32_t)args[ARG_pclk_idle_high].u_bool
        }

        panel_io_config->clk_src = LCD_CLK_SRC_PLL160M;
        panel_io_config->timings = timings;
        panel_io_config->hsync_gpio_num = (int)args[ARG_hsync].u_int;
        panel_io_config->vsync_gpio_num = (int)args[ARG_vsync].u_int;
        panel_io_config->de_gpio_num = (int)args[ARG_de].u_int;
        panel_io_config->pclk_gpio_num = (int)args[ARG_pclk].u_int;
        panel_io_config->data_gpio_nums[0] = (int)args[ARG_data0].u_int;
        panel_io_config->data_gpio_nums[1] = (int)args[ARG_data1].u_int;
        panel_io_config->data_gpio_nums[2] = (int)args[ARG_data2].u_int;
        panel_io_config->data_gpio_nums[3] = (int)args[ARG_data3].u_int;
        panel_io_config->data_gpio_nums[4] = (int)args[ARG_data4].u_int;
        panel_io_config->data_gpio_nums[5] = (int)args[ARG_data5].u_int;
        panel_io_config->data_gpio_nums[6] = (int)args[ARG_data6].u_int;
        panel_io_config->data_gpio_nums[7] = (int)args[ARG_data7].u_int;
        panel_io_config->data_gpio_nums[8] = (int)args[ARG_data8].u_int;
        panel_io_config->data_gpio_nums[9] = (int)args[ARG_data9].u_int;
        panel_io_config->data_gpio_nums[10] = (int)args[ARG_data10].u_int;
        panel_io_config->data_gpio_nums[11] = (int)args[ARG_data11].u_int;
        panel_io_config->data_gpio_nums[12] = (int)args[ARG_data12].u_int;
        panel_io_config->data_gpio_nums[13] = (int)args[ARG_data13].u_int;
        panel_io_config->data_gpio_nums[14] = (int)args[ARG_data14].u_int;
        panel_io_config->data_gpio_nums[15] = (int)args[ARG_data15].u_int;
        panel_io_config->disp_gpio_num = -1;   // -1 means no GPIO is assigned to this function
        panel_io_config->sram_trans_align = 8;
        panel_io_config->psram_trans_align = 64;
        panel_io_config->flags.refresh_on_demand = (uint32_t)args[ARG_refresh_on_demand].u_bool;
        panel_io_config->flags.fb_in_psram = 0;
        panel_io_config->flags.double_fb = 0;

        int i = 0;
        for (; i < 16; i++) {
            if (panel_io_config->data_gpio_nums[i] == -1) {
                break;
            }
        }

        panel_io_config->data_width = (size_t) i;

        LCD_DEBUG_PRINT("pclk_hz=%lu\n", timings.pclk_hz)
        LCD_DEBUG_PRINT("hsync_pulse_width=%lu\n", timings.hsync_pulse_width)
        LCD_DEBUG_PRINT("hsync_back_porch=%lu\n", timings.hsync_back_porch)
        LCD_DEBUG_PRINT("hsync_front_porch=%lu\n", timings.hsync_front_porch)
        LCD_DEBUG_PRINT("vsync_pulse_width=%lu\n", timings.vsync_pulse_width)
        LCD_DEBUG_PRINT("vsync_back_porch=%lu\n", timings.vsync_back_porch)
        LCD_DEBUG_PRINT("vsync_front_porch=%lu\n", timings.vsync_front_porch)
        LCD_DEBUG_PRINT("hsync_idle_low=%d\n", timings.flags.hsync_idle_low)
        LCD_DEBUG_PRINT("vsync_idle_low=%d\n", timings.flags.vsync_idle_low)
        LCD_DEBUG_PRINT("de_idle_high=%d\n", timings.flags.de_idle_high)
        LCD_DEBUG_PRINT("pclk_active_neg=%d\n", timings.flags.pclk_active_neg)
        LCD_DEBUG_PRINT("pclk_idle_high=%d\n", timings.flags.pclk_idle_high)
        LCD_DEBUG_PRINT("clk_src=%d\n", panel_io_config->clk_src)
        LCD_DEBUG_PRINT("hsync_gpio_num=%d\n", panel_io_config->hsync_gpio_num)
        LCD_DEBUG_PRINT("vsync_gpio_num=%d\n", panel_io_config->vsync_gpio_num)
        LCD_DEBUG_PRINT("de_gpio_num=%d\n", panel_io_config->de_gpio_num)
        LCD_DEBUG_PRINT("pclk_gpio_num=%d\n", panel_io_config->pclk_gpio_num)
        LCD_DEBUG_PRINT("data_gpio_nums[0]=%d\n", panel_io_config->data_gpio_nums[0])
        LCD_DEBUG_PRINT("data_gpio_nums[1]=%d\n", panel_io_config->data_gpio_nums[1])
        LCD_DEBUG_PRINT("data_gpio_nums[2]=%d\n", panel_io_config->data_gpio_nums[2])
        LCD_DEBUG_PRINT("data_gpio_nums[3]=%d\n", panel_io_config->data_gpio_nums[3])
        LCD_DEBUG_PRINT("data_gpio_nums[4]=%d\n", panel_io_config->data_gpio_nums[4])
        LCD_DEBUG_PRINT("data_gpio_nums[5]=%d\n", panel_io_config->data_gpio_nums[5])
        LCD_DEBUG_PRINT("data_gpio_nums[6]=%d\n", panel_io_config->data_gpio_nums[6])
        LCD_DEBUG_PRINT("data_gpio_nums[7]=%d\n", panel_io_config->data_gpio_nums[7])
        LCD_DEBUG_PRINT("data_gpio_nums[8]=%d\n", panel_io_config->data_gpio_nums[8])
        LCD_DEBUG_PRINT("data_gpio_nums[9]=%d\n", panel_io_config->data_gpio_nums[9])
        LCD_DEBUG_PRINT("data_gpio_nums[10]=%d\n", panel_io_config->data_gpio_nums[10])
        LCD_DEBUG_PRINT("data_gpio_nums[11]=%d\n", panel_io_config->data_gpio_nums[11])
        LCD_DEBUG_PRINT("data_gpio_nums[12]=%d\n", panel_io_config->data_gpio_nums[12])
        LCD_DEBUG_PRINT("data_gpio_nums[13]=%d\n", panel_io_config->data_gpio_nums[13])
        LCD_DEBUG_PRINT("data_gpio_nums[14]=%d\n", panel_io_config->data_gpio_nums[14])
        LCD_DEBUG_PRINT("data_gpio_nums[15]=%d\n", panel_io_config->data_gpio_nums[15])
        LCD_DEBUG_PRINT("sram_trans_align=%d\n", panel_io_config->sram_trans_align)
        LCD_DEBUG_PRINT("psram_trans_align=%d\n", panel_io_config->psram_trans_align)
        LCD_DEBUG_PRINT("refresh_on_demand=%d\n", panel_io_config->flags.refresh_on_demand)
        LCD_DEBUG_PRINT("fb_in_psram=%d\n", panel_io_config->flags.fb_in_psram)
        LCD_DEBUG_PRINT("double_fb=%d\n", panel_io_config->flags.double_fb)
        LCD_DEBUG_PRINT("data_width=%d\n", panel_io_config->data_width)

        self->panel_io_handle.get_lane_count = &rgb_get_lane_count;
        self->panel_io_handle.del = &rgb_del;
        self->panel_io_handle.rx_param = &rgb_rx_param;
        self->panel_io_handle.tx_param = &rgb_tx_param;
        self->panel_io_handle.tx_color = &rgb_tx_color;
        self->panel_io_handle.init = &rgb_init;

        return MP_OBJ_FROM_PTR(self);
    }

    mp_lcd_err_t rgb_del(mp_obj_t obj)
    {
        LCD_DEBUG_PRINT("rgb_del(self)\n")

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        if (self->panel_handle != NULL) {
            mp_lcd_lock_acquire(&self->sw_rot->handles.tx_color_lock);
            self->partial_buf = NULL;
            mp_lcd_event_set(&self->sw_rot->handles.copy_task_exit);
            mp_lcd_lock_release(&self->sw_rot->handles.copy_lock);
            mp_lcd_lock_release(&self->sw_rot->handles.tx_color_lock);

            mp_lcd_err_t ret = esp_lcd_panel_del(self->panel_handle);

            if (ret != 0) {
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_del)"), ret);
            }
            self->panel_handle = NULL;

            mp_lcd_lock_delete(&self->sw_rot->handles.copy_lock);
            mp_lcd_lock_delete(&self->sw_rot->handles.tx_color_lock);

            mp_lcd_event_clear(&self->sw_rot->handles.swap_bufs);
            mp_lcd_event_delete(&self->sw_rot->handles.swap_bufs);
            mp_lcd_event_delete(&self->sw_rot->handles.copy_task_exit);

            uint8_t i = 0;

            for (;i<rgb_bus_count;i++) {
                if (rgb_bus_objs[i] == self) {
                    rgb_bus_objs[i] = NULL;
                    break;
                }
            }

            for (uint8_t j=i + 1;j<rgb_bus_count;j++) {
                rgb_bus_objs[j - i + 1] = rgb_bus_objs[j];
            }

            rgb_bus_count--;
            rgb_bus_objs = m_realloc(rgb_bus_objs, rgb_bus_count * sizeof(mp_lcd_rgb_bus_obj_t *));
            return ret;
        } else {
            return LCD_FAIL;
        }
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

    mp_lcd_err_t rgb_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        LCD_UNUSED(obj);
        LCD_UNUSED(param);
        LCD_UNUSED(lcd_cmd);
        LCD_UNUSED(param_size);
        LCD_DEBUG_PRINT("rgb_tx_param(self, lcd_cmd=%d, param, param_size=%d)\n", lcd_cmd, param_size)

        return LCD_OK;
    }


    mp_lcd_err_t rgb_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp,
                          uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits,
                          uint8_t param_bits, bool sw_rotate)
    {
        LCD_UNUSED(cmd_bits);
        LCD_UNUSED(param_bits);
        LCD_UNUSED(sw_rotate);

        LCD_DEBUG_PRINT("rgb_init(self, width=%i, height=%i, bpp=%d, buffer_size=%lu, rgb565_byte_swap=%d)\n", width, height, bpp, buffer_size, rgb565_byte_swap)

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        self->sw_rotate = 1;

        if (bpp == 16 && rgb565_byte_swap) {
            /*
            We change the pins aound when the bus width is 16 and wanting to
            swap bytes. This does the same thing as moving the bytes around in
            the buffer but without having to iterate over the entire buffer to
            swap the bytes around. Swapping the bytes around on larger displays
            has a serious performance impact on the speed. Moving the pins
            eliminates the need to do that.
            */
            if (self->panel_io_config->data_width == 16) {
                int temp_pin;

                for (uint8_t i = 0; i < 8; i++) {
                    temp_pin = self->panel_io_config->data_gpio_nums[i];
                    self->panel_io_config->data_gpio_nums[i] = self->panel_io_config->data_gpio_nums[i + 8];
                    self->panel_io_config->data_gpio_nums[i + 8] = temp_pin;
                }

                self->rgb565_byte_swap = false;
            } else {
                self->rgb565_byte_swap = true;
            }
        } else {
            self->rgb565_byte_swap = false;
        }

        self->panel_io_config->timings.h_res = (uint32_t)width;
        self->panel_io_config->timings.v_res = (uint32_t)height;
        self->panel_io_config->bits_per_pixel = (size_t)bpp;

        self->sw_rot->data.dst_width = (uint32_t)width;
        self->sw_rot->data.dst_height = (uint32_t)height;
        self->sw_rot->data.bytes_per_pixel = (uint8_t)(bpp / 8);
        self->sw_rot->init.cb = &rgb_init_cb;
        self->sw_rot->flush_cb = &rgb_flush_cb;

        self->panel_io_config->flags.fb_in_psram = 1;
        self->panel_io_config->flags.double_fb = 1;

        mp_lcd_lock_init(&self->sw_rot->handles.copy_lock);
        mp_lcd_lock_init(&self->sw_rot->handles.tx_color_lock);
        mp_lcd_event_init(&self->sw_rot->handles.copy_task_exit);
        mp_lcd_event_init(&self->sw_rot->handles.swap_bufs);
        mp_lcd_event_set(&self->sw_rot->handles.swap_bufs);
        mp_lcd_lock_init(&self->sw_rot->handles.init_lock);
        mp_lcd_lock_acquire(&self->sw_rot->handles.init_lock);

        LCD_DEBUG_PRINT("h_res=%lu\n", self->panel_io_config->timings.h_res)
        LCD_DEBUG_PRINT("v_res=%lu\n", self->panel_io_config->timings.v_res)
        LCD_DEBUG_PRINT("bits_per_pixel=%d\n", self->panel_io_config->bits_per_pixel)
        LCD_DEBUG_PRINT("rgb565_byte_swap=%d\n", self->rgb565_byte_swap)

        xTaskCreatePinnedToCore(
                mp_lcd_sw_rotate_task, "rgb_task", DEFAULT_STACK_SIZE / sizeof(StackType_t),
                self, ESP_TASK_PRIO_MAX - 1, &self->sw_rot->handles.task_handle, 0);

        mp_lcd_lock_acquire(&self->sw_rot->handles.init_lock);
        mp_lcd_lock_release(&self->sw_rot->handles.init_lock);
        mp_lcd_lock_delete(&self->sw_rot->handles.init_lock);

        if (self->sw_rot->init.err != LCD_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, self->sw_rot->init.err_msg, self->sw_rot->init.err);
            return sself->sw_rot->init.err;
        } else {
            // add the new bus ONLY after successfull initilization of the bus
            rgb_bus_count++;
            rgb_bus_objs = m_realloc(rgb_bus_objs, rgb_bus_count * sizeof(mp_lcd_rgb_bus_obj_t *));
            rgb_bus_objs[rgb_bus_count - 1] = self;

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


    mp_lcd_err_t rgb_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size,
                              int x_start, int y_start, int x_end, int y_end, uint8_t rotation,
                              bool last_update, bool rgb565_dither)
    {
        LCD_DEBUG_PRINT("rgb_tx_color(self, lcd_cmd=%d, color, color_size=%d, x_start=%d, y_start=%d, x_end=%d, y_end=%d)\n", lcd_cmd, color_size, x_start, y_start, x_end, y_end)
        LCD_UNUSED(color_size);
        LCD_UNUSED(lcd_cmd);

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;
        
        rgb_bus_lock_acquire(&self->sw_rot->handles.tx_color_lock);

        self->sw_rot->data.rgb565_dither = (uint8_t)rgb565_dither;
        self->sw_rot->data.last_update = (uint8_t)last_update;
        self->sw_rot->buffers.partial = (uint8_t *)color;
        self->sw_rot->data.x_start = x_start;
        self->sw_rot->data.y_start = y_start;
        self->sw_rot->data.x_end = x_end;
        self->sw_rot->data.y_end = y_end;
        self->sw_rot->data.rotation = rotation;

        mp_lcd_lock_release(&self->sw_rot->handles.copy_lock);

        return LCD_OK;
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
