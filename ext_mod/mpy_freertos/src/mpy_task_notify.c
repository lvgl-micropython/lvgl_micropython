
#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "mpy_task.h"
#include "mpy_task_notify.h"


#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static mp_obj_t mp_xTaskNotifyGive(mp_obj_t xTaskToNotify_in)
{
    mp_obj_freertos_task_t *xTaskToNotify = MP_OBJ_TO_PTR(xTaskToNotify_in);

    BaseType_t ret = xTaskNotifyGive(xTaskToNotify->task.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTaskNotifyGive_obj, mp_xTaskNotifyGive);


static mp_obj_t mp_xTaskNotifyGiveIndexed(mp_obj_t xTaskToNotify_in, mp_obj_t uxIndexToNotify_in)
{
    mp_obj_freertos_task_t *xTaskToNotify = MP_OBJ_TO_PTR(xTaskToNotify_in);
    UBaseType_t uxIndexToNotify = (UBaseType_t)mp_obj_get_int_truncated(uxIndexToNotify_in);

    BaseType_t ret = xTaskNotifyGiveIndexed(xTaskToNotify->task.handle, uxIndexToNotify);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xTaskNotifyGiveIndexed_obj, mp_xTaskNotifyGiveIndexed);


static mp_obj_t mp_vTaskNotifyGiveFromISR(mp_obj_t xTaskToNotify_in)
{
    mp_obj_freertos_task_t *xTaskToNotify = MP_OBJ_TO_PTR(xTaskToNotify_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    vTaskNotifyGiveFromISR(xTaskToNotify->task.handle, &pxHigherPriorityTaskWoken);
    return mp_obj_new_int((int)pxHigherPriorityTaskWoken);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vTaskNotifyGiveFromISR_obj, mp_vTaskNotifyGiveFromISR);


static mp_obj_t mp_vTaskNotifyGiveIndexedFromISR(mp_obj_t xTaskToNotify_in, mp_obj_t uxIndexToNotify_in)
{
    mp_obj_freertos_task_t *xTaskToNotify = MP_OBJ_TO_PTR(xTaskToNotify_in);
    UBaseType_t uxIndexToNotify = (UBaseType_t)mp_obj_get_int_truncated(uxIndexToNotify_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    vTaskNotifyGiveIndexedFromISR(xTaskToNotify->task.handle, uxIndexToNotify,
                                  &pxHigherPriorityTaskWoken);
    return mp_obj_new_int((int)pxHigherPriorityTaskWoken);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_vTaskNotifyGiveIndexedFromISR_obj, mp_vTaskNotifyGiveIndexedFromISR);


static mp_obj_t mp_ulTaskNotifyTake(mp_obj_t xClearCountOnExit_in, mp_obj_t xTicksToWait_in)
{
    BaseType_t xClearCountOnExit = (BaseType_t)mp_obj_get_int_truncated(xClearCountOnExit_in);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);

    uint32_t ret = ulTaskNotifyTake(xClearCountOnExit, xTicksToWait);
    return mp_obj_new_int_from_uint(ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_ulTaskNotifyTake_obj, mp_ulTaskNotifyTake);


static mp_obj_t mp_ulTaskNotifyTakeIndexed(mp_obj_t uxIndexToWaitOn_in, mp_obj_t xClearCountOnExit_in,
                                           mp_obj_t xTicksToWait_in)
{
    UBaseType_t uxIndexToWaitOn = (UBaseType_t)mp_obj_get_int_truncated(uxIndexToWaitOn_in);
    BaseType_t xClearCountOnExit = (BaseType_t)mp_obj_get_int_truncated(xClearCountOnExit_in);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);

    uint32_t ret = ulTaskNotifyTakeIndexed(uxIndexToWaitOn, xClearCountOnExit, xTicksToWait);
    return mp_obj_new_int_from_uint(ret);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_ulTaskNotifyTakeIndexed_obj, mp_ulTaskNotifyTakeIndexed);


static mp_obj_t mp_xTaskNotify(mp_obj_t xTaskToNotify_in, mp_obj_t ulValue_in, mp_obj_t eAction_in)
{
    mp_obj_freertos_task_t *xTaskToNotify = MP_OBJ_TO_PTR(xTaskToNotify_in);
    uint32_t ulValue = (uint32_t)mp_obj_get_int_truncated(ulValue_in);
    eNotifyAction eAction = (eNotifyAction)mp_obj_get_int_truncated(eAction_in);

    BaseType_t ret = xTaskNotify(xTaskToNotify->task.handle, ulValue, eAction);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xTaskNotify_obj, mp_xTaskNotify);


static mp_obj_t mp_xTaskNotifyIndexed(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    mp_obj_freertos_task_t *xTaskToNotify = MP_OBJ_TO_PTR(args[0]);
    UBaseType_t uxIndexToNotify = (UBaseType_t)mp_obj_get_int_truncated(args[1]);
    uint32_t ulValue = (uint32_t)mp_obj_get_int_truncated(args[2]);
    eNotifyAction eAction = (eNotifyAction)mp_obj_get_int_truncated(args[3]);

    BaseType_t ret = xTaskNotifyIndexed(xTaskToNotify->task.handle, uxIndexToNotify, ulValue, eAction);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_xTaskNotifyIndexed_obj, 4, 4, mp_xTaskNotifyIndexed);


static mp_obj_t mp_xTaskNotifyAndQuery(mp_obj_t xTaskToNotify_in, mp_obj_t ulValue_in, mp_obj_t eAction_in)
{
    mp_obj_freertos_task_t *xTaskToNotify = MP_OBJ_TO_PTR(xTaskToNotify_in);
    uint32_t ulValue = (uint32_t)mp_obj_get_int_truncated(ulValue_in);
    eNotifyAction eAction = (eNotifyAction)mp_obj_get_int_truncated(eAction_in);
    uint32_t pulPreviousNotifyValue = 0;

    BaseType_t ret = xTaskNotifyAndQuery(xTaskToNotify->task.handle, ulValue,
                                         eAction, &pulPreviousNotifyValue);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int_from_uint(pulPreviousNotifyValue),
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xTaskNotifyAndQuery_obj, mp_xTaskNotifyAndQuery);


static mp_obj_t mp_xTaskNotifyAndQueryIndexed(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    mp_obj_freertos_task_t *xTaskToNotify = MP_OBJ_TO_PTR(args[0]);
    UBaseType_t uxIndexToNotify = (UBaseType_t)mp_obj_get_int_truncated(args[1]);
    uint32_t ulValue = (uint32_t)mp_obj_get_int_truncated(args[2]);
    eNotifyAction eAction = (eNotifyAction)mp_obj_get_int_truncated(args[3]);
    uint32_t pulPreviousNotifyValue = 0;

    BaseType_t ret = xTaskNotifyAndQueryIndexed(xTaskToNotify->task.handle, uxIndexToNotify,
                                                ulValue, eAction, &pulPreviousNotifyValue);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int_from_uint(pulPreviousNotifyValue),
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_xTaskNotifyAndQueryIndexed_obj, 4, 4,
                                    mp_xTaskNotifyAndQueryIndexed);


static mp_obj_t mp_xTaskNotifyAndQueryFromISR(mp_obj_t xTaskToNotify_in, mp_obj_t ulValue_in,
                                              mp_obj_t eAction_in)
{
    mp_obj_freertos_task_t *xTaskToNotify = MP_OBJ_TO_PTR(xTaskToNotify_in);
    uint32_t ulValue = (uint32_t)mp_obj_get_int_truncated(ulValue_in);
    eNotifyAction eAction = (eNotifyAction)mp_obj_get_int_truncated(eAction_in);

    uint32_t pulPreviousNotifyValue = 0;
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xTaskNotifyAndQueryFromISR(xTaskToNotify->task.handle, ulValue,
                                                eAction, &pulPreviousNotifyValue,
                                                &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[3] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int_from_uint(pulPreviousNotifyValue),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };
    return mp_obj_new_tuple(3, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xTaskNotifyAndQueryFromISR_obj, mp_xTaskNotifyAndQueryFromISR);


static mp_obj_t mp_xTaskNotifyAndQueryIndexedFromISR(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    mp_obj_freertos_task_t *xTaskToNotify = MP_OBJ_TO_PTR(args[0]);
    UBaseType_t uxIndexToNotify = (UBaseType_t)mp_obj_get_int_truncated(args[1]);
    uint32_t ulValue = (uint32_t)mp_obj_get_int_truncated(args[2]);
    eNotifyAction eAction = (eNotifyAction)mp_obj_get_int_truncated(args[3]);
    uint32_t pulPreviousNotifyValue = 0;
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xTaskNotifyAndQueryIndexedFromISR(xTaskToNotify->task.handle,
                                                       uxIndexToNotify, ulValue,
                                                       eAction, &pulPreviousNotifyValue,
                                                       &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[3] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int_from_uint(pulPreviousNotifyValue),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };
    return mp_obj_new_tuple(3, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_xTaskNotifyAndQueryIndexedFromISR_obj, 4, 4,
                                    mp_xTaskNotifyAndQueryIndexedFromISR);


static mp_obj_t mp_xTaskNotifyFromISR(mp_obj_t xTaskToNotify_in, mp_obj_t ulValue_in,
                                      mp_obj_t eAction_in)
{
    mp_obj_freertos_task_t *xTaskToNotify = MP_OBJ_TO_PTR(xTaskToNotify_in);
    uint32_t ulValue = (uint32_t)mp_obj_get_int_truncated(ulValue_in);
    eNotifyAction eAction = (eNotifyAction)mp_obj_get_int_truncated(eAction_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xTaskNotifyFromISR(xTaskToNotify->task.handle, ulValue,
                                        eAction, &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xTaskNotifyFromISR_obj, mp_xTaskNotifyFromISR);


static mp_obj_t mp_xTaskNotifyIndexedFromISR(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    mp_obj_freertos_task_t *xTaskToNotify = MP_OBJ_TO_PTR(args[0]);
    UBaseType_t uxIndexToNotify = (UBaseType_t)mp_obj_get_int_truncated(args[1]);
    uint32_t ulValue = (uint32_t)mp_obj_get_int_truncated(args[2]);
    eNotifyAction eAction = (eNotifyAction)mp_obj_get_int_truncated(args[3]);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xTaskNotifyIndexedFromISR(xTaskToNotify->task.handle, uxIndexToNotify,
                                               ulValue, eAction, &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_xTaskNotifyIndexedFromISR_obj, 4, 4,
                                    mp_xTaskNotifyIndexedFromISR);


static mp_obj_t mp_xTaskNotifyWait(mp_obj_t ulBitsToClearOnEntry_in,
                                   mp_obj_t ulBitsToClearOnExit_in,
                                   mp_obj_t xTicksToWait_in)
{
    uint32_t ulBitsToClearOnEntry = (uint32_t)mp_obj_get_int_truncated(ulBitsToClearOnEntry_in);
    uint32_t ulBitsToClearOnExit = (uint32_t)mp_obj_get_int_truncated(ulBitsToClearOnExit_in);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);
    uint32_t pulNotificationValue = 0;

    BaseType_t ret = xTaskNotifyWait(ulBitsToClearOnEntry, ulBitsToClearOnExit,
                                     &pulNotificationValue, xTicksToWait);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int_from_uint(pulNotificationValue)
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xTaskNotifyWait_obj, mp_xTaskNotifyWait);


static mp_obj_t mp_xTaskNotifyWaitIndexed(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    UBaseType_t uxIndexToWaitOn = (UBaseType_t)mp_obj_get_int_truncated(args[0]);
    uint32_t ulBitsToClearOnEntry = (uint32_t)mp_obj_get_int_truncated(args[1]);
    uint32_t ulBitsToClearOnExit = (uint32_t)mp_obj_get_int_truncated(args[2]);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(args[3]);
    uint32_t pulNotificationValue = 0;

    BaseType_t ret = xTaskNotifyWaitIndexed(uxIndexToWaitOn, ulBitsToClearOnEntry,
                                            ulBitsToClearOnExit, &pulNotificationValue,
                                            xTicksToWait);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int_from_uint(pulNotificationValue)
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_xTaskNotifyWaitIndexed_obj, 4, 4,
                                    mp_xTaskNotifyWaitIndexed);


static mp_obj_t mp_xTaskNotifyStateClear(mp_obj_t xTask_in)
{
    mp_obj_freertos_task_t *xTask = MP_OBJ_TO_PTR(xTask_in);

    BaseType_t ret = xTaskNotifyStateClear(xTask->task.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTaskNotifyStateClear_obj, mp_xTaskNotifyStateClear);


static mp_obj_t mp_xTaskNotifyStateClearIndexed(mp_obj_t xTask_in, mp_obj_t uxIndexToClear_in)
{
    mp_obj_freertos_task_t *xTask = MP_OBJ_TO_PTR(xTask_in);
    UBaseType_t uxIndexToClear = (UBaseType_t)mp_obj_get_int_truncated(uxIndexToClear_in);

    BaseType_t ret = xTaskNotifyStateClearIndexed(xTask->task.handle, uxIndexToClear);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xTaskNotifyStateClearIndexed_obj, mp_xTaskNotifyStateClearIndexed);


static mp_obj_t mp_ulTaskNotifyValueClear(mp_obj_t xTask_in, mp_obj_t ulBitsToClear_in)
{
    mp_obj_freertos_task_t *xTask = MP_OBJ_TO_PTR(xTask_in);
    uint32_t ulBitsToClear = (uint32_t)mp_obj_get_int_truncated(ulBitsToClear_in);

    uint32_t ret = ulTaskNotifyValueClear(xTask->task.handle, ulBitsToClear);
    return mp_obj_new_int_from_uint(ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_ulTaskNotifyValueClear_obj, mp_ulTaskNotifyValueClear);


static mp_obj_t mp_ulTaskNotifyValueClearIndexed(mp_obj_t xTask_in, mp_obj_t uxIndexToClear_in,
                                                 mp_obj_t ulBitsToClear_in)
{
    mp_obj_freertos_task_t *xTask = MP_OBJ_TO_PTR(xTask_in);
    UBaseType_t uxIndexToClear = (UBaseType_t)mp_obj_get_int_truncated(uxIndexToClear_in);
    uint32_t ulBitsToClear = (uint32_t)mp_obj_get_int_truncated(ulBitsToClear_in);

    uint32_t ret = ulTaskNotifyValueClearIndexed(xTask->task.handle, uxIndexToClear, ulBitsToClear);
    return mp_obj_new_int_from_uint(ret);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_ulTaskNotifyValueClearIndexed_obj, mp_ulTaskNotifyValueClearIndexed);





