// micropython includes
#include "py/obj.h"
#include "py/runtime.h"


#ifndef __THREAD_LOCK_H__
    #define __THREAD_LOCK_H__

    #include "thread_port.h"

    typedef struct _mp_obj_lock_t {
        mp_obj_base_t base;
        thread_lock_t lock;
        volatile bool locked;
    } mp_obj_lock_t;

    int threading_lock_acquire(thread_lock_t *lock, int32_t wait_ms); // needs to be defined in port
    void threading_lock_release(thread_lock_t *lock); // needs to be defined in port
    void threading_lock_init(thread_lock_t *lock); // needs to be defined in port
    void threading_lock_delete(thread_lock_t *lock); // needs to be defined in port

    extern const mp_obj_type_t mp_type_threading_lock_t;
    extern const mp_obj_type_t mp_type_multiprocessing_lock_t;


#endif