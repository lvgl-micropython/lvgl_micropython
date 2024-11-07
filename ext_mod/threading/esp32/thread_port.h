#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"


#ifndef __THREAD_PORT_H__
    #define __THREAD_PORT_H__

    #include "threading.h"
    #include "multiprocessing.h"

    #include "thread_thread.h"
    #include "thread_event.h"
    #include "thread_semphamore.h"
    #include "thread_lock.h"
    #include "thread_rlock.h"

    struct _thread_lock_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    };

    struct _thread_rlock_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    };

    struct _thread_semphamore_t {
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

#endif /* __THREAD_PORT_H__ */