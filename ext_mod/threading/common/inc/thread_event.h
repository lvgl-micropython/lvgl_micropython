// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"


#ifndef __THREAD_EVENT_H__
    #define __THREAD_EVENT_H__

    typedef struct _mp_event_group_t {
        EventGroupHandle_t handle;
        StaticEventGroup_t buffer;
    } mp_event_group_t;


    typedef struct _mp_obj_thread_event_t {
        mp_obj_base_t base;
        mp_event_group_t event;
    } mp_obj_thread_event_t;


    extern const mp_obj_fun_builtin_fixed_t event_is_set_obj;
    extern const mp_obj_fun_builtin_fixed_t event_set_obj;
    extern const mp_obj_fun_builtin_fixed_t event_clear_obj;
    extern const mp_obj_fun_builtin_fixed_t event_is_set_obj;
    extern const mp_obj_fun_builtin_fixed_t event__del__obj;
    extern const mp_obj_fun_builtin_var_t event_wait_obj;

#endif













