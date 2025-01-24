#include "sw_rotate_task.h"

#ifndef __SW_ROTATE_TASK_COMMON_H__
    #define __SW_ROTATE_TASK_COMMON_H__

    typedef struct _mp_lcd_lock_t mp_lcd_lock_t;
    typedef struct _mp_lcd_event_t mp_lcd_event_t;

    /* event */
    void mp_lcd_event_init(mp_lcd_event_t *event);
    void mp_lcd_event_delete(mp_lcd_event_t *event);
    bool mp_lcd_event_isset(mp_lcd_event_t *event);
    void mp_lcd_event_set(mp_lcd_event_t *event);
    void mp_lcd_event_clear(mp_lcd_event_t *event);
    void mp_lcd_event_clear_from_isr(mp_lcd_event_t *event);
    bool mp_lcd_event_isset_from_isr(mp_lcd_event_t *event);
    void mp_lcd_event_set_from_isr(mp_lcd_event_t *event);
    void mp_lcd_event_wait(mp_lcd_event_t *event);

    /* lock */
    bool mp_lcd_lock_acquire(mp_lcd_lock_t *lock);
    void mp_lcd_lock_release(mp_lcd_lock_t *lock);
    void mp_lcd_lock_init(mp_lcd_lock_t *lock);
    void mp_lcd_lock_delete(mp_lcd_lock_t *lock);
    void mp_lcd_lock_release_from_isr(mp_lcd_lock_t *lock);


    bool mp_lcd_start_rotate_task(void *self_in);

#endif /* __SW_ROTATE_TASK_COMMON_H__ */