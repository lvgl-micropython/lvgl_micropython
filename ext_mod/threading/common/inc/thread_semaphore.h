// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#ifndef __THREAD_SEMAPHORE_H__
    #define __THREAD_SEMAPHORE_H__

    #include "thread_common.h"

    typedef struct _mp_obj_thread_semaphore_t {
        mp_obj_base_t base;
        threading_mutex_t mutex;
        uint16_t start_value;
        volatile uint16_t value;
        volatile uint16_t waiting;
    } mp_obj_thread_semaphore_t;


    void semaphore_attr_func(mp_obj_t self_in, qstr attr, mp_obj_t *dest);

    extern const mp_obj_fun_builtin_fixed_t thread_semaphore__del__obj;
    extern const mp_obj_fun_builtin_var_t thread_semaphore_acquire_obj;
    extern const mp_obj_fun_builtin_var_t thread_semaphore__enter__obj;
    extern const mp_obj_fun_builtin_var_t thread_semaphore_release_obj;
    extern const mp_obj_fun_builtin_var_t thread_semaphore__exit__obj;

#endif