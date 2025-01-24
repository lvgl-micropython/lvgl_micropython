#ifndef __SW_ROTATE_TASK_H__
    #define __SW_ROTATE_TASK_H__

    #define mp_lcd_task_handle_t  TaskHandle_t
    
    typedef struct _mp_lcd_lock_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    } mp_lcd_lock_t;

    typedef struct _mp_lcd_event_t {
        EventGroupHandle_t handle;
        StaticEventGroup_t buffer;
    } mp_lcd_event_t;

#endif /* __SW_ROTATE_TASK_H__ */