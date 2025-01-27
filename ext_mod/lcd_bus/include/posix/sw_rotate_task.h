
#include <pthread.h>
#include <stdint.h>


#ifndef __SW_ROTATE_TASK_H__
    #define __SW_ROTATE_TASK_H__

    #define mp_lcd_task_handle_t  pthread_t
    
    struct _mp_lcd_lock_t {
        pthread_mutex_t handle;
        pthread_mutexattr_t attr;
    };

    struct _mp_lcd_event_t {
        pthread_cond_t handle;
        struct _mp_lcd_lock_t event_lock;
        uint8_t state;
        pthread_condattr_t attr;
    };

#endif /* __SW_ROTATE_TASK_H__ */
