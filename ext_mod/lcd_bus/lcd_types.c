// Copyright (c) 2024 - 2025 Kevin G. Schlosser

//local includes
#include "lcd_types.h"

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objarray.h"
#include "py/binary.h"

void rgb565_byte_swap(void *buf, uint32_t buf_size_px)
{
    uint16_t *buf16 = (uint16_t *)buf;

    while (buf_size_px > 0) {
        buf16[0] =  (buf16[0] << 8) | (buf16[0] >> 8);
        buf16++;
        buf_size_px--;
    }
}


#ifdef ESP_IDF_VERSION
    // esp-idf includes
    #include "esp_lcd_panel_io.h"
    #include "esp_lcd_types.h"
    #include "esp_heap_caps.h"
    #include "rom/ets_sys.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "esp_system.h"
    #include "esp_cpu.h"

    // micropy includes
    #include "py/gc.h"
    #include "py/stackctrl.h"
    #include "mphalport.h"

    // The 2 functions below are specific to ESP32. They cat called within an ISR context
    // since the rest of the boards are either bitbang or utilize the micropython
    // builtin data busses which do not support DMA transfers the functions do not
    // get called within an ISR context so we have to define the functions differently

    // cb_isr function taken directly from:
    // https://github.com/lvgl/lv_binding_micropython/blob/master/driver/esp32/espidf.c
    // Requires CONFIG_FREERTOS_INTERRUPT_BACKTRACE=n in sdkconfig
    //
    // Can't use mp_sched_schedule because lvgl won't yield to give micropython a chance to run
    // Must run Micropython in ISR itself.
    // Called in ISR context!
    void cb_isr(mp_obj_t cb)
    {
        volatile uint32_t sp = (uint32_t)esp_cpu_get_sp();

        // Calling micropython from ISR
        // See: https://github.com/micropython/micropython/issues/4895
        void *old_state = mp_thread_get_state();

        mp_state_thread_t ts; // local thread state for the ISR
        mp_thread_set_state(&ts);
        mp_stack_set_top((void*)sp); // need to include in root-pointer scan
        mp_stack_set_limit(CONFIG_FREERTOS_IDLE_TASK_STACKSIZE - 1024); // tune based on ISR thread stack size
        mp_locals_set(mp_state_ctx.thread.dict_locals); // use main thread's locals
        mp_globals_set(mp_state_ctx.thread.dict_globals); // use main thread's globals

        mp_sched_lock(); // prevent VM from switching to another MicroPython thread
        gc_lock(); // prevent memory allocation

        nlr_buf_t nlr;
        if (nlr_push(&nlr) == 0) {
            mp_call_function_n_kw(cb, 0, 0, NULL);
            nlr_pop();
        } else {
            ets_printf("Uncaught exception in IRQ callback handler!\n");
            mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));  // changed to &mp_plat_print to fit this context
        }

        gc_unlock();
        mp_sched_unlock();

        mp_thread_set_state(old_state);
        mp_hal_wake_main_task_from_isr();
    }

    // called when esp_lcd_panel_draw_bitmap is completed
    bool bus_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
    {
        mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)user_ctx;

        if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
            cb_isr(self->callback);
        }
        self->trans_done = true;
        return false;
    }


    mp_obj_t lcd_panel_io_free_framebuffer(mp_obj_t obj, mp_obj_t buf)
    {
        mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;

        if (self->panel_io_handle.free_framebuffer == NULL) {
            mp_obj_array_t *array_buf = (mp_obj_array_t *)MP_OBJ_TO_PTR(buf);
            void *item_buf = array_buf->items;

            if (item_buf == NULL) {
                return mp_const_none;
            }

            if (array_buf == self->view1) {
                heap_caps_free(item_buf);
                self->view1 = NULL;
                LCD_DEBUG_PRINT("lcd_panel_io_free_framebuffer(self, buf=1)\n")
            } else if (array_buf == self->view2) {
                heap_caps_free(item_buf);
                self->view2 = NULL;
                LCD_DEBUG_PRINT("lcd_panel_io_free_framebuffer(self, buf=2)\n")
            } else {
                mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("No matching buffer found"));
            }
            return mp_const_none;
        } else {
            return self->panel_io_handle.free_framebuffer(obj, buf);
        }
    }

    mp_lcd_err_t lcd_panel_io_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
       mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;

        if (self->panel_io_handle.rx_param == NULL) {
            LCD_DEBUG_PRINT("lcd_panel_io_rx_param(self, lcd_cmd=%d, param, param_size=%d)\n", lcd_cmd, param_size)
            return esp_lcd_panel_io_rx_param(self->panel_io_handle.panel_io, lcd_cmd, param, param_size);
        } else {
            return self->panel_io_handle.rx_param(obj, lcd_cmd, param, param_size);
        }
    }


    mp_lcd_err_t lcd_panel_io_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;

        if (self->panel_io_handle.tx_param == NULL) {
            LCD_DEBUG_PRINT("lcd_panel_io_tx_param(self, lcd_cmd=%d, param, param_size=%d)\n", lcd_cmd, param_size)
            return esp_lcd_panel_io_tx_param(self->panel_io_handle.panel_io, lcd_cmd, param, param_size);
        } else {
            return self->panel_io_handle.tx_param(obj, lcd_cmd, param, param_size);
        }
    }


    mp_lcd_err_t lcd_panel_io_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update)
    {
        mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;

        if (self->rgb565_byte_swap) {
            rgb565_byte_swap((uint16_t *)color, (uint32_t)(color_size / 2));
        }

        if (self->panel_io_handle.tx_color == NULL) {
            LCD_UNUSED(x_start);
            LCD_UNUSED(y_start);
            LCD_UNUSED(x_end);
            LCD_UNUSED(y_end);
            LCD_UNUSED(rotation);
            LCD_UNUSED(last_update);

            LCD_DEBUG_PRINT("lcd_panel_io_tx_color(self, lcd_cmd=%d, color, color_size=%d)\n", lcd_cmd, color_size)
            return esp_lcd_panel_io_tx_color(self->panel_io_handle.panel_io, lcd_cmd, color, color_size);
        } else {
            return self->panel_io_handle.tx_color(obj, lcd_cmd, color, color_size, x_start, y_start, x_end, y_end, rotation, last_update);
        }
    }


    mp_obj_t lcd_panel_io_allocate_framebuffer(mp_obj_t obj, uint32_t size, uint32_t caps)
    {
        mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;
        LCD_DEBUG_PRINT("lcd_panel_io_allocate_framebuffer(self, size=%lu, caps=%lu)\n", size, caps)

        if (self->panel_io_handle.allocate_framebuffer == NULL) {
            mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;

            void *buf = heap_caps_calloc(1, size, caps);

            if (buf == NULL) {
               mp_raise_msg_varg(
                   &mp_type_MemoryError,
                   MP_ERROR_TEXT("Not enough memory available (%d)"),
                   size
               );
               return mp_const_none;
            }

            mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, buf));
            view->typecode |= 0x80; // used to indicate writable buffer

            if (self->view1 == NULL) {
                self->view1 = view;
                self->buffer_flags = caps;
            } else if (self->buffer_flags != caps) {
                mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("allocation flags must be the same for both buffers"));
                return mp_const_none;
            } else if (self->view2 == NULL) {
                self->view2 = view;
            } else {
                heap_caps_free(buf);
                mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("There is a maximum of 2 frame buffers allowed"));
                return mp_const_none;
            }

            return MP_OBJ_FROM_PTR(view);
        } else {
            return self->panel_io_handle.allocate_framebuffer(obj, size, caps);
        }
    }

#else
    bool bus_trans_done_cb(lcd_panel_io_t *panel_io, void *edata, void *user_ctx)
    {
        LCD_UNUSED(edata);
        LCD_UNUSED(panel_io);

        mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)user_ctx;

        if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
            mp_call_function_n_kw(self->callback, 0, 0, NULL);
        }

        self->trans_done = true;
        return false;
    }


    mp_obj_t lcd_panel_io_free_framebuffer(mp_obj_t obj, mp_obj_t buf)
    {
        mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;

        if (self->panel_io_handle.free_framebuffer == NULL) {
            mp_obj_array_t *array_buf = (mp_obj_array_t *)MP_OBJ_TO_PTR(buf);

            void *buf = array_buf->items;

            if (buf == self->buf1) {
                m_free(buf);
                self->buf1 = NULL;
            } else if (buf == self->buf2) {
                m_free(buf);
                self->buf2 = NULL;
            } else {
                mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("No matching buffer found"));
            }
            return mp_const_none;
        } else {
            return self->panel_io_handle.free_framebuffer(obj, buf);
        }
    }


    mp_lcd_err_t lcd_panel_io_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;

        if (self->panel_io_handle.rx_param == NULL) return LCD_ERR_NOT_SUPPORTED;
        return self->panel_io_handle.rx_param(obj, lcd_cmd, param, param_size);
    }


    mp_lcd_err_t lcd_panel_io_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;

        return self->panel_io_handle.tx_param(obj, lcd_cmd, param, param_size);
    }


    mp_lcd_err_t lcd_panel_io_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update)
    {
        mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;

        if (self->rgb565_byte_swap) {
            rgb565_byte_swap((uint16_t *)color, (uint32_t)(color_size / 2));
        }

        return self->panel_io_handle.tx_color(obj, lcd_cmd, color, color_size, x_start, y_start, x_end, y_end, rotation, last_update);
    }

    mp_obj_t lcd_panel_io_allocate_framebuffer(mp_obj_t obj, uint32_t size, uint32_t caps)
    {
        mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;

        if (self->panel_io_handle.allocate_framebuffer == NULL) {
            LCD_UNUSED(caps);
            void *buf = m_malloc(size);

            if (buf == NULL) {
                mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Unable to allocate frame buffer"));
                return mp_const_none;
            } else {
                if (self->buf1 == NULL) {
                    self->buf1 = buf;
                    self->buffer_flags = caps;
                } else if (self->buf2 == NULL && self->buffer_flags == caps) {
                    self->buf2 = buf;
                } else {
                    m_free(buf);
                    if (self->buf2 == NULL) {
                        mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("allocation flags must be the same for both buffers"));
                    } else {
                        mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Only 2 buffers can be allocated"));
                    }
                    return mp_const_none;
                }

                mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, buf));
                view->typecode |= 0x80; // used to indicate writable buffer
                return MP_OBJ_FROM_PTR(view);
            }
        } else {
            return self->panel_io_handle.allocate_framebuffer(obj, size, caps);
        }
    }
#endif


mp_lcd_err_t lcd_panel_io_del(mp_obj_t obj)
{
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;

    if (self->panel_io_handle.del != NULL) {
        return self->panel_io_handle.del(obj);
    } else {
        LCD_DEBUG_PRINT("lcd_panel_io_del(self)\n")
        return LCD_OK;
    }
}


mp_lcd_err_t lcd_panel_io_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits)
{
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;

    return self->panel_io_handle.init(obj, width, height, bpp, buffer_size, rgb565_byte_swap, cmd_bits, param_bits);
}


mp_lcd_err_t lcd_panel_io_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
{
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)obj;

    return self->panel_io_handle.get_lane_count(obj, lane_count);
}
