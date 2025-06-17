#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/atomic.h"


#ifndef __ATOMIC_H__
    #define __ATOMIC_H__

    extern const mp_obj_fun_builtin_fixed_t mp_ATOMIC_ENTER_CRITICAL_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_ATOMIC_EXIT_CRITICAL_obj;

#endif



