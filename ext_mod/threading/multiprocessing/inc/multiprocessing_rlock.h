// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "thread_common.h"
#include "thread_rlock.h"


#ifndef __THREADING_RLOCK_H__
    #define __THREADING_RLOCK_H__

    extern const mp_obj_type_t mp_type_multiprocessing_rlock_t;

#endif