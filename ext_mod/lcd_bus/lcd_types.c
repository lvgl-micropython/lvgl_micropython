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
