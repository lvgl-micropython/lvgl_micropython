#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"


#ifndef __THREAD_PORT_H__
    #define __THREAD_PORT_H__

    struct _thread_lock_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    };

    struct _thread_rlock_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    };

    struct _thread_semaphore_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    };

    struct _thread_event_t {
        EventGroupHandle_t handle;
        StaticEventGroup_t buffer;
    };

    struct _thread_t {
        TaskHandle_t handle;
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