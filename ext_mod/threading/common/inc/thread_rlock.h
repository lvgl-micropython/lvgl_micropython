// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "thread_common.h"

#ifndef __THREAD_RLOCK_H__
    #define __THREAD_RLOCK_H__

    typedef struct _thread_rlock_t thread_rlock_t; // needs to be defined in port

    typedef struct _mp_obj_rlock_t {
        mp_obj_base_t base;
        thread_rlock_t rlock;
        volatile bool locked;
        volatile int count;
    } mp_obj_rlock_t;

    int threading_rlock_acquire(thread_rlock_t *rlock, int32_t wait_ms); // needs to be defined in port
    void threading_rlock_release(thread_rlock_t *rlock); // needs to be defined in port
    void threading_rlock_init(thread_rlock_t *rlock); // needs to be defined in port
    void threading_rlock_delete(thread_rlock_t *rlock); // needs to be defined in port

    extern const mp_obj_type_t mp_type_threading_rlock_t;
    extern const mp_obj_type_t mp_type_multiprocessing_rlock_t;

#endif