
#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"


#ifndef __EVENT_GROUPS_H__
    #define __EVENT_GROUPS_H__

    typedef struct _freertos_event_group_t {
        EventGroupHandle_t handle;
        StaticEventGroup_t buffer;
    } freertos_event_group_t;


    typedef struct _mp_obj_freertos_event_group_t {
        freertos_event_group_t event_group;
        mp_freertos_types type;
    } mp_obj_freertos_event_group_t;

    extern const mp_obj_fun_builtin_fixed_t mp_xEventGroupCreateStatic_obj;
    extern const mp_obj_fun_builtin_var_t mp_xEventGroupWaitBits_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xEventGroupClearBits_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xEventGroupClearBitsFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xEventGroupSetBits_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xEventGroupSetBitsFromISR_obj;
    extern const mp_obj_fun_builtin_var_t mp_xEventGroupSync_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xEventGroupGetBits_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xEventGroupGetBitsFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vEventGroupDelete_obj;





#endif
