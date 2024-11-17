#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED
    // micropython includes
    #include "py/obj.h"
    #include "py/runtime.h"
    #include "py/gc.h"

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"
    #include "freertos/event_groups.h"
    #include "freertos/idf_additions.h"

    #include "esp_lcd_panel_ops.h"

    #include "rgb_bus.h"

    #include <string.h>

    #define RGB_BIT_0 (1 << 0)


    void rgb_bus_event_init(rgb_bus_event_t *event)
    {
        event->handle = xEventGroupCreateStatic(&event->buffer);
    }


    void rgb_bus_event_delete(rgb_bus_event_t *event)
    {
        xEventGroupSetBits(event->handle, RGB_BIT_0);
        vEventGroupDelete(event->handle);
    }


    bool rgb_bus_event_isset(rgb_bus_event_t *event)
    {
        return (bool)(xEventGroupGetBits(event->handle) == RGB_BIT_0);
    }


    bool rgb_bus_event_isset_from_isr(rgb_bus_event_t *event)
    {
        return (bool)(xEventGroupGetBitsFromISR(event->handle) == RGB_BIT_0);
    }


    void rgb_bus_event_set(rgb_bus_event_t *event)
    {
        xEventGroupSetBits(event->handle, RGB_BIT_0);
    }


    void rgb_bus_event_clear(rgb_bus_event_t *event)
    {
        xEventGroupClearBits(event->handle, RGB_BIT_0);
    }

    void rgb_bus_event_clear_from_isr(rgb_bus_event_t *event)
    {
        xEventGroupClearBitsFromISR(event->handle, RGB_BIT_0);
    }

    void rgb_bus_event_set_from_isr(rgb_bus_event_t *event)
    {
        xEventGroupSetBitsFromISR(event->handle, RGB_BIT_0, pdFALSE);
    }


    int rgb_bus_lock_acquire(rgb_bus_lock_t *lock, int32_t wait_ms)
    {
        return pdTRUE == xSemaphoreTake(lock->handle, wait_ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS((uint16_t)wait_ms));
    }


    void rgb_bus_lock_release(rgb_bus_lock_t *lock)
    {
        xSemaphoreGive(lock->handle);
    }


    void rgb_bus_lock_release_from_isr(rgb_bus_lock_t *lock)
    {
        xSemaphoreGiveFromISR(lock->handle, pdFALSE);
    }


    void rgb_bus_lock_init(rgb_bus_lock_t *lock)
    {
        lock->handle = xSemaphoreCreateBinaryStatic(&lock->buffer);
        xSemaphoreGive(lock->handle);
    }


    void rgb_bus_lock_delete(rgb_bus_lock_t *lock)
    {
        vSemaphoreDelete(lock->handle);
    }

    #define RGB_BUS_ROTATION_0    (0)
    #define RGB_BUS_ROTATION_90   (1)
    #define RGB_BUS_ROTATION_180  (2)
    #define RGB_BUS_ROTATION_270  (3)

    typedef void (* copy_func_cb_t)(uint8_t *to, const uint8_t *from);

    static void copy_pixels(
                uint8_t *to, uint8_t *from, uint32_t x_start, uint32_t y_start,
                uint32_t x_end, uint32_t y_end, uint32_t h_res, uint32_t v_res,
                uint32_t bytes_per_pixel, copy_func_cb_t func, uint8_t rotate);


    __attribute__((always_inline))
    static inline void copy_8bpp(uint8_t *to, const uint8_t *from)
    {
        *to++ = *from++;
    }

    __attribute__((always_inline))
    static inline void copy_16bpp(uint8_t *to, const uint8_t *from)
    {
        *to++ = *from++;
        *to++ = *from++;
    }

    __attribute__((always_inline))
    static inline void copy_24bpp(uint8_t *to, const uint8_t *from)
    {
        *to++ = *from++;
        *to++ = *from++;
        *to++ = *from++;
    }

    void rgb_bus_copy_task(void *self_in) {
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)self_in;

        copy_func_cb_t func;
        uint8_t bytes_per_pixel = self->bytes_per_pixel;

        switch (bytes_per_pixel) {
            case 1:
                func = copy_8bpp;
                break;
            case 2:
                func = copy_16bpp;
                break;
            case 3:
                func = copy_24bpp;
                break;
            default:
                // raise error
                return;
        }

        rgb_bus_lock_acquire(&self->copy_lock, -1);

        while (!rgb_bus_event_isset(&self->copy_task_exit)) {
            rgb_bus_lock_acquire(&self->copy_lock, -1);

            if (rgb_bus_event_isset(&self->partial_copy)) {
                rgb_bus_event_clear(&self->partial_copy);

                copy_pixels(
                    self->idle_fb, self->partial_buf,
                    self->x_start, self->y_start,
                    self->x_end, self->y_end,
                    self->width, self->height,
                    bytes_per_pixel, func, self->rotation);

                if (self->callback != mp_const_none) mp_sched_schedule(self->callback, MP_OBJ_FROM_PTR(self));
            }

            if (rgb_bus_event_isset(&self->last_update)) {
                rgb_bus_event_clear(&self->last_update);
                uint8_t *idle_fb = self->idle_fb;
                rgb_bus_event_set(&self->swap_bufs);

                esp_lcd_panel_draw_bitmap(
                    self->panel_handle,
                    0,
                    0,
                    self->width,
                    self->height,
                    idle_fb
                );

                rgb_bus_lock_acquire(&self->swap_lock, -1);
                memcpy(self->idle_fb, self->active_fb, self->width * self->height * bytes_per_pixel);
            }
        }
    }


    void copy_pixels(
        uint8_t *to,
        uint8_t *from,
        uint32_t x_start,
        uint32_t y_start,
        uint32_t x_end,
        uint32_t y_end,
        uint32_t h_res,
        uint32_t v_res,
        uint32_t bytes_per_pixel,
        copy_func_cb_t func,
        uint8_t rotate
    ) {

        if (rotate == RGB_BUS_ROTATION_90 || rotate == RGB_BUS_ROTATION_270) {
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

        uint16_t copy_bytes_per_line = (x_end - x_start) * (uint16_t)bytes_per_pixel;
        int pixels_per_line = h_res;
        uint32_t bytes_per_line = bytes_per_pixel * pixels_per_line;
        size_t offset = y_start * copy_bytes_per_line + x_start * bytes_per_pixel;

        switch (rotate) {
            case RGB_BUS_ROTATION_0:
                uint8_t *fb = to + (y_start * h_res + x_start) * bytes_per_pixel;
                for (int y = y_start; y < y_end; y++) {
                    memcpy(fb, from, copy_bytes_per_line);
                    fb += bytes_per_line;
                    from += copy_bytes_per_line;
                }
                break;

            case RGB_BUS_ROTATION_180:
                uint32_t index;
                for (int y = y_start; y < y_end; y++) {
                    index = ((v_res - 1 - y) * h_res + (h_res - 1 - x_start)) * bytes_per_pixel;
                    for (size_t x = x_start; x < x_end; x++)
                    {
                        func(to + index, from);
                        index -= bytes_per_pixel;
                        from += bytes_per_pixel;
                    }
                }
                break;

            case RGB_BUS_ROTATION_90:
                uint32_t j;
                uint32_t i;

                for (int y = y_start; y < y_end; y++) {
                    for (int x = x_start; x < x_end; x++) {
                        j = y * copy_bytes_per_line + x * bytes_per_pixel - offset;
                        i = (x * h_res + y) * bytes_per_pixel;
                        func(to + i, from + j);
                    }
                }
                break;

            case RGB_BUS_ROTATION_270:
                uint32_t jj;
                uint32_t ii;

                for (int y = y_start; y < y_end; y++) {
                    for (int x = x_start; x < x_end; x++) {
                        jj = y * copy_bytes_per_line + x * bytes_per_pixel - offset;
                        ii = ((v_res - 1 - x) * h_res + h_res - 1 - y) * bytes_per_pixel;
                        func(to + ii, from + jj);
                    }
                }
                break;

            default:
                break;
        }
    }

#endif