
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


#ifndef __TASK__H__
    #define __TASH__H__

    typedef struct _freertos_task_t {
        TaskHandle_t handle;
        StaticTask_t buffer;
    } freertos_task_t;


    typedef struct _mp_obj_freertos_task_t {
        freertos_task_t task;
        mp_freertos_types type;
        mp_obj_t callback;
        mp_obj_t pvParameters;
        StackType_t *puxStackBuffer;
    } mp_obj_freertos_task_t;

    void xTask_cb(void *self_in);

    extern const mp_obj_fun_builtin_fixed_t mp_taskVALID_CORE_ID_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_taskYIELD_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_taskENTER_CRITICAL_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_taskENTER_CRITICAL_FROM_ISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_taskENTER_CRITICAL_ISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_taskEXIT_CRITICAL_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_taskEXIT_CRITICAL_FROM_ISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_taskEXIT_CRITICAL_ISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_taskDISABLE_INTERRUPTS_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_taskENABLE_INTERRUPTS_obj;
    extern const mp_obj_fun_builtin_var_t mp_xTaskCreateStatic_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskCreateRestrictedStatic_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskAllocateMPURegions_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskDelete_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskDelay_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskDelayUntil_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskDelayUntil_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskAbortDelay_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxTaskPriorityGet_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxTaskPriorityGetFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_eTaskGetState_obj;
    extern const mp_obj_fun_builtin_var_t mp_vTaskGetInfo_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskPrioritySet_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskSuspend_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskResume_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskResumeFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskStartScheduler_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskEndScheduler_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskSuspendAll_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskResumeAll_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskGetTickCount_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskGetTickCountFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxTaskGetNumberOfTasks_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_pcTaskGetName_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskGetHandle_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskGetStaticBuffers_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxTaskGetStackHighWaterMark_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxTaskGetStackHighWaterMark2_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskSetApplicationTaskTag_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskGetApplicationTaskTag_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskGetApplicationTaskTagFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskSetThreadLocalStoragePointer_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_pvTaskGetThreadLocalStoragePointer_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vApplicationStackOverflowHook_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vApplicationTickHook_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vApplicationGetIdleTaskMemory_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskGetIdleTaskHandle_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxTaskGetSystemState_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskGetRunTimeStats_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_ulTaskGetIdleRunTimeCounter_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_ulTaskGetIdleRunTimePercent_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskSetTimeOutState_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskCheckForTimeOut_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskCatchUpTicks_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskIncrementTick_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskPlaceOnEventList_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskPlaceOnUnorderedEventList_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskPlaceOnEventListRestricted_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskRemoveFromEventList_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskRemoveFromUnorderedEventList_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskSwitchContext_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxTaskResetEventItemValue_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskGetCurrentTaskHandle_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskMissedYield_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskGetSchedulerState_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskPriorityInherit_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskPriorityDisinherit_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskPriorityDisinheritAfterTimeout_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_pvTaskIncrementMutexHeldCount_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskInternalSetTimeOutState_obj;



#endif