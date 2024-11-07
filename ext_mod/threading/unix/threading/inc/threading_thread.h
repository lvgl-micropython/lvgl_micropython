// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "thread_thread.h"


#ifndef __THREADING_THREAD_H__
    #define __THREADING_THREAD_H__

    struct _threading_mutex_t {
        pthread_mutex_t handle;
    };

    extern const mp_obj_type_t mp_type_threading_thread_t;

#endif