// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "thread_common.h"

#ifndef __THREAD_RLOCK_H__
    #define __THREAD_RLOCK_H__

    typedef struct _thread_rlock_t thread_rlock_t;

    typedef struct _mp_obj_rlock_t {
        mp_obj_base_t base;
        thread_rlock_t rlock;
        volatile bool locked;
        volatile int count;
    } mp_obj_rlock_t;

    int threading_rlock_acquire(thread_rlock_t *rlock, int32_t wait_ms);
    void threading_rlock_release(thread_rlock_t *rlock);
    void threading_rlock_init(thread_rlock_t *rlock);
    void threading_rlock_delete(thread_rlock_t *rlock);

#endif