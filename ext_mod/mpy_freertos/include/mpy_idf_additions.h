#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"

#ifndef __IDF_ADDITIONS_H__
    #define __IDF_ADDITIONS_H__

    extern const mp_obj_fun_builtin_var_t mp_xTaskCreateStaticPinnedToCore_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskGetCoreID_obj;

#if !CONFIG_FREERTOS_SMP
#if INCLUDE_xTaskGetIdleTaskHandle == 1
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskGetIdleTaskHandleForCore_obj;
#endif
#if INCLUDE_xTaskGetIdleTaskHandle == 1 || configUSE_MUTEXES == 1
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskGetCurrentTaskHandleForCore_obj;
#endif
#endif

    extern const mp_obj_fun_builtin_var_t mp_xTaskCreatePinnedToCoreWithCaps_obj;
    extern const mp_obj_fun_builtin_var_t mp_xTaskCreateWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskDeleteWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueCreateWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vQueueDeleteWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreCreateBinaryWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreCreateCountingWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreCreateMutexWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xSemaphoreCreateRecursiveMutexWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vSemaphoreDeleteWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferCreateWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vStreamBufferDeleteWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xMessageBufferCreateWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vMessageBufferDeleteWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xEventGroupCreateWithCaps_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vEventGroupDeleteWithCaps_obj;

#endif