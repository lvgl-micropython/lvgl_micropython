#include <pthread.h>

#include <semaphore.h>


#ifndef __THREAD_PORT_H__
    #define __THREAD_PORT_H__

    struct _thread_lock_t {
        pthread_mutex_t handle;
        volatile uint16_t ref_count;
    };

    struct _thread_rlock_t {
        pthread_mutex_t handle;
        volatile uint16_t ref_count;
    };

    struct _thread_semaphore_t {
        sem_t handle;
        volatile uint16_t ref_count;
    };

    struct _thread_event_t {
        sem_t handle;
        volatile uint16_t ref_count;
        volatile bool is_set;
    };

    struct _thread_t {
        pthread_t handle;
    }

    void THREADING_FREERTOS_TASK_DELETE_HOOK(void *tcb);

    #include "threading.h"
    #include "multiprocessing.h"

    #include "thread_thread.h"
    #include "thread_event.h"
    #include "thread_semaphore.h"
    #include "thread_lock.h"
    #include "thread_rlock.h"

#endif /* __THREAD_PORT_H__ */