// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED

    // local includes
    #include "lcd_types.h"
    #include "modlcd_bus.h"
    #include "rgb_bus.h"

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
            ARG_rgb565_dither
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
            { MP_QSTR_rgb565_dither,      MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } },
        };

        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        // create new object
        mp_lcd_rgb_bus_obj_t *self = m_new_obj(mp_lcd_rgb_bus_obj_t);
        self->base.type = &mp_lcd_rgb_bus_type;

        self->callback = mp_const_none;

        self->rgb565_dither = (uint8_t)args[ARG_rgb565_dither].u_bool;

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
            rgb_bus_lock_acquire(&self->tx_color_lock, -1);
            self->partial_buf = NULL;
            rgb_bus_event_set(&self->copy_task_exit);
            rgb_bus_lock_release(&self->copy_lock);
            rgb_bus_lock_release(&self->tx_color_lock);

            mp_lcd_err_t ret = esp_lcd_panel_del(self->panel_handle);

            if (ret != 0) {
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_del)"), ret);
            }
            self->panel_handle = NULL;

            rgb_bus_lock_delete(&self->copy_lock);
            rgb_bus_lock_delete(&self->tx_color_lock);

            rgb_bus_event_clear(&self->swap_bufs);
            rgb_bus_event_delete(&self->swap_bufs);
            rgb_bus_event_delete(&self->copy_task_exit);

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


    mp_lcd_err_t rgb_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits)
    {
        LCD_UNUSED(cmd_bits);
        LCD_UNUSED(param_bits);

        LCD_DEBUG_PRINT("rgb_init(self, width=%i, height=%i, bpp=%d, buffer_size=%lu, rgb565_byte_swap=%d)\n", width, height, bpp, buffer_size, rgb565_byte_swap)

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        if (bpp != 16 && self->rgb565_dither) self->rgb565_dither = 0;

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

                self->rgb565_byte_swap = false;
            } else {
                self->rgb565_byte_swap = true;
            }
        } else {
            self->rgb565_byte_swap = false;
        }

        self->panel_io_config.timings.h_res = (uint32_t)width;
        self->panel_io_config.timings.v_res = (uint32_t)height;
        self->panel_io_config.bits_per_pixel = (size_t)bpp;

        self->width = width;
        self->height = height;
        self->bytes_per_pixel = bpp / 8;

        self->panel_io_config.flags.fb_in_psram = 1;
        self->panel_io_config.flags.double_fb = 1;

        rgb_bus_lock_init(&self->copy_lock);
        rgb_bus_lock_init(&self->tx_color_lock);
        rgb_bus_event_init(&self->copy_task_exit);
        rgb_bus_event_init(&self->swap_bufs);
        rgb_bus_event_set(&self->swap_bufs);
        rgb_bus_lock_init(&self->init_lock);
        rgb_bus_lock_acquire(&self->init_lock, -1);

        LCD_DEBUG_PRINT("h_res=%lu\n", self->panel_io_config.timings.h_res)
        LCD_DEBUG_PRINT("v_res=%lu\n", self->panel_io_config.timings.v_res)
        LCD_DEBUG_PRINT("bits_per_pixel=%d\n", self->panel_io_config.bits_per_pixel)
        LCD_DEBUG_PRINT("rgb565_byte_swap=%d\n", self->rgb565_byte_swap)

        xTaskCreatePinnedToCore(
                rgb_bus_copy_task, "rgb_task", DEFAULT_STACK_SIZE / sizeof(StackType_t),
                self, ESP_TASK_PRIO_MAX - 1, &self->copy_task_handle, 0);

        rgb_bus_lock_acquire(&self->init_lock, -1);
        rgb_bus_lock_release(&self->init_lock);
        rgb_bus_lock_delete(&self->init_lock);

        if (self->init_err != LCD_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, self->init_err_msg, self->init_err);
            return self->init_err;
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


    mp_lcd_err_t rgb_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update)
    {
        LCD_DEBUG_PRINT("rgb_tx_color(self, lcd_cmd=%d, color, color_size=%d, x_start=%d, y_start=%d, x_end=%d, y_end=%d)\n", lcd_cmd, color_size, x_start, y_start, x_end, y_end)
        LCD_UNUSED(color_size);

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;
        
        rgb_bus_lock_acquire(&self->tx_color_lock, -1);

        self->last_update = (uint8_t)last_update;
        self->partial_buf = (uint8_t *)color;
        self->x_start = x_start;
        self->y_start = y_start;
        self->x_end = x_end;
        self->y_end = y_end;
        self->rotation = rotation;

        rgb_bus_lock_release(&self->copy_lock);
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
    #include "../common_src/rgb_bus.c"

#endif /*SOC_LCD_RGB_SUPPORTED*/
