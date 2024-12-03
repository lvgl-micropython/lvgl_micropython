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
    #include "rom/ets_sys.h"
    #include "esp_system.h"
    #include "esp_cpu.h"

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

    void rgb_bus_event_wait(rgb_bus_event_t *event)
    {
        xEventGroupWaitBits(event->handle, RGB_BIT_0, pdFALSE, pdTRUE, portMAX_DELAY);
    }


    bool rgb_bus_event_isset(rgb_bus_event_t *event)
    {
        return (bool)(xEventGroupGetBits(event->handle) & RGB_BIT_0);
    }


    bool rgb_bus_event_isset_from_isr(rgb_bus_event_t *event)
    {
        return (bool)(xEventGroupGetBitsFromISR(event->handle) & RGB_BIT_0);
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


    static bool rgb_bus_trans_done_cb(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
    {
        LCD_UNUSED(edata);
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)user_ctx;
        rgb_panel_t *rgb_panel = __containerof(panel, rgb_panel_t, base);
        uint8_t *curr_buf = rgb_panel->fbs[rgb_panel->cur_fb_index];

        if (curr_buf != self->active_fb && !rgb_bus_event_isset_from_isr(&self->swap_bufs)) {
            uint8_t *idle_fb = self->idle_fb;
            self->idle_fb = self->active_fb;
            self->active_fb = idle_fb;
            rgb_bus_event_set_from_isr(&self->swap_bufs);
        }

        return false;
    }


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
        LCD_DEBUG_PRINT("rgb_bus_copy_task - STARTED\n")

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)self_in;

        esp_lcd_rgb_panel_event_callbacks_t callbacks = { .on_vsync = rgb_bus_trans_done_cb };

        self->init_err = esp_lcd_new_rgb_panel(&self->panel_io_config, &self->panel_handle);
        if (self->init_err != 0) {
            self->init_err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_rgb_panel)");
            rgb_bus_lock_release(&self->init_lock);
            return;
        }

        self->init_err = esp_lcd_rgb_panel_register_event_callbacks(self->panel_handle, &callbacks, self);
        if (self->init_err != 0) {
            self->init_err_msg = MP_ERROR_TEXT("%d(esp_lcd_rgb_panel_register_event_callbacks)");
            rgb_bus_lock_release(&self->init_lock);
            return;
        }

        self->init_err = esp_lcd_panel_reset(self->panel_handle);
        if (self->init_err != 0) {
            self->init_err_msg = MP_ERROR_TEXT("%d(esp_lcd_panel_reset)");
            rgb_bus_lock_release(&self->init_lock);
            return;
        }

        self->init_err = esp_lcd_panel_init(self->panel_handle);
        if (self->init_err != 0) {
            self->init_err_msg = MP_ERROR_TEXT("%d(esp_lcd_panel_init)");
            rgb_bus_lock_release(&self->init_lock);
            return;
        }

        rgb_panel_t *rgb_panel = __containerof((esp_lcd_panel_t *)self->panel_handle, rgb_panel_t, base);

        self->active_fb = rgb_panel->fbs[0];
        self->idle_fb = rgb_panel->fbs[1];

        uint8_t *idle_fb;
        copy_func_cb_t func;
        bool last_update;

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

        self->init_err = LCD_OK;
        rgb_bus_lock_release(&self->init_lock);

        bool exit = rgb_bus_event_isset(&self->copy_task_exit);
        while (!exit) {
            rgb_bus_lock_acquire(&self->copy_lock, -1);

            if (self->partial_buf == NULL) break;
            last_update = self->last_update;


        #if LCD_RGB_OPTIMUM_FB_SIZE
            self->optimum_fb.flush_count += 1;
        #endif

            idle_fb = self->idle_fb;

            copy_pixels(
                idle_fb, self->partial_buf,
                self->x_start, self->y_start,
                self->x_end, self->y_end,
                self->width, self->height,
                bytes_per_pixel, func, self->rotation);

            rgb_bus_lock_release(&self->tx_color_lock);

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
                if (self->optimum_fb.curr_index == 254) {
                    self->optimum_fb.curr_index = 0;
                } else {
                    self->optimum_fb.curr_index += 1;
                }
                if (self->optimum_fb.sample_count < 255) {
                    self->optimum_fb.sample_count += 1;
                }
                self->optimum_fb.samples[self->optimum_fb.curr_index] = self->optimum_fb.flush_count;
                self->optimum_fb.flush_count = 0;

                rgb_bus_lock_release(&self->optimum_fb.lock);
                rgb_bus_lock_acquire(&self->optimum_fb.lock, -1);
            #endif

                mp_lcd_err_t ret = esp_lcd_panel_draw_bitmap(
                    self->panel_handle,
                    0,
                    0,
                    self->width - 1,
                    self->height - 1,
                    idle_fb
                );

                if (ret != 0) {
                    mp_printf(&mp_plat_print, "esp_lcd_panel_draw_bitmap error (%d)\n", ret);
                } else {
                    rgb_bus_event_clear(&self->swap_bufs);
                    rgb_bus_event_wait(&self->swap_bufs);
                    memcpy(self->idle_fb, self->active_fb, self->width * self->height * bytes_per_pixel);
                }
            }

            exit = rgb_bus_event_isset(&self->copy_task_exit);
        }

        LCD_DEBUG_PRINT(&mp_plat_print, "rgb_bus_copy_task - STOPPED\n")
    }


    void copy_pixels(uint8_t *dst, uint8_t *src, uint32_t x_start, uint32_t y_start,
            uint32_t x_end, uint32_t y_end, uint32_t d_width, uint32_t d_height,
            uint32_t bytes_per_pixel, copy_func_cb_t func, uint8_t rotate)
    {
        if (rotate == RGB_BUS_ROTATION_90 || rotate == RGB_BUS_ROTATION_270) {
            x_start = MIN(x_start, d_height);
            x_end = MIN(x_end, d_height);
            y_start = MIN(y_start, d_width);
            y_end = MIN(y_end, d_width);
        } else {
            x_start = MIN(x_start, d_width);
            x_end = MIN(x_end, d_width);
            y_start = MIN(y_start, d_height);
            y_end = MIN(y_end, d_height);
        }

        uint16_t src_bytes_per_line = (x_end - x_start + 1) * (uint16_t)bytes_per_pixel;
        uint32_t dst_bytes_per_line = bytes_per_pixel * d_width;
        size_t offset = y_start * src_bytes_per_line + x_start * bytes_per_pixel;

        // mp_printf(&mp_plat_print, "x_start=%lu, y_start=%lu, x_end=%lu, y_end=%lu, copy_bytes_per_line=%u, bytes_per_line=%lu, %lu\n",
        //         x_start, y_start, x_end, y_end, copy_bytes_per_line, bytes_per_line, (uint32_t)((y_start * h_res + x_start) * bytes_per_pixel));

        switch (rotate) {
            case RGB_BUS_ROTATION_0:
                uint8_t *fb = dst + ((y_start * d_width + x_start) * bytes_per_pixel);

                if (x_start == 0 && x_end == (d_width - 1)) {
                    memcpy(fb, src, d_width * (y_end - y_start + 1) * bytes_per_pixel);
                } else {
                    for (int y = y_start; y < y_end; y++) {
                        memcpy(fb, src, src_bytes_per_line);
                        fb += dst_bytes_per_line;
                        src += src_bytes_per_line;
                    }
                }

                break;

            // SWAP_XY   MIRROR_Y
            case RGB_BUS_ROTATION_90:
                uint32_t j;
                uint32_t i;

                for (int y = y_start; y < y_end; y++) {
                    for (int x = x_start; x < x_end; x++) {
                        j = y * src_bytes_per_line + x * bytes_per_pixel - offset;
                        i = ((d_height - 1 - x) * d_width + y) * bytes_per_pixel;
                        func(dst + i, src + j);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case RGB_BUS_ROTATION_180:
                uint32_t index;
                for (int y = y_start; y < y_end; y++) {
                    index = ((d_height - 1 - y) * d_width + (d_width - 1 - x_start)) * bytes_per_pixel;
                    for (size_t x = x_start; x < x_end; x++) {
                        func(dst + index, src);
                        index -= bytes_per_pixel;
                        src += bytes_per_pixel;
                    }
                }
                break;

            // SWAP_XY   MIRROR_X
            case RGB_BUS_ROTATION_270:
                uint32_t jj;
                uint32_t ii;

                for (int y = y_start; y < y_end; y++) {
                    for (int x = x_start; x < x_end; x++) {
                        jj = y * src_bytes_per_line + x * bytes_per_pixel - offset;
                        ii = (x * d_width + d_width - 1 - y) * bytes_per_pixel;
                        func(dst + ii, src + jj);
                    }
                }
                break;

            default:
                break;
        }
    }

#endif
