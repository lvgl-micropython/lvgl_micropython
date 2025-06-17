
#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "mpy_timers.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"


static uint32_t next_timer_id = 0;


// typedef void (* TimerCallbackFunction_t)( TimerHandle_t xTimer );
static void mp_TimerCallbackFunction(TimerHandle_t xTimer)
{
    mp_obj_freertos_timer_t *self = (mp_obj_freertos_timer_t *)xTimer;
    // callback
    mp_obj_t args[1] = { MP_OBJ_FROM_PTR(self) };
    mp_call_function_n_kw(self->callback, 1, 0, &args[0]);
}


static mp_obj_t mp_xTimerCreateStatic(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    mp_obj_freertos_timer_t *self = m_new_obj(mp_obj_freertos_timer_t);
    self->type = mp_freertos_timer_type;

    size_t str_len;
    const char *pcTimerName = mp_obj_str_get_data(args[0], &str_len);

    TickType_t xTimerPeriodInTicks = (TickType_t)mp_obj_get_int_truncated(args[1]);
    BaseType_t xAutoReload = (BaseType_t)mp_obj_get_int(args[2]);
    self->callback = args[3];

    self->pvTimerID = next_timer_id;
    next_timer_id++;

    self->timer.handle = xTimerCreateStatic(pcTimerName, xTimerPeriodInTicks,
                                            xAutoReload, (void *)self->pvTimerID,
                                            &mp_TimerCallbackFunction,
                                            &self->timer.buffer);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_xTimerCreateStatic_obj, 4, 4,
                                    mp_xTimerCreateStatic);


static mp_obj_t mp_pvTimerGetTimerID(mp_obj_t xTimer_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    uint32_t ret = (uint32_t)pvTimerGetTimerID(xTimer->timer.handle);
    return mp_obj_new_int_from_uint(ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_pvTimerGetTimerID_obj, mp_pvTimerGetTimerID);


static mp_obj_t mp_vTimerSetTimerID(mp_obj_t xTimer_in, mp_obj_t pvNewID_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    xTimer->pvTimerID = (uint32_t)mp_obj_get_int_truncated(pvNewID_in);

    vTimerSetTimerID(xTimer->timer.handle, (void *)xTimer->pvTimerID);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_vTimerSetTimerID_obj, mp_vTimerSetTimerID);


static mp_obj_t mp_xTimerIsTimerActive(mp_obj_t xTimer_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    BaseType_t ret = xTimerIsTimerActive(xTimer->timer.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTimerIsTimerActive_obj, mp_xTimerIsTimerActive);

// TaskHandle_t xTimerGetTimerDaemonTaskHandle(void);


static mp_obj_t mp_xTimerStart(mp_obj_t xTimer_in, mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int(xTicksToWait_in);
    BaseType_t ret = xTimerStart(xTimer->timer.handle, xTicksToWait);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xTimerStart_obj, mp_xTimerStart);


static mp_obj_t mp_xTimerStop(mp_obj_t xTimer_in, mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int(xTicksToWait_in);
    BaseType_t ret = xTimerStop(xTimer->timer.handle, xTicksToWait);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xTimerStop_obj, mp_xTimerStop);


static mp_obj_t mp_xTimerChangePeriod(mp_obj_t xTimer_in, mp_obj_t xNewPeriod_in,
                                      mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    TickType_t xNewPeriod = (TickType_t)mp_obj_get_int_truncated(xNewPeriod_in);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);

    BaseType_t ret = xTimerChangePeriod(xTimer->timer.handle, xNewPeriod, xTicksToWait);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xTimerChangePeriod_obj, mp_xTimerChangePeriod);


static mp_obj_t mp_xTimerDelete(mp_obj_t xTimer_in, mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);
    BaseType_t ret = xTimerDelete(xTimer->timer.handle, xTicksToWait);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xTimerDelete_obj, mp_xTimerDelete);


static mp_obj_t mp_xTimerReset(mp_obj_t xTimer_in, mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);
    BaseType_t ret = xTimerReset(xTimer->timer.handle, xTicksToWait);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xTimerReset_obj, mp_xTimerReset);


static mp_obj_t mp_xTimerStartFromISR(mp_obj_t xTimer_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xTimerStartFromISR(xTimer->timer.handle,
                                        &pxHigherPriorityTaskWoken);

     mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTimerStartFromISR_obj, mp_xTimerStartFromISR);


static mp_obj_t mp_xTimerStopFromISR(mp_obj_t xTimer_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xTimerStopFromISR(xTimer->timer.handle,
                                       &pxHigherPriorityTaskWoken);

     mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTimerStopFromISR_obj, mp_xTimerStopFromISR);


static mp_obj_t mp_xTimerChangePeriodFromISR(mp_obj_t xTimer_in, mp_obj_t xNewPeriod_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    TickType_t xNewPeriod = (TickType_t)mp_obj_get_int_truncated(xNewPeriod_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xTimerChangePeriodFromISR(xTimer->timer.handle, xNewPeriod,
                                               &pxHigherPriorityTaskWoken);

     mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xTimerChangePeriodFromISR_obj,
                          mp_xTimerChangePeriodFromISR);


static mp_obj_t mp_xTimerResetFromISR(mp_obj_t xTimer_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xTimerResetFromISR(xTimer->timer.handle,
                                        &pxHigherPriorityTaskWoken);

     mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken),
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTimerResetFromISR_obj, mp_xTimerResetFromISR);

// typedef void (* PendedFunction_t)(void *, uint32_t);

// BaseType_t xTimerPendFunctionCallFromISR(PendedFunction_t xFunctionToPend, void * pvParameter1, uint32_t ulParameter2, BaseType_t * pxHigherPriorityTaskWoken);

// BaseType_t xTimerPendFunctionCall(PendedFunction_t xFunctionToPend, void * pvParameter1, uint32_t ulParameter2, TickType_t xTicksToWait);


static mp_obj_t mp_pcTimerGetName(mp_obj_t xTimer_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    const char *ret = pcTimerGetName(xTimer->timer.handle);
    return mp_obj_new_str_from_cstr(ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_pcTimerGetName_obj, mp_pcTimerGetName);


static mp_obj_t mp_vTimerSetReloadMode(mp_obj_t xTimer_in, mp_obj_t xAutoReload_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    BaseType_t xAutoReload = (BaseType_t)mp_obj_get_int(xAutoReload_in);
    vTimerSetReloadMode(xTimer->timer.handle, xAutoReload);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_vTimerSetReloadMode_obj, mp_vTimerSetReloadMode);


static mp_obj_t mp_xTimerGetReloadMode(mp_obj_t xTimer_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    BaseType_t ret = xTimerGetReloadMode(xTimer->timer.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTimerGetReloadMode_obj, mp_xTimerGetReloadMode);


static mp_obj_t mp_uxTimerGetReloadMode(mp_obj_t xTimer_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    UBaseType_t ret = uxTimerGetReloadMode(xTimer->timer.handle);
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_uxTimerGetReloadMode_obj, mp_uxTimerGetReloadMode);


static mp_obj_t mp_xTimerGetPeriod(mp_obj_t xTimer_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    TickType_t ret = xTimerGetPeriod(xTimer->timer.handle);
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTimerGetPeriod_obj, mp_xTimerGetPeriod);


static mp_obj_t mp_xTimerGetExpiryTime(mp_obj_t xTimer_in)
{
    mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
    TickType_t ret = xTimerGetExpiryTime(xTimer->timer.handle);
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTimerGetExpiryTime_obj, mp_xTimerGetExpiryTime);

// BaseType_t xTimerGetStaticBuffer(TimerHandle_t xTimer, StaticTimer_t ** ppxTimerBuffer);


static mp_obj_t mp_xTimerCreateTimerTask(void)
{
    BaseType_t ret = xTimerCreateTimerTask();
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xTimerCreateTimerTask_obj, mp_xTimerCreateTimerTask);

// BaseType_t xTimerGenericCommand(TimerHandle_t xTimer, const BaseType_t xCommandID, const TickType_t xOptionalValue, BaseType_t * const pxHigherPriorityTaskWoken, const TickType_t xTicksToWait);


#if ( configUSE_TRACE_FACILITY == 1 )

    static mp_obj_t mp_vTimerSetTimerNumber(mp_obj_t xTimer_in, mp_obj_t uxTimerNumber_in)
    {
        mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
        UBaseType_t uxTimerNumber = (UBaseType_t)mp_obj_get_int_truncated(uxTimerNumber_in);
        vTimerSetTimerNumber(xTimer->timer.handle, uxTimerNumber);
        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_2(mp_vTimerSetTimerNumber_obj, mp_vTimerSetTimerNumber);


    static mp_obj_t mp_uxTimerGetTimerNumber(mp_obj_t xTimer_in)
    {
        mp_obj_freertos_timer_t *xTimer = MP_OBJ_TO_PTR(xTimer_in);
        UBaseType_t ret = uxTimerGetTimerNumber(xTimer->timer.handle);
        return mp_obj_new_int_from_uint((uint32_t)ret);
    }

    MP_DEFINE_CONST_FUN_OBJ_1(mp_uxTimerGetTimerNumber_obj, mp_uxTimerGetTimerNumber);

#endif


// void vApplicationGetTimerTaskMemory(StaticTask_t ** ppxTimerTaskTCBBuffer, StackType_t ** ppxTimerTaskStackBuffer, uint32_t * pulTimerTaskStackSize);