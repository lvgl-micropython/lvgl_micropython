// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "thread_common.h"
#include "thread_lock.h"

#ifndef __THREADING_LOCK_H__
    #define __THREADING_LOCK_H__

    extern const mp_obj_type_t mp_type_threading_lock_t;

#endif