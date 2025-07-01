
#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "mpy_task.h"
#include "mpy_semphr.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static mp_obj_t mp_taskVALID_CORE_ID(mp_obj_t xCoreID_in)
{
    BaseType_t xCoreID = (BaseType_t)mp_obj_get_int(xCoreID_in);

    BaseType_t ret = taskVALID_CORE_ID(xCoreID);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_taskVALID_CORE_ID_obj, mp_taskVALID_CORE_ID);


static mp_obj_t mp_taskYIELD(void)
{
    taskYIELD();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_taskYIELD_obj, mp_taskYIELD);


static mp_obj_t mp_taskENTER_CRITICAL(mp_obj_t x_in)
{
    mp_obj_freertos_semaphore_t *x = MP_OBJ_TO_PTR(x_in);

    // #define taskENTER_CRITICAL( x )            portENTER_CRITICAL( x )
    // #define portENTER_CRITICAL(mux)                     vPortEnterCritical(mux)
    // static inline void __attribute__((always_inline)) vPortEnterCritical(portMUX_TYPE *mux);

    taskENTER_CRITICAL((portMUX_TYPE *)x->semaphore.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_taskENTER_CRITICAL_obj, mp_taskENTER_CRITICAL);


static mp_obj_t mp_taskENTER_CRITICAL_FROM_ISR(void)
{
    taskENTER_CRITICAL_FROM_ISR();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_taskENTER_CRITICAL_FROM_ISR_obj,
                          mp_taskENTER_CRITICAL_FROM_ISR);


static mp_obj_t mp_taskENTER_CRITICAL_ISR(mp_obj_t x_in)
{
    mp_obj_freertos_semaphore_t *x = MP_OBJ_TO_PTR(x_in);

    // #define taskENTER_CRITICAL_ISR( x )        portENTER_CRITICAL_ISR( x )
    // #define portENTER_CRITICAL_ISR(mux)                 vPortEnterCritical(mux)
    // static inline void __attribute__((always_inline)) vPortEnterCriticalSafe(portMUX_TYPE *mux);

    taskENTER_CRITICAL_ISR((portMUX_TYPE *)x->semaphore.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_taskENTER_CRITICAL_ISR_obj, mp_taskENTER_CRITICAL_ISR);


static mp_obj_t mp_taskEXIT_CRITICAL(mp_obj_t x_in)
{
    mp_obj_freertos_semaphore_t *x = MP_OBJ_TO_PTR(x_in);

    // #define taskEXIT_CRITICAL( x )             portEXIT_CRITICAL( x )
    // #define portEXIT_CRITICAL(mux)                      vPortExitCritical(mux)
    // void vPortExitCritical(portMUX_TYPE *mux);

    taskEXIT_CRITICAL((portMUX_TYPE *)x->semaphore.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_taskEXIT_CRITICAL_obj, mp_taskEXIT_CRITICAL);


static mp_obj_t mp_taskEXIT_CRITICAL_FROM_ISR(mp_obj_t prev_level_in)
{
    UBaseType_t prev_level = (UBaseType_t)mp_obj_get_int_truncated(prev_level_in);

    // #define taskEXIT_CRITICAL_FROM_ISR( x )    portCLEAR_INTERRUPT_MASK_FROM_ISR( x )
    // #define portCLEAR_INTERRUPT_MASK_FROM_ISR(prev_level)       vPortClearInterruptMaskFromISR(prev_level)
    // static inline void vPortClearInterruptMaskFromISR(UBaseType_t prev_level);

    taskEXIT_CRITICAL_FROM_ISR(prev_level);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_taskEXIT_CRITICAL_FROM_ISR_obj,
                          mp_taskEXIT_CRITICAL_FROM_ISR);


static mp_obj_t mp_taskEXIT_CRITICAL_ISR(mp_obj_t x_in)
{
    mp_obj_freertos_semaphore_t *x = MP_OBJ_TO_PTR(x_in);

    // #define taskEXIT_CRITICAL_ISR( x )         portEXIT_CRITICAL_ISR( x )
    // #define portEXIT_CRITICAL_ISR(mux)                  vPortExitCritical(mux)
    // void vPortExitCritical(portMUX_TYPE *mux);

    taskEXIT_CRITICAL_ISR((portMUX_TYPE *)x->semaphore.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_taskEXIT_CRITICAL_ISR_obj, mp_taskEXIT_CRITICAL_ISR);


static mp_obj_t mp_taskDISABLE_INTERRUPTS(void)
{
    taskDISABLE_INTERRUPTS();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_taskDISABLE_INTERRUPTS_obj, mp_taskDISABLE_INTERRUPTS);


static mp_obj_t mp_taskENABLE_INTERRUPTS(void)
{
    taskENABLE_INTERRUPTS();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_taskENABLE_INTERRUPTS_obj, mp_taskENABLE_INTERRUPTS);


void xTask_cb(void *self_in)
{
    mp_obj_freertos_task_t *self = (mp_obj_freertos_task_t *)self_in;
    // callback
    mp_obj_t args[1] = { self->pvParameters };
    mp_call_function_n_kw(self->callback, 1, 0, &args[0]);
}


static mp_obj_t mp_xTaskCreateStatic(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    mp_obj_freertos_task_t *self = m_new_obj(mp_obj_freertos_task_t);
    self->type = mp_freertos_task_type;

    self->callback = args[0];

    size_t str_len;
    const char *pcName = mp_obj_str_get_data(args[1], &str_len);

    uint32_t ulStackDepth = (uint32_t)mp_obj_get_int_truncated(args[2]);
    self->pvParameters = args[3];

    UBaseType_t uxPriority = (UBaseType_t)mp_obj_get_int_truncated(args[4]);

    if (args[5] != mp_const_none) {
        self->puxStackBuffer = malloc(ulStackDepth * sizeof(StackType_t));

        mp_obj_tuple_t *puxStackBuffer = MP_OBJ_TO_PTR(args[5]);

        for (uint32_t i=0; i<ulStackDepth; i++) {
            StackType_t *stack_item = (StackType_t *)MP_OBJ_FROM_PTR(puxStackBuffer->items[i]);
            self->puxStackBuffer[i] = *stack_item;
        }
    } else {
        self->puxStackBuffer = NULL;
    }

    self->task.handle = xTaskCreateStatic(&xTask_cb, pcName, ulStackDepth, self,
                                          uxPriority, self->puxStackBuffer,
                                          &self->task.buffer);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_xTaskCreateStatic_obj, 6, 6, mp_xTaskCreateStatic);


// #if ( portUSING_MPU_WRAPPERS == 1 )
//     BaseType_t xTaskCreateRestrictedStatic(const TaskParameters_t * const pxTaskDefinition, TaskHandle_t * pxCreatedTask);
// #endif

// void vTaskAllocateMPURegions(TaskHandle_t xTask, const MemoryRegion_t * const pxRegions );

static mp_obj_t mp_vTaskDelete(mp_obj_t xTaskToDelete_in)
{
    if (xTaskToDelete_in == mp_const_none) {
        vTaskDelete(NULL);
    } else {
        mp_obj_freertos_task_t *xTaskToDelete = MP_OBJ_TO_PTR(xTaskToDelete_in);
        vTaskDelete(xTaskToDelete->task.handle);
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vTaskDelete_obj, mp_vTaskDelete);


static mp_obj_t mp_vTaskDelay(mp_obj_t xTicksToDelay_in)
{
    TickType_t xTicksToDelay = (TickType_t)mp_obj_get_int_truncated(xTicksToDelay_in);

    vTaskDelay(xTicksToDelay);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vTaskDelay_obj, mp_vTaskDelay);


static mp_obj_t mp_xTaskDelayUntil(mp_obj_t xTimeIncrement_in)
{
    TickType_t xTimeIncrement = (TickType_t)mp_obj_get_int_truncated(xTimeIncrement_in);
    TickType_t pxPreviousWakeTime = 0;

    BaseType_t ret = xTaskDelayUntil(&pxPreviousWakeTime, xTimeIncrement);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int_from_uint((uint32_t)pxPreviousWakeTime)
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTaskDelayUntil_obj, mp_xTaskDelayUntil);


static mp_obj_t mp_vTaskDelayUntil(mp_obj_t xTimeIncrement_in)
{
    TickType_t xTimeIncrement = (TickType_t)mp_obj_get_int_truncated(xTimeIncrement_in);
    TickType_t pxPreviousWakeTime = 0;

    vTaskDelayUntil(&pxPreviousWakeTime, xTimeIncrement);
    return mp_obj_new_int_from_uint((uint32_t)pxPreviousWakeTime);
}


MP_DEFINE_CONST_FUN_OBJ_1(mp_vTaskDelayUntil_obj, mp_vTaskDelayUntil);


static mp_obj_t mp_xTaskAbortDelay(mp_obj_t xTask_in)
{
    mp_obj_freertos_task_t *xTask = MP_OBJ_TO_PTR(xTask_in);

    BaseType_t ret = xTaskAbortDelay(xTask->task.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTaskAbortDelay_obj, mp_xTaskAbortDelay);


static mp_obj_t mp_uxTaskPriorityGet(mp_obj_t xTask_in)
{
    mp_obj_freertos_task_t *xTask = MP_OBJ_TO_PTR(xTask_in);

    UBaseType_t ret = uxTaskPriorityGet(xTask->task.handle);
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_uxTaskPriorityGet_obj, mp_uxTaskPriorityGet);


static mp_obj_t mp_uxTaskPriorityGetFromISR(mp_obj_t xTask_in)
{
    mp_obj_freertos_task_t *xTask = MP_OBJ_TO_PTR(xTask_in);

    UBaseType_t ret = uxTaskPriorityGetFromISR(xTask->task.handle);
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_uxTaskPriorityGetFromISR_obj, mp_uxTaskPriorityGetFromISR);


static mp_obj_t mp_eTaskGetState(mp_obj_t xTask_in)
{
    mp_obj_freertos_task_t *xTask = MP_OBJ_TO_PTR(xTask_in);

    eTaskState ret = eTaskGetState(xTask->task.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_eTaskGetState_obj, mp_eTaskGetState);


// void vTaskGetInfo(TaskHandle_t xTask, TaskStatus_t * pxTaskStatus, BaseType_t xGetFreeStackSpace, eTaskState eState );

static mp_obj_t mp_vTaskPrioritySet(mp_obj_t xTask_in, mp_obj_t uxNewPriority_in)
{
    mp_obj_freertos_task_t *xTask = MP_OBJ_TO_PTR(xTask_in);
    UBaseType_t uxNewPriority = (UBaseType_t)mp_obj_get_int_truncated(uxNewPriority_in);

    vTaskPrioritySet(xTask->task.handle, uxNewPriority);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_vTaskPrioritySet_obj, mp_vTaskPrioritySet);


static mp_obj_t mp_vTaskSuspend(mp_obj_t xTaskToSuspend_in)
{
    mp_obj_freertos_task_t *xTaskToSuspend = MP_OBJ_TO_PTR(xTaskToSuspend_in);

    vTaskSuspend(xTaskToSuspend->task.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vTaskSuspend_obj, mp_vTaskSuspend);


static mp_obj_t mp_vTaskResume(mp_obj_t xTaskToResume_in)
{
    mp_obj_freertos_task_t *xTaskToResume = MP_OBJ_TO_PTR(xTaskToResume_in);

    vTaskResume(xTaskToResume->task.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vTaskResume_obj, mp_vTaskResume);


static mp_obj_t mp_xTaskResumeFromISR(mp_obj_t xTaskToResume_in)
{
    mp_obj_freertos_task_t *xTaskToResume = MP_OBJ_TO_PTR(xTaskToResume_in);

    BaseType_t ret = xTaskResumeFromISR(xTaskToResume->task.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTaskResumeFromISR_obj, mp_xTaskResumeFromISR);


static mp_obj_t mp_vTaskStartScheduler(void)
{
    vTaskStartScheduler();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_vTaskStartScheduler_obj, mp_vTaskStartScheduler);


static mp_obj_t mp_vTaskEndScheduler(void)
{
    vTaskEndScheduler();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_vTaskEndScheduler_obj, mp_vTaskEndScheduler);


static mp_obj_t mp_vTaskSuspendAll(void)
{
    vTaskSuspendAll();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_vTaskSuspendAll_obj, mp_vTaskSuspendAll);


static mp_obj_t mp_xTaskResumeAll(void)
{
    BaseType_t ret = xTaskResumeAll();
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xTaskResumeAll_obj, mp_xTaskResumeAll);


static mp_obj_t mp_xTaskGetTickCount(void)
{
    TickType_t ret = xTaskGetTickCount();
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xTaskGetTickCount_obj, mp_xTaskGetTickCount);


static mp_obj_t mp_xTaskGetTickCountFromISR(void)
{
    TickType_t ret = xTaskGetTickCountFromISR();
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xTaskGetTickCountFromISR_obj, mp_xTaskGetTickCountFromISR);


static mp_obj_t mp_uxTaskGetNumberOfTasks(void)
{
    UBaseType_t ret = uxTaskGetNumberOfTasks();
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_uxTaskGetNumberOfTasks_obj, mp_uxTaskGetNumberOfTasks);


static mp_obj_t mp_pcTaskGetName(mp_obj_t xTaskToQuery_in)
{
    mp_obj_freertos_task_t *xTaskToQuery = MP_OBJ_TO_PTR(xTaskToQuery_in);

    const char *ret = pcTaskGetName(xTaskToQuery->task.handle);
    return mp_obj_new_str_from_cstr(ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_pcTaskGetName_obj, mp_pcTaskGetName);

// TaskHandle_t xTaskGetHandle(const char * pcNameToQuery);

// BaseType_t xTaskGetStaticBuffers(TaskHandle_t xTask, StackType_t ** ppuxStackBuffer, StaticTask_t ** ppxTaskBuffer );

static mp_obj_t mp_uxTaskGetStackHighWaterMark(mp_obj_t xTask_in)
{
    mp_obj_freertos_task_t *xTask = MP_OBJ_TO_PTR(xTask_in);

    UBaseType_t ret = uxTaskGetStackHighWaterMark(xTask->task.handle);
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_uxTaskGetStackHighWaterMark_obj, mp_uxTaskGetStackHighWaterMark);


// configSTACK_DEPTH_TYPE uxTaskGetStackHighWaterMark2(TaskHandle_t xTask);

// #ifdef configUSE_APPLICATION_TASK_TAG
// #if configUSE_APPLICATION_TASK_TAG == 1
//     void vTaskSetApplicationTaskTag(TaskHandle_t xTask, TaskHookFunction_t pxHookFunction);
//     TaskHookFunction_t xTaskGetApplicationTaskTag(TaskHandle_t xTask);
//     TaskHookFunction_t xTaskGetApplicationTaskTagFromISR(TaskHandle_t xTask);
// #endif /* configUSE_APPLICATION_TASK_TAG ==1 */
// #endif /* ifdef configUSE_APPLICATION_TASK_TAG */


#if (configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0)

    static mp_obj_t mp_vTaskSetThreadLocalStoragePointer(mp_obj_t xTaskToSet_in,
                                                         mp_obj_t xIndex_in,
                                                         mp_obj_t pvValue_in)
    {
        mp_obj_freertos_task_t *xTaskToSet = MP_OBJ_TO_PTR(xTaskToSet_in);
        BaseType_t xIndex = (BaseType_t)mp_obj_get_int(xIndex_in);

        vTaskSetThreadLocalStoragePointer(xTaskToSet->task.handle, xIndex, 
                                          (void *)pvValue_in);
        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_3(mp_vTaskSetThreadLocalStoragePointer_obj,
                              mp_vTaskSetThreadLocalStoragePointer);


    static mp_obj_t mp_pvTaskGetThreadLocalStoragePointer(mp_obj_t xTaskToQuery_in,
                                                          mp_obj_t xIndex_in)
    {
        mp_obj_freertos_task_t *xTaskToQuery = MP_OBJ_TO_PTR(xTaskToQuery_in);
        BaseType_t xIndex = (BaseType_t)mp_obj_get_int(xIndex_in);

        mp_obj_t ret = (mp_obj_t)pvTaskGetThreadLocalStoragePointer(xTaskToQuery->task.handle,
                                                                    xIndex);
        return ret;
    }

    MP_DEFINE_CONST_FUN_OBJ_2(mp_pvTaskGetThreadLocalStoragePointer_obj,
                              mp_pvTaskGetThreadLocalStoragePointer);

#endif


#if (configCHECK_FOR_STACK_OVERFLOW > 0)

    static mp_obj_t mp_vApplicationStackOverflowHook(mp_obj_t xTask_in,
                                                     mp_obj_t pcTaskName_in)
    {
        mp_obj_freertos_task_t *xTask = MP_OBJ_TO_PTR(xTask_in);

        size_t str_len;
        const char *pcTaskName = mp_obj_str_get_data(pcTaskName_in, &str_len);

        vApplicationStackOverflowHook(xTask->task.handle, (char *)pcTaskName);
        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_2(mp_vApplicationStackOverflowHook_obj,
                              mp_vApplicationStackOverflowHook);

#endif


#if (configUSE_TICK_HOOK > 0)
    void vApplicationTickHook(void);

    static mp_obj_t mp_vApplicationTickHook(void)
    {
        vApplicationTickHook();
        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_0(mp_vApplicationTickHook_obj, mp_vApplicationTickHook);

#endif


// void vApplicationGetIdleTaskMemory(StaticTask_t ** ppxIdleTaskTCBBuffer, StackType_t ** ppxIdleTaskStackBuffer, uint32_t * pulIdleTaskStackSize );

// TaskHandle_t xTaskGetIdleTaskHandle(void);

// UBaseType_t uxTaskGetSystemState(TaskStatus_t * const pxTaskStatusArray, const UBaseType_t uxArraySize, configRUN_TIME_COUNTER_TYPE * const pulTotalRunTime);


// void vTaskGetRunTimeStats(char * pcWriteBuffer);

// configRUN_TIME_COUNTER_TYPE ulTaskGetIdleRunTimeCounter(void);

// configRUN_TIME_COUNTER_TYPE ulTaskGetIdleRunTimePercent(void);

// void vTaskSetTimeOutState(TimeOut_t * const pxTimeOut);

// BaseType_t xTaskCheckForTimeOut(TimeOut_t * const pxTimeOut, TickType_t * const pxTicksToWait);

static mp_obj_t mp_xTaskCatchUpTicks(mp_obj_t xTicksToCatchUp_in)
{
    TickType_t xTicksToCatchUp = (TickType_t)mp_obj_get_int_truncated(xTicksToCatchUp_in);

    BaseType_t ret = xTaskCatchUpTicks(xTicksToCatchUp);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTaskCatchUpTicks_obj, mp_xTaskCatchUpTicks);


static mp_obj_t mp_xTaskIncrementTick(void)
{
    BaseType_t ret = xTaskIncrementTick();
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xTaskIncrementTick_obj, mp_xTaskIncrementTick);


// void vTaskPlaceOnEventList(List_t * const pxEventList, const TickType_t xTicksToWait);

// void vTaskPlaceOnUnorderedEventList(List_t * pxEventList, const TickType_t xItemValue, const TickType_t xTicksToWait);

// void vTaskPlaceOnEventListRestricted(List_t * const pxEventList, TickType_t xTicksToWait, const BaseType_t xWaitIndefinitely);


// BaseType_t xTaskRemoveFromEventList(const List_t * const pxEventList);

// void vTaskRemoveFromUnorderedEventList(ListItem_t * pxEventListItem, const TickType_t xItemValue);


static mp_obj_t mp_vTaskSwitchContext(void)
{
    vTaskSwitchContext();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_vTaskSwitchContext_obj, mp_vTaskSwitchContext);


// TickType_t uxTaskResetEventItemValue(void);

// TaskHandle_t xTaskGetCurrentTaskHandle(void);

static mp_obj_t mp_vTaskMissedYield(void)
{
    vTaskMissedYield();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_vTaskMissedYield_obj, mp_vTaskMissedYield);


static mp_obj_t mp_xTaskGetSchedulerState(void)
{
    BaseType_t ret = xTaskGetSchedulerState();
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xTaskGetSchedulerState_obj, mp_xTaskGetSchedulerState);


static mp_obj_t mp_xTaskPriorityInherit(mp_obj_t pxMutexHolder_in)
{
    mp_obj_freertos_task_t *pxMutexHolder = MP_OBJ_TO_PTR(pxMutexHolder_in);

    BaseType_t ret = xTaskPriorityInherit(pxMutexHolder->task.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTaskPriorityInherit_obj, mp_xTaskPriorityInherit);


static mp_obj_t mp_xTaskPriorityDisinherit(mp_obj_t pxMutexHolder_in)
{
    mp_obj_freertos_task_t *pxMutexHolder = MP_OBJ_TO_PTR(pxMutexHolder_in);

    BaseType_t ret = xTaskPriorityDisinherit(pxMutexHolder->task.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTaskPriorityDisinherit_obj, mp_xTaskPriorityDisinherit);


static mp_obj_t mp_vTaskPriorityDisinheritAfterTimeout(mp_obj_t pxMutexHolder_in,
                                                       mp_obj_t uxHighestPriorityWaitingTask_in)
{
    mp_obj_freertos_task_t *pxMutexHolder = MP_OBJ_TO_PTR(pxMutexHolder_in);
    UBaseType_t uxHighestPriorityWaitingTask = (UBaseType_t)mp_obj_get_int_truncated(uxHighestPriorityWaitingTask_in);

    vTaskPriorityDisinheritAfterTimeout(pxMutexHolder->task.handle,
                                        uxHighestPriorityWaitingTask);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_vTaskPriorityDisinheritAfterTimeout_obj,
                          mp_vTaskPriorityDisinheritAfterTimeout);



// TaskHandle_t pvTaskIncrementMutexHeldCount(void);

// void vTaskInternalSetTimeOutState(TimeOut_t * const pxTimeOut);
