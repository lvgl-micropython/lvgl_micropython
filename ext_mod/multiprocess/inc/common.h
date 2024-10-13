#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#ifndef __COMMON_H__

    #define __COMMON_H__

    typedef struct _mp_mutex_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    } mp_mutex_t;

    int lock_acquire(mp_mutex_t *mutex, int wait);
    void lock_release(mp_mutex_t *mutex);
    void lock_init(mp_mutex_t *mutex);

    int rlock_acquire(mp_mutex_t *mutex, int wait);
    void rlock_release(mp_mutex_t *mutex);
    void rlock_init(mp_mutex_t *mutex);


#endif