// micropython includes
#include "py/obj.h"
#include "py/runtime.h"


#ifndef __THREAD_SEMAPHORE_H__
    #define __THREAD_SEMAPHORE_H__

    typedef struct _thread_semaphore_t thread_semaphore_t; // needs to be defined in port

    typedef struct _mp_obj_thread_semaphore_t {
        mp_obj_base_t base;
        thread_semaphore_t sem;
        uint16_t start_value;
    } mp_obj_thread_semaphore_t;

    uint16_t threading_semaphore_get_count(thread_semaphore_t *sem); // needs to be defined in port
    bool threading_semaphore_acquire(thread_semaphore_t *sem, int32_t wait_ms); // needs to be defined in port
    void threading_semaphore_release(thread_semaphore_t *sem); // needs to be defined in port
    void threading_semaphore_init(thread_semaphore_t *sem); // needs to be defined in port
    void threading_semaphore_delete(thread_semaphore_t *sem); // needs to be defined in port

    extern const mp_obj_type_t mp_type_threading_semaphore_t;
    extern const mp_obj_type_t mp_type_multiprocessing_semaphore_t;

#endif