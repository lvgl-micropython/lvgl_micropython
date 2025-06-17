
#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "mpy_event_groups.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"



mp_obj_t mp_xEventGroupCreateStatic(void)
{
    mp_obj_freertos_event_group_t *self = m_new_obj(mp_obj_freertos_event_group_t);
    self->type = mp_freertos_event_group_type;

    self->event_group.handle = xEventGroupCreateStatic(&self->event_group.buffer);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xEventGroupCreateStatic_obj, mp_xEventGroupCreateStatic);


mp_obj_t mp_xEventGroupWaitBits(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    mp_obj_freertos_event_group_t *xEventGroup = MP_OBJ_TO_PTR(args[0]);
    EventBits_t uxBitsToWaitFor = (EventBits_t)mp_obj_get_int_truncated(args[1]);
    BaseType_t xClearOnExit = (BaseType_t)mp_obj_get_int(args[2]);
    BaseType_t xWaitForAllBits = (BaseType_t)mp_obj_get_int(args[3]);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(args[4]);

    EventBits_t res = xEventGroupWaitBits(xEventGroup->event_group.handle, uxBitsToWaitFor, xClearOnExit, xWaitForAllBits, xTicksToWait);
    return mp_obj_new_int_from_uint((uint32_t)res);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_xEventGroupWaitBits_obj, 5, 5, mp_xEventGroupWaitBits);


mp_obj_t mp_xEventGroupClearBits(mp_obj_t xEventGroup_in, mp_obj_t uxBitsToClear_in)
{
    mp_obj_freertos_event_group_t *xEventGroup = MP_OBJ_TO_PTR(xEventGroup_in);
    EventBits_t uxBitsToClear = (EventBits_t)mp_obj_get_int_truncated(uxBitsToClear_in);

    EventBits_t res = xEventGroupClearBits(xEventGroup->event_group.handle, uxBitsToClear);
    return mp_obj_new_int_from_uint((uint32_t)res);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xEventGroupClearBits_obj, mp_xEventGroupClearBits);


mp_obj_t mp_xEventGroupClearBitsFromISR(mp_obj_t xEventGroup_in, mp_obj_t uxBitsToClear_in)
{
    mp_obj_freertos_event_group_t *xEventGroup = MP_OBJ_TO_PTR(xEventGroup_in);
    EventBits_t uxBitsToClear = (EventBits_t)mp_obj_get_int_truncated(uxBitsToClear_in);

    BaseType_t res = xEventGroupClearBitsFromISR(xEventGroup->event_group.handle, uxBitsToClear);
    return mp_obj_new_int((int)res);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xEventGroupClearBitsFromISR_obj, mp_xEventGroupClearBitsFromISR);


mp_obj_t mp_xEventGroupSetBits(mp_obj_t xEventGroup_in, mp_obj_t uxBitsToSet_in)
{
    mp_obj_freertos_event_group_t *xEventGroup = MP_OBJ_TO_PTR(xEventGroup_in);
    EventBits_t uxBitsToSet = (EventBits_t)mp_obj_get_int_truncated(uxBitsToSet_in);

    EventBits_t res = xEventGroupSetBits(xEventGroup->event_group.handle, uxBitsToSet);
    return mp_obj_new_int_from_uint((uint32_t)res);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xEventGroupSetBits_obj, mp_xEventGroupSetBits);


mp_obj_t mp_xEventGroupSetBitsFromISR(mp_obj_t xEventGroup_in, mp_obj_t uxBitsToSet_in)
{
    mp_obj_freertos_event_group_t *xEventGroup = MP_OBJ_TO_PTR(xEventGroup_in);
    EventBits_t uxBitsToSet = (EventBits_t)mp_obj_get_int_truncated(uxBitsToSet_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xEventGroupSetBitsFromISR(xEventGroup->event_group.handle, uxBitsToSet, &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken),
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xEventGroupSetBitsFromISR_obj, mp_xEventGroupSetBitsFromISR);


mp_obj_t mp_xEventGroupSync(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    mp_obj_freertos_event_group_t *xEventGroup = MP_OBJ_TO_PTR(args[0]);
    EventBits_t uxBitsToSet = (EventBits_t)mp_obj_get_int_truncated(args[1]);
    EventBits_t uxBitsToWaitFor = (EventBits_t)mp_obj_get_int_truncated(args[2]);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(args[3]);

    EventBits_t res = xEventGroupSync(xEventGroup->event_group.handle, uxBitsToSet, uxBitsToWaitFor, xTicksToWait);
    return mp_obj_new_int_from_uint((uint32_t)res);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_xEventGroupSync_obj, 4, 4, mp_xEventGroupSync);


mp_obj_t mp_xEventGroupGetBits(mp_obj_t xEventGroup_in)
{
    return mp_xEventGroupClearBits(xEventGroup_in, mp_obj_new_int_from_uint(0));
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xEventGroupGetBits_obj, mp_xEventGroupGetBits);


mp_obj_t mp_xEventGroupGetBitsFromISR(mp_obj_t xEventGroup_in)
{
    mp_obj_freertos_event_group_t *xEventGroup = MP_OBJ_TO_PTR(xEventGroup_in);

    EventBits_t res = xEventGroupGetBitsFromISR(xEventGroup->event_group.handle);
    return mp_obj_new_int_from_uint((uint32_t)res);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xEventGroupGetBitsFromISR_obj, mp_xEventGroupGetBitsFromISR);


mp_obj_t mp_vEventGroupDelete(mp_obj_t xEventGroup_in)
{
    mp_obj_freertos_event_group_t *xEventGroup = MP_OBJ_TO_PTR(xEventGroup_in);

    vEventGroupDelete(xEventGroup->event_group.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vEventGroupDelete_obj, mp_vEventGroupDelete);


// BaseType_t xEventGroupGetStaticBuffer( EventGroupHandle_t xEventGroup, StaticEventGroup_t ** ppxEventGroupBuffer );
