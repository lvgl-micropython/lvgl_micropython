#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#ifndef __SEMPHR_H__
    #define __SEMPHR_H__

    typedef struct _freertos_semaphore_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    } freertos_semaphore_t;

    typedef struct _mp_obj_freertos_semaphore_t {
        freertos_semaphore_t semaphore;
        mp_freertos_types type;
    } mp_obj_freertos_semaphore_t;

    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreCreateBinaryStatic_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreTake_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreTakeRecursive_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreGive_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreGiveRecursive_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreGiveFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreTakeFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreCreateMutexStatic_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreCreateRecursiveMutexStatic_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreCreateCountingStatic_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxSemaphoreGetCount_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxSemaphoreGetCountFromISR_obj;

#endif