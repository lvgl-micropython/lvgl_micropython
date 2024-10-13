// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "common.h"


#ifndef __THREADING_SEMAPHORE_H__
    #define __THREADING_SEMAPHORE_H__

    typedef struct _mp_obj_threading_semaphore_t {
        mp_obj_base_t base;
        threading_mutex_t mutex;
        uint16_t start_value;
        volatile uint16_t value;
        volatile uint16_t waiting;
    } mp_obj_threading_semaphore_t;

    extern const mp_obj_type_t mp_type_threading_semaphore_t;


#endif