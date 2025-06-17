
#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "mpy_projdefs.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"


static mp_obj_t mp_pdMS_TO_TICKS(mp_obj_t xTimeInMs_in)
{
    TickType_t xTimeInMs = (TickType_t)mp_obj_get_int_truncated(xTimeInMs_in);

    TickType_t ret = pdTICKS_TO_MS(xTimeInMs);
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_pdMS_TO_TICKS_obj, mp_pdMS_TO_TICKS);



static mp_obj_t mp_pdTICKS_TO_MS(mp_obj_t xTicks_in)
{
    uint64_t xTicks = (uint64_t)mp_obj_get_int_truncated(xTicks_in);

    TickType_t ret = pdTICKS_TO_MS(xTicks);
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_pdTICKS_TO_MS_obj, mp_pdTICKS_TO_MS);