
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

#ifndef __TIMERS_H__
    #define __TIMERS_H__

    typedef struct _freertos_timer_t {
        TimerHandle_t handle;
        StaticTimer_t buffer;
    } freertos_timer_t;

    typedef struct _mp_obj_freertos_timer_t {
        freertos_timer_t timer;
        mp_freertos_types type;
        mp_obj_t callback;
        uint32_t pvTimerID;
    } mp_obj_freertos_timer_t;

    extern const mp_obj_fun_builtin_var_t mp_xTimerCreateStatic_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_pvTimerGetTimerID_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTimerSetTimerID_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerIsTimerActive_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerStart_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerStop_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerChangePeriod_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerDelete_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerReset_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerStartFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerStopFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerChangePeriodFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerResetFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_pcTimerGetName_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTimerSetReloadMode_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerGetReloadMode_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxTimerGetReloadMode_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerGetPeriod_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerGetExpiryTime_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTimerCreateTimerTask_obj;

#if ( configUSE_TRACE_FACILITY == 1 )
    extern const mp_obj_fun_builtin_fixed_t mp_vTimerSetTimerNumber_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxTimerGetTimerNumber_obj;
#endif

#endif