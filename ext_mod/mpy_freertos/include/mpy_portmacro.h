#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#ifndef __PORTMACRO_H__
    #define __PORTMACRO_H__

    typedef struct _freertos_spinlock_t {
        spinlock_t *handle;
        void *buffer;
    } freertos_spinlock_t;

    typedef struct _mp_obj_freertos_spinlock_t {
        freertos_spinlock_t spinlock;
        mp_freertos_types type;
    } mp_obj_freertos_spinlock_t;

    extern const mp_obj_fun_builtin_fixed_t mp_portNOP_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xPortInIsrContext_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vPortAssertIfInISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xPortInterruptedFromISRContext_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xPortSetInterruptMaskFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vPortClearInterruptMaskFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portMUX_INITIALIZE_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xPortEnterCriticalTimeout_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vPortEnterCritical_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vPortExitCritical_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xPortEnterCriticalTimeoutCompliance_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vPortEnterCriticalCompliance_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vPortExitCriticalCompliance_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xPortEnterCriticalTimeoutSafe_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vPortEnterCriticalSafe_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vPortExitCriticalSafe_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vPortYield_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vPortYieldOtherCore_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xPortGetTickRateHz_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xPortGetCoreID_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portGET_CORE_ID_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portDISABLE_INTERRUPTS_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portENABLE_INTERRUPTS_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portSET_INTERRUPT_MASK_FROM_ISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portCLEAR_INTERRUPT_MASK_FROM_ISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portASSERT_IF_IN_ISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portCHECK_IF_IN_ISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portTRY_ENTER_CRITICAL_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portENTER_CRITICAL_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portEXIT_CRITICAL_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portTRY_ENTER_CRITICAL_ISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portENTER_CRITICAL_ISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portEXIT_CRITICAL_ISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portTRY_ENTER_CRITICAL_SAFE_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portENTER_CRITICAL_SAFE_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portEXIT_CRITICAL_SAFE_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portYIELD_obj;
    extern const mp_obj_fun_builtin_fixed_t mp__frxt_setup_switch_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portYIELD_FROM_ISR_NO_ARG_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portYIELD_FROM_ISR_ARG_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portYIELD_WITHIN_API_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portYIELD_CORE_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portGET_RUN_TIME_COUNTER_VALUE_obj;

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
    extern const mp_obj_fun_builtin_fixed_t mp_portRECORD_READY_PRIORITY_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portRESET_READY_PRIORITY_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_portGET_HIGHEST_PRIORITY_obj;
#endif

    extern const mp_obj_fun_builtin_fixed_t mp_os_task_switch_is_pended_obj;

#endif