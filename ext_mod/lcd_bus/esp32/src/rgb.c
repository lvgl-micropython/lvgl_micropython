// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED

    // local includes
    #include "lcd_types.h"
    #include "lcd_bus.h"
    #include "../inc/rgb.h"

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

    // stdlib includes
    #include <string.h>

    #define DEFAULT_STACK_SIZE    (5 * 1024)

    mp_lcd_err_t rgb_del(mp_obj_t obj);
    mp_lcd_err_t rgb_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
    mp_lcd_err_t rgb_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t rgb_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);

    mp_lcd_err_t rgb_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp,
                          uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits,
                          uint8_t param_bits, bool sw_rotate);

    mp_lcd_err_t rgb_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size,
                              int x_start, int y_start, int x_end, int y_end,
                              uint8_t rotation, bool last_update, bool dither);


    static bool rgb_bus_trans_done_cb(esp_lcd_panel_handle_t panel,
                                const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
    {
        LCD_UNUSED(edata);
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)user_ctx;
        rgb_panel_t *rgb_panel = __containerof(panel, rgb_panel_t, base);
        uint8_t *curr_buf = rgb_panel->fbs[rgb_panel->cur_fb_index];

        if (curr_buf != self->buffers.active_fb && !lcd_event_isset_from_isr(&self->buffers.swap_bufs)) {
            uint8_t *idle_fb = self->buffers.idle_fb;
            self->buffers.idle_fb = self->buffers.active_fb;
            self->buffers.active_fb = idle_fb;
            lcd_event_set_from_isr(&self->buffers.swap_bufs);
        }

        return false;
    }

    mp_obj_t mp_lcd_rgb_bus_make_new(const mp_obj_type_t *type, size_t n_args,
                                     size_t n_kw, const mp_obj_t *all_args)
    {
        enum { ARG_hsync, ARG_vsync, ARG_de, ARG_pclk, ARG_data0, ARG_data1, ARG_data2,
               ARG_data3, ARG_data4, ARG_data5, ARG_data6, ARG_data7, ARG_data8, ARG_data9,
               ARG_data10, ARG_data11, ARG_data12, ARG_data13, ARG_data14, ARG_data15, ARG_freq,
               ARG_hsync_front_porch, ARG_hsync_back_porch, ARG_hsync_pulse_width, ARG_hsync_idle_low,
               ARG_vsync_front_porch, ARG_vsync_back_porch, ARG_vsync_pulse_width, ARG_vsync_idle_low,
               ARG_de_idle_high, ARG_pclk_idle_high, ARG_pclk_active_low };

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
        };

        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        // create new object
        mp_lcd_rgb_bus_obj_t *self = m_new_obj(mp_lcd_rgb_bus_obj_t);
        self->base.type = &mp_lcd_rgb_bus_type;

        self->callback = mp_const_none;

        // TODO: change self->panel_io_config to pointer and add allocation
        // TODO: change self->bus_config to pointer and add allocation

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
        self->panel_io_config.dma_burst_size = 64;
        self->panel_io_config.flags.refresh_on_demand = 0;
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
        self->panel_io_handle.init = &rgb_init;

        return MP_OBJ_FROM_PTR(self);
    }

    mp_lcd_err_t rgb_del(mp_obj_t obj)
    {
        LCD_DEBUG_PRINT("rgb_del(self)\n")

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;       

        if (self->panel_handle != NULL) {
            // TODO: add proper FreeRTOS task shutdown
            lcd_lock_acquire(&self->task.tx_color_lock);
            self->buffers.partial_buf = NULL;
            lcd_event_set(&self->task.copy_task_exit);
            lcd_lock_release(&self->buffers.copy_lock);
            lcd_lock_release(&self->task.tx_color_lock);

            mp_lcd_err_t ret = esp_lcd_panel_del(self->panel_handle);

            if (ret != 0) {
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_del)"), ret);
            }
            self->panel_handle = NULL;

            lcd_lock_delete(&self->buffers.copy_lock);
            lcd_lock_delete(&self->task.tx_color_lock);

            lcd_event_clear(&self->buffers.swap_bufs);
            lcd_event_delete(&self->buffers.swap_bufs);
            lcd_event_delete(&self->task.copy_task_exit);

            if (self->view1 != NULL) {
                heap_caps_free(self->view1->items);
                self->view1->items = NULL;
                self->view1->len = 0;
                self->view1 = NULL;
                LCD_DEBUG_PRINT("rgb_free_framebuffer(self, buf=1)\n")
            }

            if (self->view2 != NULL) {
                heap_caps_free(self->view2->items);
                self->view2->items = NULL;
                self->view2->len = 0;
                self->view2 = NULL;
                LCD_DEBUG_PRINT("rgb_free_framebuffer(self, buf=1)\n")
            }
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

        LCD_DEBUG_PRINT("rgb_init(self, width=%i, height=%i, bpp=%d, buffer_size=%lu, rgb565_byte_swap=%d)\n",
                        width, height, bpp, buffer_size, rgb565_byte_swap)

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

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

                self->rot_data.byteswap = false;
            } else {
                self->rot_data.byteswap = true;
            }
        } else {
            self->rot_data.byteswap = false;
        }

        // TODO: alloc self->task_init

        self->panel_io_config.timings.h_res = (uint32_t)width;
        self->panel_io_config.timings.v_res = (uint32_t)height;
        self->panel_io_config.bits_per_pixel = (size_t)bpp;

        self->rot_data.dst_width = width;
        self->rot_data.dst_height = height;
        self->rot_data.bytes_per_pixel = bpp / 8;

        self->panel_io_config.flags.fb_in_psram = 1;
        self->panel_io_config.flags.double_fb = 1;

        lcd_lock_init(&self->buffers.copy_lock);
        lcd_lock_init(&self->task.tx_color_lock);
        lcd_event_init(&self->task.copy_task_exit);
        lcd_event_init(&self->buffers.swap_bufs);
        lcd_event_set(&self->buffers.swap_bufs);
        lcd_lock_init(&self->task_init->init_lock);
        lcd_lock_acquire(&self->task_init->init_lock);

        LCD_DEBUG_PRINT("h_res=%lu\n", self->panel_io_config.timings.h_res)
        LCD_DEBUG_PRINT("v_res=%lu\n", self->panel_io_config.timings.v_res)
        LCD_DEBUG_PRINT("bits_per_pixel=%d\n", self->panel_io_config.bits_per_pixel)

        xTaskCreatePinnedToCore(
                rgb_copy_task, "rgb_task", DEFAULT_STACK_SIZE / sizeof(StackType_t),
                self, ESP_TASK_PRIO_MAX - 1, &self->task_handle, 0);

        lcd_lock_acquire(&self->task_init->init_lock);
        lcd_lock_release(&self->task_init->init_lock);
        lcd_lock_delete(&self->task_init->init_lock);

        if (self->task_init->init_err != LCD_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, self->task_init->init_err_msg,
                              self->task_init->init_err);

            return self->task_init->init_err;
        } else {
            // TODO: free self->task_init
            // TODO: free self->panel_io_config
            // TODO: free self->bus_config
            return LCD_OK;
        }
    }


    mp_lcd_err_t rgb_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;
        *lane_count = (uint8_t)self->panel_io_config.data_width;

        LCD_DEBUG_PRINT("rgb_get_lane_count(self)-> %d\n",
                        (uint8_t)self->panel_io_config.data_width)

        return LCD_OK;
    }


    mp_lcd_err_t rgb_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size,
                              int x_start, int y_start, int x_end, int y_end, uint8_t rotation,
                              bool last_update, bool dither)
    {
        LCD_DEBUG_PRINT("rgb_tx_color(self, lcd_cmd=%d, color, color_size=%d, x_start=%d, y_start=%d, x_end=%d, y_end=%d)\n",
                        lcd_cmd, color_size, x_start, y_start, x_end, y_end)

        LCD_UNUSED(color_size);

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        lcd_lock_acquire(&self->task.tx_color_lock, -1);

        self->rot_data.last_update = (uint8_t)last_update;
        self->rot_data.partial_buf = (uint8_t *)color;
        self->rot_data.x_start = x_start;
        self->rot_data.y_start = y_start;
        self->rot_data.x_end = x_end;
        self->rot_data.y_end = y_end;
        self->rot_data.rotation = rotation;
        self->rot_data.dither = (uint8_t)dither;

        lcd_lock_release(&self->buffers.copy_lock);
//        if (self->callback != mp_const_none) {
//            mp_call_function_n_kw(self->callback, 0, 0, NULL);
//        }

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
    #include "../../unsupported/src/rgb_bus.c"

#endif /*SOC_LCD_RGB_SUPPORTED*/
