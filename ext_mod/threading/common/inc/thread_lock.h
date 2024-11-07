// micropython includes
#include "py/obj.h"
#include "py/runtime.h"


#ifndef __THREAD_LOCK_H__
    #define __THREAD_LOCK_H__

    #include "threading_common.h"

    typedef struct _thread_lock_t thread_lock_t;

    typedef struct _mp_obj_lock_t {
        mp_obj_base_t base;
        thread_lock_t lock;
        volatile bool locked;
    } mp_obj_lock_t;

    int threading_lock_acquire(thread_lock_t *lock, int32_t wait_ms);
    void threading_lock_release(thread_lock_t *lock);
    void threading_lock_init(thread_lock_t *lock);
    void threading_lock_delete(thread_lock_t *lock);

#endif