//local includes
#include "lcd_types.h"

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objarray.h"

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


    mp_lcd_err_t lcd_panel_io_rx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size)
    {
        if (io->rx_param == NULL) {
            return esp_lcd_panel_io_rx_param(io->panel_io, lcd_cmd, param, param_size);
        } else {
            return io->rx_param(io, lcd_cmd, param, param_size);
        }
    }


    mp_lcd_err_t lcd_panel_io_tx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size)
    {
        if (io->tx_param == NULL) {
            return esp_lcd_panel_io_tx_param(io->panel_io, lcd_cmd, param, param_size);
        } else {
            return io->tx_param(io, lcd_cmd, param, param_size);
        }
    }


    mp_lcd_err_t lcd_panel_io_tx_color(lcd_panel_io_t *io, int lcd_cmd, void *color, size_t color_size)
    {
        if (io->tx_color == NULL) {

            mp_lcd_bus_obj_t *self = __containerof(io, mp_lcd_bus_obj_t, panel_io_handle);
            if (self->rgb565_byte_swap) {
                rgb565_byte_swap((uint16_t *)color, (uint32_t)(color_size / 2));
            }

            return esp_lcd_panel_io_tx_color(io->panel_io, lcd_cmd, color, color_size);
        } else {
            return io->tx_color(io, lcd_cmd, color, color_size);
        }
    }

#else
    bool bus_trans_done_cb(lcd_panel_io_t *panel_io, void *edata, void *user_ctx)
    {
        mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)user_ctx;

        if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
            mp_call_function_n_kw(self->callback, 0, 0, NULL);
        }
        self->trans_done = true;
        return false;
    }


    mp_lcd_err_t lcd_panel_io_rx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size)
    {
        if (io->rx_param == NULL) return LCD_ERR_NOT_SUPPORTED;
        return io->rx_param(io, lcd_cmd, param, param_size);
    }


    mp_lcd_err_t lcd_panel_io_tx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size)
    {
        return io->tx_param(io, lcd_cmd, param, param_size);
    }


    mp_lcd_err_t lcd_panel_io_tx_color(lcd_panel_io_t *io, int lcd_cmd, void *color, size_t color_size)
    {
        mp_lcd_bus_obj_t *self = __containerof(io, mp_lcd_bus_obj_t, panel_io_handle);
        if (self->rgb565_byte_swap) {
            rgb565_byte_swap((uint16_t *)color, (uint32_t)(color_size / 2));
        }

        return io->tx_color(io, lcd_cmd, color, color_size);
    }

#endif

mp_lcd_err_t lcd_panel_io_del(lcd_panel_io_t  *io)
{
    if (io->del != NULL) {
        return io->del(io);
    } else {
        return LCD_OK;
    }
}


mp_lcd_err_t lcd_panel_io_init(lcd_panel_io_t *io, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size)
{
    return io->init(io, width, height, bpp, buffer_size);
}


mp_lcd_err_t lcd_panel_io_get_lane_count(lcd_panel_io_t *io, uint8_t *lane_count)
{
    return io->get_lane_count(io, lane_count);
}
