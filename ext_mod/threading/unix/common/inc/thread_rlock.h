// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "thread_common.h"

#ifndef __THREAD_RLOCK_H__
    #define __THREAD_RLOCK_H__

    typedef struct _mp_obj_thread_rlock_t {
        mp_obj_base_t base;
        threading_mutex_t mutex;
        volatile bool locked;
        volatile int count;
    } mp_obj_thread_rlock_t;

    extern const mp_obj_fun_builtin_fixed_t thread_rlock_locked_obj;
    extern const mp_obj_fun_builtin_fixed_t thread_rlock_release_obj;
    extern const mp_obj_fun_builtin_var_t thread_rlock__exit__obj;
    extern const mp_obj_fun_builtin_var_t thread_rlock_acquire_obj;
    extern const mp_obj_fun_builtin_var_t thread_rlock__enter__obj;

#endif