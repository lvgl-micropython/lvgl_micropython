
#include <pthread.h>

#include "common/lcd_common_types.h"
#include "common/sw_rotate.h"
#include "common/sw_rotate_task_common.h"
#include "sw_rotate_task.h"

#include "lcd_types.h"


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


static void* sw_rotate_task(void* self_in)
{
    mp_lcd_sw_rotate_task(self_in);
    return NULL;
}


bool mp_lcd_start_rotate_task(void *self_in)
{
    mp_lcd_obj_t *self = (mp_lcd_obj_t *)self_in;

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
