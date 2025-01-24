
#include <pthread.h>

#ifndef __SW_ROTATE_TASK_H__
    #define __SW_ROTATE_TASK_H__

    #define mp_lcd_task_handle_t  pthread_t
    
    struct _mp_lcd_lock_t {
        pthread_mutex_t handle;
        pthread_mutexattr_t attr;
    };

    struct _mp_lcd_event_t {
        pthread_cond_t handle;
        mp_lcd_lock_t event_lock;
        uint8_t state;
        pthread_condattr_t attr;
    };

#endif /* __SW_ROTATE_TASK_H__ */


pthread_atfork(3),


pthread_cleanup_push(3),

pthread_cond_signal(3),
pthread_cond_wait(3),

pthread_attr_init(3),
pthread_cancel(3),
pthread_create(3),
pthread_detach(3),
pthread_equal(3),
pthread_exit(3),
pthread_key_create(3),
pthread_kill(3),


pthread_mutex_lock(3),
pthread_mutex_unlock(3),
pthread_mutexattr_destroy(3),
pthread_mutexattr_init(3),

pthread_once(3),

pthread_rwlockattr_setkind_np(3),
pthread_setcancelstate(3),
pthread_setcanceltype(3),
pthread_setspecific(3),
pthread_sigmask(3),
pthread_sigqueue(3),
pthread_testcancel(3)


pthread_spin_init(3),
pthread_spin_lock(3),