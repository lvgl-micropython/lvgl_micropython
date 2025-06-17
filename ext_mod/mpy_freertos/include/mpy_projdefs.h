
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"


#ifndef __PROJDEFS_H__
    #define __PROJDEFS_H__

    extern const mp_obj_fun_builtin_fixed_t mp_pdMS_TO_TICKS_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_pdTICKS_TO_MS_obj;
#endif