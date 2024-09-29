#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED

    // local includes
    #include "lcd_types.h"
    #include "modlcd_bus.h"
    #include "rgb_bus.h"
    #include "esp_rgb_bus.h"


    // esp-idf includes
    #include "hal/lcd_hal.h"
    #include "esp_pm.h"
    #include "esp_intr_alloc.h"
    #include "esp_heap_caps.h"

    #include "esp_lcd_panel_io.h"
    #include "esp_lcd_panel_ops.h"
    #include "esp_lcd_panel_interface.h"

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"
    #include "freertos/idf_additions.h"

    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"
    #include "py/runtime.h"
    #include "py/objarray.h"
    #include "py/binary.h"

    // stdlib includes
    #include <string.h>

    typedef enum {
        ROTATION_0 = 0,
        ROTATION_90 = 1,
        ROTATION_180 = 2,
        ROTATION_270 = 3
    } esp_lcd_rgb_rotation_t;


    static bool rgb_bus_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
    {
        LCD_UNUSED(edata);

        lcd_panel_io_rgb_t *io = __containerof(panel_io, lcd_panel_io_rgb_t, base);
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)user_ctx;

        if (io->bus->swapped_framebuf && self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
            if (self->has_partial_buffer) {
                xSemaphoreGiveFromISR(self->taskCopySeph, (BaseType_t * const)pdTRUE);
            } else {
                cb_isr(self->callback);
            }

            io->bus->swapped_framebuf = false;
            self->trans_done = true;
            return true;
        }
        return false;
    }


    void copy_task(void *self_in)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)self_in;
        esp_lcd_rgb_bus_t *rgb_bus = (esp_lcd_rgb_bus_t *)self->bus_handle;

        uint32_t buf_size = rgb_bus->v_res * rgb_bus->h_res * (rgb_bus->fb_bits_per_pixel / 8);

        uint8_t *fb0 = rgb_bus->fbs[0];
        uint8_t *fb1 = rgb_bus->fbs[1];

        while(1) {
            xSemaphoreTake(self->taskCopySeph, portMAX_DELAY);
            if (rgb_bus->cur_fb_index == 0) {
                memcpy(fb1, fb0, buf_size);
            } else {
                memcpy(fb0, fb1, buf_size);
            }
            xSemaphoreGive(self->flushCopySeph);
        }
    }


    mp_lcd_err_t rgb_del(mp_obj_t obj);
    mp_lcd_err_t rgb_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
    mp_lcd_err_t rgb_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
    mp_lcd_err_t rgb_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t rgb_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t rgb_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, int rotation, bool last_flush);
    mp_obj_t rgb_allocate_framebuffer(mp_obj_t obj, uint32_t size, uint32_t caps);
    mp_obj_t rgb_free_framebuffer(mp_obj_t obj, mp_obj_t buf);

    mp_obj_t mp_lcd_rgb_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        enum {
            ARG_hsync,
            ARG_vsync,
            ARG_de,
            ARG_pclk,
            ARG_data_pins,
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
        };

        const mp_arg_t allowed_args[] = {
            { MP_QSTR_hsync,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_vsync,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_de,                 MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_pclk,               MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data_pins,          MP_ARG_OBJ  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
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

        self->bus_config = (esp_lcd_rgb_bus_config_t *)heap_caps_calloc(1, sizeof(esp_lcd_rgb_bus_config_t), MALLOC_CAP_INTERNAL);
        self->bus_config->clk_src = LCD_CLK_SRC_PLL160M;
        self->bus_config->hsync_gpio_num = (int)args[ARG_hsync].u_int;
        self->bus_config->vsync_gpio_num = (int)args[ARG_vsync].u_int;
        self->bus_config->de_gpio_num = (int)args[ARG_de].u_int;
        self->bus_config->pclk_gpio_num = (int)args[ARG_pclk].u_int;

        mp_obj_tuple_t *data_pins = MP_OBJ_TO_PTR(args[ARG_data_pins].u_obj);
        self->bus_config->data_width = (size_t)data_pins->len;
        self->num_lanes = self->bus_config->data_width;

        for (uint8_t i = 0; i < data_pins->len; i++) self->bus_config->data_gpio_nums[i] = (int)mp_obj_get_int(data_pins->items[i]);
        for (uint8_t i = data_pins->len; i < 16; i++) self->bus_config->data_gpio_nums[i] = -1;

        self->bus_config->sram_trans_align = 8;
        self->bus_config->psram_trans_align = 64;
        self->bus_config->flags.fb_in_psram = 0;
        self->bus_config->flags.double_fb = 0;
        self->bus_config->flags.pclk_active_neg = (uint32_t)args[ARG_pclk_active_low].u_bool;
        self->bus_config->flags.pclk_idle_high = (uint32_t)args[ARG_pclk_idle_high].u_bool;

        self->panel_io_config = (esp_lcd_panel_io_rgb_config_t *)heap_caps_calloc(1, sizeof(esp_lcd_panel_io_rgb_config_t), MALLOC_CAP_INTERNAL);
        self->panel_io_config->pclk_hz = (uint32_t)args[ARG_freq].u_int;
        self->panel_io_config->hsync_pulse_width = (uint32_t)args[ARG_hsync_pulse_width].u_int;
        self->panel_io_config->hsync_back_porch = (uint32_t)args[ARG_hsync_back_porch].u_int;
        self->panel_io_config->hsync_front_porch = (uint32_t)args[ARG_hsync_front_porch].u_int;
        self->panel_io_config->vsync_pulse_width = (uint32_t)args[ARG_vsync_pulse_width].u_int;
        self->panel_io_config->vsync_back_porch = (uint32_t)args[ARG_vsync_back_porch].u_int;
        self->panel_io_config->vsync_front_porch = (uint32_t)args[ARG_vsync_front_porch].u_int;
        self->panel_io_config->flags.hsync_idle_low = (uint32_t)args[ARG_hsync_idle_low].u_bool;
        self->panel_io_config->flags.vsync_idle_low = (uint32_t)args[ARG_vsync_idle_low].u_bool;
        self->panel_io_config->flags.de_idle_high = (uint32_t)args[ARG_de_idle_high].u_bool;


    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("pclk_hz=%lu\n", self->panel_io_config->pclk_hz);
        printf("hsync_pulse_width=%lu\n", self->panel_io_config->hsync_pulse_width);
        printf("hsync_back_porch=%lu\n", self->panel_io_config->hsync_back_porch);
        printf("hsync_front_porch=%lu\n", self->panel_io_config->hsync_front_porch);
        printf("vsync_pulse_width=%lu\n", self->panel_io_config->vsync_pulse_width);
        printf("vsync_back_porch=%lu\n", self->panel_io_config->vsync_back_porch);
        printf("vsync_front_porch=%lu\n", self->panel_io_config->vsync_front_porch);
        printf("hsync_idle_low=%d\n", self->panel_io_config->flags.hsync_idle_low);
        printf("vsync_idle_low=%d\n", self->panel_io_config->flags.vsync_idle_low);
        printf("de_idle_high=%d\n", self->panel_io_config->flags.de_idle_high);

        printf("pclk_active_neg=%d\n", self->bus_config->flags.pclk_active_neg);
        printf("pclk_idle_high=%d\n", self->bus_config->flags.pclk_idle_high);
        printf("clk_src=%d\n", self->bus_config->clk_src);
        printf("hsync_gpio_num=%d\n", self->bus_config->hsync_gpio_num);
        printf("vsync_gpio_num=%d\n", self->bus_config->vsync_gpio_num);
        printf("de_gpio_num=%d\n", self->bus_config->de_gpio_num);
        printf("pclk_gpio_num=%d\n", self->bus_config->pclk_gpio_num);
        printf("data_gpio_nums[0]=%d\n", self->bus_config->data_gpio_nums[0]);
        printf("data_gpio_nums[1]=%d\n", self->bus_config->data_gpio_nums[1]);
        printf("data_gpio_nums[2]=%d\n", self->bus_config->data_gpio_nums[2]);
        printf("data_gpio_nums[3]=%d\n", self->bus_config->data_gpio_nums[3]);
        printf("data_gpio_nums[4]=%d\n", self->bus_config->data_gpio_nums[4]);
        printf("data_gpio_nums[5]=%d\n", self->bus_config->data_gpio_nums[5]);
        printf("data_gpio_nums[6]=%d\n", self->bus_config->data_gpio_nums[6]);
        printf("data_gpio_nums[7]=%d\n", self->bus_config->data_gpio_nums[7]);
        printf("data_gpio_nums[8]=%d\n", self->bus_config->data_gpio_nums[8]);
        printf("data_gpio_nums[9]=%d\n", self->bus_config->data_gpio_nums[9]);
        printf("data_gpio_nums[10]=%d\n", self->bus_config->data_gpio_nums[10]);
        printf("data_gpio_nums[11]=%d\n", self->bus_config->data_gpio_nums[11]);
        printf("data_gpio_nums[12]=%d\n", self->bus_config->data_gpio_nums[12]);
        printf("data_gpio_nums[13]=%d\n", self->bus_config->data_gpio_nums[13]);
        printf("data_gpio_nums[14]=%d\n", self->bus_config->data_gpio_nums[14]);
        printf("data_gpio_nums[15]=%d\n", self->bus_config->data_gpio_nums[15]);
        printf("sram_trans_align=%d\n", self->bus_config->sram_trans_align);
        printf("psram_trans_align=%d\n", self->bus_config->psram_trans_align);
        printf("disp_active_low=%d\n", self->bus_config->flags.disp_active_low);
        printf("fb_in_psram=%d\n", self->bus_config->flags.fb_in_psram);
        printf("double_fb=%d\n", self->bus_config->flags.double_fb);
        printf("data_width=%d\n", self->bus_config->data_width);

    #endif

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

    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("rgb_del(self)\n");
    #endif

        mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle.panel_io);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
        }

        ret = esp_lcd_del_rgb_bus(self->bus_handle);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_del_rgb_bus)"), ret);
        }

        if (self->has_partial_buffer) vTaskDelete(self->copyTask);

        return ret;

    }

    mp_lcd_err_t rgb_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        LCD_UNUSED(obj);
        LCD_UNUSED(param);

    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("rgb_rx_param(self, lcd_cmd=%d, param, param_size=%d)\n", lcd_cmd, param_size);
    #else
        LCD_UNUSED(lcd_cmd);
        LCD_UNUSED(param_size);
    #endif

        return LCD_OK;
    }

    mp_lcd_err_t rgb_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        LCD_UNUSED(obj);
        LCD_UNUSED(param);

    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("rgb_tx_param(self, lcd_cmd=%d, param, param_size=%d)\n", lcd_cmd, param_size);
    #else
        LCD_UNUSED(lcd_cmd);
        LCD_UNUSED(param_size);
    #endif

        return LCD_OK;
    }

    mp_obj_t rgb_free_framebuffer(mp_obj_t obj, mp_obj_t buf)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        /*
        if (self->panel_io_handle != NULL) {
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Unable to free buffer"));
            return mp_const_none;
        }
        */

        mp_obj_array_t *array_buf = (mp_obj_array_t *)MP_OBJ_TO_PTR(buf);
        void *item_buf = array_buf->items;

        if (array_buf == self->view1) {
            heap_caps_free(item_buf);
            self->view1 = NULL;
        #if CONFIG_LCD_ENABLE_DEBUG_LOG
            printf("rgb_free_framebuffer(self, buf=1)\n");
        #endif
        } else if (array_buf == self->view2) {
            heap_caps_free(item_buf);
            self->view2 = NULL;
        #if CONFIG_LCD_ENABLE_DEBUG_LOG
            printf("rgb_free_framebuffer(self, buf=2)\n");
        #endif
        } else {
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("No matching buffer found"));
        }
        return mp_const_none;
    }

    mp_obj_t rgb_allocate_framebuffer(mp_obj_t obj, uint32_t size, uint32_t caps)
    {
    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("rgb_allocate_framebuffer(self, size=%lu, caps=%lu)\n", size, caps);
    #endif

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        void *buf = heap_caps_calloc(1, 1, MALLOC_CAP_INTERNAL);
        mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, 1, buf));
        view->typecode |= 0x80; // used to indicate writable buffer

        uint32_t available = (uint32_t)heap_caps_get_largest_free_block(caps);
        if (available < size) {
            heap_caps_free(buf);
            mp_raise_msg_varg(
                &mp_type_MemoryError,
                MP_ERROR_TEXT("Not enough memory available in SPIRAM (%d)"),
                size
            );
            return mp_const_none;
        }
        if ((caps | MALLOC_CAP_SPIRAM) == caps) {
            self->bus_config->flags.fb_in_psram = 1;
        }

        if (self->view1 == NULL) {
            self->buffer_size = size;
            self->view1 = view;
        } else if (self->buffer_size != size) {
            heap_caps_free(buf);
            mp_raise_msg_varg(
                &mp_type_MemoryError,
                MP_ERROR_TEXT("Frame buffer sizes do not match (%d)"),
                size
            );
            return mp_const_none;
        } else if (self->view2 == NULL) {
            self->view2 = view;
            self->bus_config->flags.double_fb = 1;
        } else {
            heap_caps_free(buf);
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("There is a maximum of 2 frame buffers allowed"));
            return mp_const_none;
        }

        return MP_OBJ_FROM_PTR(view);
    }

    mp_lcd_err_t rgb_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits)
    {
        LCD_UNUSED(cmd_bits);
        LCD_UNUSED(param_bits);
    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("rgb_init(self, width=%i, height=%i, bpp=%d, buffer_size=%lu, rgb565_byte_swap=%d)\n", width, height, bpp, buffer_size, rgb565_byte_swap);
    #endif
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;


        if (buffer_size != self->buffer_size) {
            if (self->view2 != NULL)  {
                mp_raise_msg_varg(
                    &mp_type_ValueError,
                    MP_ERROR_TEXT("you cannot use double buffering with a partial sized frame buffer (%d)"),
                    self->buffer_size
                );
            } else {
                void *buf = heap_caps_calloc(1, self->buffer_size, MALLOC_CAP_INTERNAL);
                if (buf == NULL) {
                    buf = heap_caps_calloc(1, self->buffer_size, MALLOC_CAP_SPIRAM);
                    if (buf == NULL) {
                        mp_raise_msg_varg(
                            &mp_type_MemoryError,
                            MP_ERROR_TEXT("Not enough memory available to allocate partial buffer (%d)"),
                            self->buffer_size
                        );
                    }
                }
                void *tmp_buf = self->view1->items;
                self->view1->items = buf;
                self->view1->len = self->buffer_size;
                heap_caps_free(tmp_buf);
                self->bus_config->flags.double_fb = 1;

                self->has_partial_buffer = true;
                self->taskCopySeph = xSemaphoreCreateBinary();
                self->flushCopySeph = xSemaphoreCreateBinary();
            }
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

            if (self->bus_config->data_width == 16) {
                int temp_pin;

                for (uint8_t i = 0; i < 8; i++) {
                    temp_pin = self->bus_config->data_gpio_nums[i];
                    self->bus_config->data_gpio_nums[i] = self->bus_config->data_gpio_nums[i + 8];
                    self->bus_config->data_gpio_nums[i + 8] = temp_pin;
                }
                self->rgb565_byte_swap = false;
            } else {
                self->rgb565_byte_swap = true;
            }
        } else {
            self->rgb565_byte_swap = false;
        }

        self->bus_config->h_res = (uint32_t)width;
        self->bus_config->v_res = (uint32_t)height;
        self->bus_config->bits_per_pixel = (size_t)bpp;

    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("h_res=%lu\n", self->bus_config->h_res);
        printf("v_res=%lu\n", self->bus_config->v_res);
        printf("bits_per_pixel=%d\n", self->bus_config->bits_per_pixel);
        printf("rgb565_byte_swap=%d\n", self->rgb565_byte_swap);
    #endif

        mp_lcd_err_t ret = esp_lcd_new_rgb_bus(self->bus_config, &self->bus_handle);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_rgb_bus)"), ret);
            return ret;
        }

        ret = esp_lcd_new_panel_io_rgb(self->bus_handle, self->panel_io_config, &self->panel_io_handle.panel_io);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_rgb)"), ret);
            return ret;
        }

        esp_lcd_panel_io_callbacks_t callbacks = {
            .on_color_trans_done = rgb_bus_trans_done_cb
        };

        ret = esp_lcd_panel_io_register_event_callbacks(self->panel_io_handle.panel_io, &callbacks, self);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_rgb_panel_register_event_callbacks)"), ret);
            return ret;
        }

        if (self->has_partial_buffer) {
            xTaskCreatePinnedToCore(copy_task, "rgb_copy", 8192, self, 1, &self->copyTask, 1);
        } else {
            esp_lcd_rgb_bus_t *rgb_bus = (esp_lcd_rgb_bus_t *)self->bus_handle;

            void *buf1 = self->view1->items;
            self->view1->items = (void *)rgb_bus->fbs[0];
            self->view1->len = buffer_size;
            heap_caps_free(buf1);

            if (self->bus_config->flags.double_fb) {
                void *buf2 = self->view2->items;
                self->view2->items = (void *)rgb_bus->fbs[1];
                self->view2->len = buffer_size;
                heap_caps_free(buf2);
            }
        }

        heap_caps_free(self->bus_config);
        self->bus_config = NULL;

        heap_caps_free(self->panel_io_config);
        self->panel_io_config = NULL;

        return LCD_OK;
    }


    mp_lcd_err_t rgb_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;
        *lane_count = self->num_lanes;
    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("rgb_get_lane_count(self)-> %d\n", self->num_lanes);
    #endif

        return LCD_OK;
    }

    __attribute__((always_inline)) static inline void cpy_pixel_8bpp(uint8_t *to, const uint8_t *from)
    {
        *to++ = *from++;
    }

    __attribute__((always_inline)) static inline void cpy_pixel_16bpp(uint8_t *to, const uint8_t *from)
    {
        *to++ = *from++;
        *to++ = *from++;
    }

    __attribute__((always_inline)) static inline void cpy_pixel_24bpp(uint8_t *to, const uint8_t *from)
    {
        *to++ = *from++;
        *to++ = *from++;
        *to++ = *from++;
    }

    typedef void (*cpy_pixel)(uint8_t *to, const uint8_t *from);

    mp_lcd_err_t rgb_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, int rotation, bool last_flush)
    {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;
        esp_lcd_rgb_bus_t *rgb_bus = (esp_lcd_rgb_bus_t *)self->bus_handle;
        esp_err_t ret = LCD_OK;

        if (color == rgb_bus->fbs[0] || color == rgb_bus->fbs[1]) {
            LCD_UNUSED(x_start);
            LCD_UNUSED(y_start);
            LCD_UNUSED(x_end);
            LCD_UNUSED(y_end);
            LCD_UNUSED(rotation);
            LCD_UNUSED(last_flush);

            ret = esp_lcd_panel_io_tx_color(self->panel_io_handle.panel_io, lcd_cmd, color, color_size);
        } else {
            xSemaphoreTake(self->flushCopySeph, portMAX_DELAY);
            uint8_t *fb;

            if (rgb_bus->cur_fb_index == 0) {
                fb = rgb_bus->fbs[1];
            } else {
                fb = rgb_bus->fbs[0];
            }

            int bytes_per_pixel = rgb_bus->fb_bits_per_pixel / 8;
            int pixels_per_line = rgb_bus->h_res;

            // round the boundary
            int h_res = rgb_bus->h_res;
            int v_res = rgb_bus->v_res;

            if (rotation == ROTATION_270) {
                x_start = MIN(x_start, v_res);
                x_end = MIN(x_end, v_res);
                y_start = MIN(y_start, h_res);
                y_end = MIN(y_end, h_res);
            } else {
                x_start = MIN(x_start, h_res);
                x_end = MIN(x_end, h_res);
                y_start = MIN(y_start, v_res);
                y_end = MIN(y_end, v_res);
            }

            uint32_t bytes_per_line = bytes_per_pixel * pixels_per_line;
            size_t bytes_to_flush = v_res * h_res * bytes_per_pixel;
            uint8_t *flush_ptr = fb;

            const uint8_t *from = (const uint8_t *)color;
            uint32_t copy_bytes_per_line = (x_end - x_start) * bytes_per_pixel;
            size_t offset = y_start * copy_bytes_per_line + x_start * bytes_per_pixel;
            uint8_t *to = fb;

            cpy_pixel cpy_func;
            if (1 == bytes_per_pixel) {
                cpy_func = cpy_pixel_8bpp;
            } else if (2 == bytes_per_pixel) {
                cpy_func = cpy_pixel_16bpp;
            } else {
                cpy_func = cpy_pixel_24bpp;
            }

            switch (rotation) {
            case ROTATION_90:
                for (int y = y_start; y < y_end; y++) {
                    for (int x = x_start; x < x_end; x++) {
                        uint32_t j = y * copy_bytes_per_line + x * bytes_per_pixel - offset;
                        uint32_t i = (x * h_res + y) * bytes_per_pixel;
                        cpy_func(to + i, from + j);
                    }
                }
                bytes_to_flush = (x_end - x_start) * bytes_per_line;
                flush_ptr = fb + x_start * bytes_per_line;
                break;

            case ROTATION_180:
                LCD_UNUSED(offset);

                for (int y = y_start; y < y_end; y++) {
                    uint32_t index = ((v_res - 1 - y) * h_res + (h_res - 1 - x_start)) * bytes_per_pixel;
                    for (size_t x = x_start; x < x_end; x++) {
                        cpy_func(to + index, from);
                        index -= bytes_per_pixel;
                        from += bytes_per_pixel;
                    }
                }
                bytes_to_flush = (y_end - y_start) * bytes_per_line;
                flush_ptr = fb + (v_res - y_end) * bytes_per_line;
                break;

            case ROTATION_270:
                for (int y = y_start; y < y_end; y++) {
                    for (int x = x_start; x < x_end; x++) {
                        uint32_t j = y * copy_bytes_per_line + x * bytes_per_pixel - offset;
                        uint32_t i = (x * h_res + h_res - 1 - y) * bytes_per_pixel;
                        cpy_func(to + i, from + j);
                    }
                }
                bytes_to_flush = (x_end - x_start) * bytes_per_line;
                flush_ptr = fb + x_start * bytes_per_line;
                break;

            default:
                LCD_UNUSED(cpy_func);
                LCD_UNUSED(offset);
                to = fb + (y_start * h_res + x_start) * bytes_per_pixel;
                for (int y = y_start; y < y_end; y++) {
                    memcpy(to, from, copy_bytes_per_line);
                    to += bytes_per_line;
                    from += copy_bytes_per_line;
                }
                bytes_to_flush = (y_end - y_start) * bytes_per_line;
                flush_ptr = fb + y_start * bytes_per_line;
                break;
            }

            LCD_UNUSED(bytes_to_flush);
            LCD_UNUSED(flush_ptr);

            if (last_flush) {
                ret = esp_lcd_panel_io_tx_color(self->panel_io_handle.panel_io, lcd_cmd, fb, (size_t)(v_res * h_res * bytes_per_pixel));
            }

            if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
                mp_call_function_n_kw(self->callback, 0, 0, NULL);
            }
        }

        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_tx_color)"), ret);
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
