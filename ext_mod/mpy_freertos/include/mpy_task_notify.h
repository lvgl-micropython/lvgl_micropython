
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


#ifndef __TASK_NOTIFY_H__
    #define __TASK_NOTIFY_H__

    extern const mp_obj_fun_builtin_fixed_t mp_xTaskNotifyGive_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskNotifyGiveIndexed_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskNotifyGiveFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vTaskNotifyGiveIndexedFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_ulTaskNotifyTake_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_ulTaskNotifyTakeIndexed_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskNotify_obj;
    extern const mp_obj_fun_builtin_var_t mp_xTaskNotifyWaitIndexed_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskNotifyWait_obj;
    extern const mp_obj_fun_builtin_var_t mp_xTaskNotifyIndexedFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskNotifyFromISR_obj;
    extern const mp_obj_fun_builtin_var_t mp_xTaskNotifyAndQueryIndexedFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskNotifyAndQueryFromISR_obj;
    extern const mp_obj_fun_builtin_var_t mp_xTaskNotifyAndQueryIndexed_obj;
    extern const mp_obj_fun_builtin_var_t mp_xTaskNotifyIndexed_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskNotifyAndQuery_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskNotifyStateClear_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xTaskNotifyStateClearIndexed_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_ulTaskNotifyValueClear_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_ulTaskNotifyValueClearIndexed_obj;

#endif