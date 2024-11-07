// micropython includes
#include "py/obj.h"
#include "py/runtime.h"


#ifndef __THREAD_SEMAPHORE_H__
    #define __THREAD_SEMAPHORE_H__

    #include "threading_common.h"

    typedef struct _thread_semphamore_t thread_semphamore_t;

    typedef struct _mp_obj_thread_semaphore_t {
        mp_obj_base_t base;
        thread_semphamore_t sem;
        uint16_t start_value;
        volatile uint16_t value;
        volatile uint16_t waiting;
    } mp_obj_thread_semaphore_t;


    uint16_t threading_semphamore_get_count(thread_semphamore_t *sem);
    bool threading_semphamore_acquire(thread_semphamore_t *sem, int32_t wait_ms);
    void threading_semphamore_release(thread_semphamore_t *sem);
    void threading_semphamore_init(thread_event_t *mutex);
    void threading_semphamore_delete(thread_event_t *mutex);

#endif