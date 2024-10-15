// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "thread_common.h"

#ifndef __THREAD_LOCK_H__
    #define __THREAD_LOCK_H__

    typedef struct _mp_obj_thread_lock_t {
        mp_obj_base_t base;
        threading_mutex_t mutex;
        volatile bool locked;
    } mp_obj_thread_lock_t;


    extern const mp_obj_fun_builtin_fixed_t thread_lock_locked_obj;
    extern const mp_obj_fun_builtin_fixed_t thread_lock_release_obj;
    extern const mp_obj_fun_builtin_var_t thread_lock__exit__obj;
    extern const mp_obj_fun_builtin_var_t thread_lock_acquire_obj;
    extern const mp_obj_fun_builtin_var_t thread_lock__enter__obj;


#endif