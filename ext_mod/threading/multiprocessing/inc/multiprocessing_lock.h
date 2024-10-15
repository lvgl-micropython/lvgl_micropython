// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "thread_common.h"
#include "thread_lock.h"

#ifndef __MULTIPROCESSING_LOCK_H__
    #define __MULTIPROCESSING_LOCK_H__

    extern const mp_obj_type_t mp_type_multiprocessing_lock_t;

#endif