#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/idf_additions.h"
#include "esp_task.h"

#include "common/lcd_common_types.h"
#include "common/sw_rotate_task_common.h"
#include "common/sw_rotate.h"
#include "sw_rotate_task.h"
#include "lcd_types.h"

#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_cpu.h"

// micropy includes
#include "py/gc.h"
#include "py/stackctrl.h"
#include "mphalport.h"


#define LCD_DEFAULT_STACK_SIZE    (5 * 1024)
#define LCDBUS_BIT_0 (1 << 0)


void mp_lcd_event_init(mp_lcd_event_t *event)
{
    event->handle = xEventGroupCreateStatic(&event->buffer);
}


void mp_lcd_event_delete(mp_lcd_event_t *event)
{
    xEventGroupSetBits(event->handle, LCDBUS_BIT_0);
    vEventGroupDelete(event->handle);

}

void mp_lcd_event_wait(mp_lcd_event_t *event)
{
    xEventGroupWaitBits(event->handle, LCDBUS_BIT_0, pdFALSE, pdTRUE, portMAX_DELAY);
}


bool mp_lcd_event_isset(mp_lcd_event_t *event)
{
    return (bool)(xEventGroupGetBits(event->handle) & LCDBUS_BIT_0);
}


bool mp_lcd_event_isset_from_isr(mp_lcd_event_t *event)
{
    return (bool)(xEventGroupGetBitsFromISR(event->handle) & LCDBUS_BIT_0);
}


void mp_lcd_event_set(mp_lcd_event_t *event)
{
    xEventGroupSetBits(event->handle, LCDBUS_BIT_0);
}


void mp_lcd_event_clear(mp_lcd_event_t *event)
{
    xEventGroupClearBits(event->handle, LCDBUS_BIT_0);
}


void mp_lcd_event_clear_from_isr(mp_lcd_event_t *event)
{
    xEventGroupClearBitsFromISR(event->handle, LCDBUS_BIT_0);
}


void mp_lcd_event_set_from_isr(mp_lcd_event_t *event)
{
    xEventGroupSetBitsFromISR(event->handle, LCDBUS_BIT_0, pdFALSE);
}


void mp_lcd_lock_acquire(mp_lcd_lock_t *lock)
{
    xSemaphoreTake(lock->handle, portMAX_DELAY);
}


void mp_lcd_lock_release(mp_lcd_lock_t *lock)
{
    xSemaphoreGive(lock->handle);
}


void mp_lcd_lock_release_from_isr(mp_lcd_lock_t *lock)
{
    xSemaphoreGiveFromISR(lock->handle, pdFALSE);
}


void mp_lcd_lock_init(mp_lcd_lock_t *lock)
{
    lock->handle = xSemaphoreCreateBinaryStatic(&lock->buffer);
    xSemaphoreGive(lock->handle);
}


void mp_lcd_lock_delete(mp_lcd_lock_t *lock)
{
    vSemaphoreDelete(lock->handle);
}


bool mp_lcd_start_rotate_task(void *self_in)
{
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)self_in;

    mp_lcd_lock_acquire(&self->sw_rot.handles.init_lock);

    xTaskCreatePinnedToCore(
                mp_lcd_sw_rotate_task, "rotate_task", LCD_DEFAULT_STACK_SIZE / sizeof(StackType_t),
                self, ESP_TASK_PRIO_MAX - 1, &self->sw_rot.handles.task_handle, 0);

    mp_lcd_lock_acquire(&self->sw_rot.handles.init_lock);
    mp_lcd_lock_release(&self->sw_rot.handles.init_lock);

    if (self->sw_rot.init.err != LCD_OK) {
        mp_raise_msg_varg(&mp_type_OSError, self->sw_rot.init.err_msg, self->sw_rot.init.err);
        return false;
    } else {
        return true;
    }
}


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
void mp_lcd_flush_ready_cb(mp_obj_t cb, bool wake)
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

    if (wake) mp_hal_wake_main_task_from_isr();
}
