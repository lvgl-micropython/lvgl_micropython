#ifdef ESP_IDF_VERSION
    #include "esp32/sw_rotate_task.h"
#elif defined(MP_PORT_UNIX)
    #include "posix/sw_rotate_task.h"
#else
    #include "other_mcus/sw_rotate_task.h"


#ifndef __SW_ROTATE_TASK_H__
    #define __SW_ROTATE_TASK_H__

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
    bool mp_lcd_lock_acquire(mp_lcd_lock_t *lock, int32_t wait_ms);
    void mp_lcd_lock_release(mp_lcd_lock_t *lock);
    void mp_lcd_lock_init(mp_lcd_lock_t *lock);
    void mp_lcd_lock_delete(mp_lcd_lock_t *lock);
    void mp_lcd_lock_release_from_isr(mp_lcd_lock_t *lock);


    void mp_lcd_sw_rotate_task(void *self_in);

#endif /* __SW_ROTATE_TASK_H__ */