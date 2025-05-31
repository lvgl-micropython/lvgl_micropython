// Copyright (c) 2024 - 2025 Kevin G. Schlosser

//local includes
#include "lcd_types.h"

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

// esp includes
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
