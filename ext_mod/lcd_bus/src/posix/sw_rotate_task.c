
#include <pthread.h>

#include "common/lcd_common_types.h"
#include "common/sw_rotate_task_common.h"
#include "common/sw_rotate.h"
#include "sw_rotate_task.h"
#include "lcd_types.h"

#include "py/gc.h"
#include "py/stackctrl.h"

void mp_lcd_event_init(mp_lcd_event_t *event)
{
    pthread_condattr_init(&event->attr);
    pthread_cond_init(&event->handle, NULL);
    mp_lcd_lock_init(&event->event_lock);
    event->state = 0;
}


void mp_lcd_event_delete(mp_lcd_event_t *event)
{
    mp_lcd_lock_acquire(&event->event_lock);
    event->state = 0;
    pthread_cond_broadcast(&event->handle);
    mp_lcd_lock_release(&event->event_lock);

    pthread_cond_destroy(&event->handle);
    pthread_condattr_destroy(&event->attr);
    mp_lcd_lock_delete(&event->event_lock);

}


void mp_lcd_event_wait(mp_lcd_event_t *event)
{
    mp_lcd_lock_acquire(&event->event_lock);
    if (event->state) {
        mp_lcd_lock_release(&event->event_lock);
    } else {
        pthread_cond_wait(&event->handle, &event->event_lock.handle);
        mp_lcd_lock_release(&event->event_lock);
    }
}


bool mp_lcd_event_isset(mp_lcd_event_t *event)
{
    mp_lcd_lock_acquire(&event->event_lock);
    uint8_t state = event->state;
    mp_lcd_lock_release(&event->event_lock);
    return (bool)state;
}


bool mp_lcd_event_isset_from_isr(mp_lcd_event_t *event)
{
    mp_lcd_lock_acquire(&event->event_lock);
    uint8_t state = event->state;
    mp_lcd_lock_release(&event->event_lock);
    return (bool)state;
}


void mp_lcd_event_set(mp_lcd_event_t *event)
{
    mp_lcd_lock_acquire(&event->event_lock);
    event->state = 1;
    pthread_cond_broadcast(&event->handle);
    mp_lcd_lock_release(&event->event_lock);
}


void mp_lcd_event_clear(mp_lcd_event_t *event)
{
    mp_lcd_lock_acquire(&event->event_lock);
    event->state = 0;
    mp_lcd_lock_release(&event->event_lock);
}


void mp_lcd_event_clear_from_isr(mp_lcd_event_t *event)
{
    mp_lcd_lock_acquire(&event->event_lock);
    event->state = 0;
    mp_lcd_lock_release(&event->event_lock);
}


void mp_lcd_event_set_from_isr(mp_lcd_event_t *event)
{
    mp_lcd_lock_acquire(&event->event_lock);
    event->state = 1;
    pthread_cond_broadcast(&event->handle);
    mp_lcd_lock_release(&event->event_lock);
}


void mp_lcd_lock_acquire(mp_lcd_lock_t *lock)
{
    pthread_mutex_lock(&lock->handle);
}


void mp_lcd_lock_release(mp_lcd_lock_t *lock)
{
    pthread_mutex_unlock(&lock->handle);
}


void mp_lcd_lock_release_from_isr(mp_lcd_lock_t *lock)
{
    pthread_mutex_unlock(&lock->handle);
}


void mp_lcd_lock_init(mp_lcd_lock_t *lock)
{
    pthread_mutexattr_init(&lock->attr);
    pthread_mutexattr_settype(&lock->attr, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&lock->handle, &lock->attr);
}


void mp_lcd_lock_delete(mp_lcd_lock_t *lock)
{
    pthread_mutex_unlock(&lock->handle);
    pthread_mutex_destroy(&lock->handle);
    pthread_mutexattr_destroy(&lock->attr);
}


void mp_lcd_flush_ready_cb(mp_obj_t cb, bool wake)
{
    LCD_UNUSED(wake);
    // Calling micropython from ISR
    // See: https://github.com/micropython/micropython/issues/4895
    void *old_state = mp_thread_get_state();
    int stack_top;

    mp_state_thread_t ts; // local thread state for the ISR
    mp_thread_set_state(&ts);
    mp_stack_set_top((void *) &stack_top); // need to include in root-pointer scan
    mp_stack_set_limit(2 * 8192); // tune based on ISR thread stack size
    mp_locals_set(mp_state_ctx.thread.dict_locals); // use main thread's locals
    mp_globals_set(mp_state_ctx.thread.dict_globals); // use main thread's globals

    mp_sched_lock(); // prevent VM from switching to another MicroPython thread
    gc_lock(); // prevent memory allocation

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_call_function_n_kw(cb, 0, 0, NULL);
        nlr_pop();
    } else {
        mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));  // changed to &mp_plat_print to fit this context
    }

    gc_unlock();
    mp_sched_unlock();

    mp_thread_set_state(old_state);
}


static void* sw_rotate_task(void* self_in)
{
    mp_lcd_sw_rotate_task(self_in);
    return NULL;
}


bool mp_lcd_start_rotate_task(void *self_in)
{
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)self_in;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&self->sw_rot.handles.task_handle, &attr, &sw_rotate_task, NULL);

    mp_lcd_lock_acquire(&self->sw_rot.handles.init_lock);
    mp_lcd_lock_release(&self->sw_rot.handles.init_lock);
    mp_lcd_lock_delete(&self->sw_rot.handles.init_lock);

    if (self->sw_rot.init.err != LCD_OK) {
        mp_raise_msg_varg(&mp_type_OSError, self->sw_rot.init.err_msg, self->sw_rot.init.err);
        return false;
    } else {
        return true;
    }
}
