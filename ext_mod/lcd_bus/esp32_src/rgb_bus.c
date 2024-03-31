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

    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"
    #include "py/runtime.h"
    #include "py/objarray.h"
    #include "py/binary.h"

    // stdlib includes
    #include <string.h>


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
    } rgb_panel_t;

    bool rgb_bus_trans_done_cb(esp_lcd_panel_handle_t panel_io, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)user_ctx;

        if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
            cb_isr(self->callback);
        }
        self->trans_done = true;
        return false;
    }

    esp_lcd_rgb_panel_event_callbacks_t callbacks = { .on_vsync = rgb_bus_trans_done_cb };

    mp_lcd_err_t rgb_del(mp_obj_t obj);
    mp_lcd_err_t rgb_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap);
    mp_lcd_err_t rgb_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
    mp_lcd_err_t rgb_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t rgb_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t rgb_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end);
    mp_obj_t rgb_allocate_framebuffer(mp_obj_t obj, uint32_t size, uint32_t caps);
    mp_obj_t rgb_free_framebuffer(mp_obj_t obj, mp_obj_t buf);

    mp_obj_t mp_lcd_rgb_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        enum {
            ARG_hsync,
            ARG_vsync,
            ARG_de,
            ARG_disp,
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
            ARG_disp_active_low,
            ARG_refresh_on_demand,
        };

        const mp_arg_t allowed_args[] = {
            { MP_QSTR_hsync,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_vsync,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_de,                 MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_disp,               MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
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
            { MP_QSTR_disp_active_low,    MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } },
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
        // self->panel_io_config.bounce_buffer_size_px = (size_t)args[ARG_bounce_buffer_size_px].u_int;
        self->panel_io_config.hsync_gpio_num = (int)args[ARG_hsync].u_int;
        self->panel_io_config.vsync_gpio_num = (int)args[ARG_vsync].u_int;
        self->panel_io_config.de_gpio_num = (int)args[ARG_de].u_int;
        self->panel_io_config.pclk_gpio_num = (int)args[ARG_pclk].u_int;
        self->panel_io_config.data_gpio_nums[0] = args[ARG_data0].u_int;
        self->panel_io_config.data_gpio_nums[1] = args[ARG_data1].u_int;
        self->panel_io_config.data_gpio_nums[2] = args[ARG_data2].u_int;
        self->panel_io_config.data_gpio_nums[3] = args[ARG_data3].u_int;
        self->panel_io_config.data_gpio_nums[4] = args[ARG_data4].u_int;
        self->panel_io_config.data_gpio_nums[5] = args[ARG_data5].u_int;
        self->panel_io_config.data_gpio_nums[6] = args[ARG_data6].u_int;
        self->panel_io_config.data_gpio_nums[7] = args[ARG_data7].u_int;
        self->panel_io_config.data_gpio_nums[8] = args[ARG_data8].u_int;
        self->panel_io_config.data_gpio_nums[9] = args[ARG_data9].u_int;
        self->panel_io_config.data_gpio_nums[10] = args[ARG_data10].u_int;
        self->panel_io_config.data_gpio_nums[11] = args[ARG_data11].u_int;
        self->panel_io_config.data_gpio_nums[12] = args[ARG_data12].u_int;
        self->panel_io_config.data_gpio_nums[13] = args[ARG_data13].u_int;
        self->panel_io_config.data_gpio_nums[14] = args[ARG_data14].u_int;
        self->panel_io_config.data_gpio_nums[15] = args[ARG_data15].u_int;
        self->panel_io_config.disp_gpio_num = (int)args[ARG_disp].u_int;
        self->panel_io_config.sram_trans_align = 64;
        self->panel_io_config.psram_trans_align = 64;
        self->panel_io_config.flags.disp_active_low = (uint32_t)args[ARG_disp_active_low].u_bool;
        self->panel_io_config.flags.refresh_on_demand = (uint32_t)args[ARG_refresh_on_demand].u_bool;
        self->panel_io_config.flags.fb_in_psram = 0;
        self->panel_io_config.flags.double_fb = 0;
        // self->panel_io_config.flags.no_fb = true;
        // self->panel_io_config.flags.bb_invalidate_cache = (uint32_t)args[ARG_bb_invalidate_cache].u_bool;

        int i = 0;
        for (; i < 16; i++) {
            if (self->panel_io_config.data_gpio_nums[i] == -1) {
                break;
            }
        }

        self->panel_io_config.data_width = (size_t) i;

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

        mp_lcd_err_t ret = esp_lcd_panel_del(self->panel_handle);
        return ret;
    }

    mp_lcd_err_t rgb_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        LCD_UNUSED(obj);
        LCD_UNUSED(lcd_cmd);
        LCD_UNUSED(param);
        LCD_UNUSED(param_size);
        return LCD_OK;
    }

    mp_lcd_err_t rgb_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        LCD_UNUSED(obj);
        LCD_UNUSED(lcd_cmd);
        LCD_UNUSED(param);
        LCD_UNUSED(param_size);
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
        } else if (array_buf == self->view2) {
            heap_caps_free(item_buf);
            self->view2 = NULL;
        } else {
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("No matching buffer found"));
        }
        return mp_const_none;
    }

    mp_obj_t rgb_allocate_framebuffer(mp_obj_t obj, uint32_t size, uint32_t caps)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        if (self->view1 == NULL) {
            if ((caps | MALLOC_CAP_SPIRAM) == caps) {
                uint32_t available = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                if (available < size) {
                    mp_raise_msg_varg(
                        &mp_type_MemoryError,
                        MP_ERROR_TEXT("Not enough memory available in SPIRAM (%d)"),
                        size
                    );
                    return mp_const_none;
                }
                self->panel_io_config.flags.fb_in_psram = 1;
            } else {
                uint32_t available = (uint32_t)heap_caps_get_largest_free_block(caps);
                if (available < size) {
                    mp_raise_msg_varg(
                        &mp_type_MemoryError,
                        MP_ERROR_TEXT("Not enough memory available (%d)"),
                        size
                    );
                    return mp_const_none;
                }
            }
            self->buffer_size = size;
            void *buf = heap_caps_calloc(1, 1, MALLOC_CAP_INTERNAL);

            mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, 1, buf));
            view->typecode |= 0x80; // used to indicate writable buffer
            self->view1 = view;
            return MP_OBJ_FROM_PTR(view);
        } else if (self->view2 == NULL) {
            if (self->buffer_size != size) {
                mp_raise_msg_varg(
                    &mp_type_MemoryError,
                    MP_ERROR_TEXT("Frame buffer sizes do not match (%d)"),
                    size
                );
                return mp_const_none;
            }

            if (self->panel_io_config.flags.fb_in_psram == 1) {
                uint32_t available = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
                if (available < size) {
                    mp_raise_msg_varg(
                        &mp_type_MemoryError,
                        MP_ERROR_TEXT("Not enough memory available in SPIRAM (%d)"),
                        size
                    );
                    return mp_const_none;
                }
            } else {
                uint32_t available = (uint32_t)heap_caps_get_largest_free_block(caps);
                if (available < size) {
                    mp_raise_msg_varg(
                        &mp_type_MemoryError,
                        MP_ERROR_TEXT("Not enough memory available (%d)"),
                        size
                    );
                    return mp_const_none;
                }
            }

            self->panel_io_config.flags.double_fb = 1;
            void *buf = heap_caps_calloc(1, 1, MALLOC_CAP_INTERNAL);
            mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, 1, buf));
            view->typecode |= 0x80; // used to indicate writable buffer
            self->view2 = view;
            return MP_OBJ_FROM_PTR(view);
        } else {
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("There is a maximum of 2 frame buffers allowed"));
            return mp_const_none;
        }
    }

    mp_lcd_err_t rgb_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        if (buffer_size != self->buffer_size) {
            mp_raise_msg_varg(
                &mp_type_ValueError,
                MP_ERROR_TEXT("Frame buffer size is not correct for the display size (%d)"),
                buffer_size
            );
        }

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

        mp_lcd_err_t ret = esp_lcd_new_rgb_panel(&self->panel_io_config, &self->panel_handle);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_rgb_panel)"), ret);
            return ret;
        }

        if (self->panel_io_config.flags.double_fb) {
            ret = esp_lcd_rgb_panel_register_event_callbacks(self->panel_handle, &callbacks, self);
            if (ret != 0) {
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_rgb_panel_register_event_callbacks)"), ret);
                return ret;
            }
        }

        ret = esp_lcd_panel_reset(self->panel_handle);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_reset)"), ret);
            return ret;
        }

        ret = esp_lcd_panel_init(self->panel_handle);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_init)"), ret);
            return ret;
        }

        rgb_panel_t *rgb_panel = __containerof((esp_lcd_panel_t *)self->panel_handle, rgb_panel_t, base);

        void *buf1 = self->view1->items;
        self->view1->items = (void *)rgb_panel->fbs[0];
        self->view1->len = buffer_size;
        heap_caps_free(buf1);

        if (self->panel_io_config.flags.double_fb) {
            void *buf2 = self->view2->items;
            self->view2->items = (void *)rgb_panel->fbs[1];
            self->view2->len = buffer_size;
            heap_caps_free(buf2);
        }

        return LCD_OK;
    }


    mp_lcd_err_t rgb_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;
        *lane_count = (uint8_t)self->panel_io_config.data_width;
        return LCD_OK;
    }

    mp_lcd_err_t rgb_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        esp_err_t ret = esp_lcd_panel_draw_bitmap(
            self->panel_handle,
            x_start,
            y_start,
            x_end,
            y_end,
            color
        );

        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_draw_bitmap)"), ret);
            return LCD_OK;
        }

        /*
        if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
            mp_call_function_n_kw(self->callback, 0, 0, NULL);
        }
        */

        if (!self->panel_io_config.flags.double_fb) {
            if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
                mp_call_function_n_kw(self->callback, 0, 0, NULL);
            }
        } else if (self->callback == mp_const_none) {
            while (!self->trans_done) {}
            self->trans_done = false;
        }

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
