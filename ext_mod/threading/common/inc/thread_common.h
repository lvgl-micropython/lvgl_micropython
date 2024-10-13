#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#ifndef __COMMON_H__

    #define __COMMON_H__

    #define THREAD_UNUSED(x) ((void)x)

    typedef struct _threading_mutex_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    } threading_mutex_t;

    int lock_acquire(threading_mutex_t *mutex, int wait);
    void lock_release(threading_mutex_t *mutex);
    void lock_init(threading_mutex_t *mutex);

    int rlock_acquire(threading_mutex_t *mutex, int wait);
    void rlock_release(threading_mutex_t *mutex);
    void rlock_init(threading_mutex_t *mutex);


#endif