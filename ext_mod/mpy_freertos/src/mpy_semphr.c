

#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "mpy_semphr.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


mp_obj_t mp_xSemaphoreCreateBinaryStatic(void)
{
    mp_obj_freertos_semaphore_t *self = m_new_obj(mp_obj_freertos_semaphore_t);
    self->type = mp_freertos_semaphore_type;

    self->semaphore.handle = xSemaphoreCreateBinaryStatic(&self->semaphore.buffer);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xSemaphoreCreateBinaryStatic_obj, mp_xSemaphoreCreateBinaryStatic);


mp_obj_t mp_xSemaphoreTake(mp_obj_t xSemaphore_in, mp_obj_t xBlockTime_in)
{
    mp_obj_freertos_semaphore_t *xSemaphore = MP_OBJ_TO_PTR(xSemaphore_in);
    TickType_t xBlockTime = (TickType_t)mp_obj_get_int_truncated(xBlockTime_in);

    BaseType_t ret = xSemaphoreTake(xSemaphore->semaphore.handle, xBlockTime);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xSemaphoreTake_obj, mp_xSemaphoreTake);


mp_obj_t mp_xSemaphoreTakeRecursive(mp_obj_t xMutex_in, mp_obj_t xBlockTime_in)
{
    mp_obj_freertos_semaphore_t *xMutex = MP_OBJ_TO_PTR(xMutex_in);
    TickType_t xBlockTime = (TickType_t)mp_obj_get_int_truncated(xBlockTime_in);

    BaseType_t ret = xSemaphoreTakeRecursive(xMutex->semaphore.handle, xBlockTime);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xSemaphoreTakeRecursive_obj, mp_xSemaphoreTakeRecursive);


mp_obj_t mp_xSemaphoreGive(mp_obj_t xSemaphore_in)
{
    mp_obj_freertos_semaphore_t *xSemaphore = MP_OBJ_TO_PTR(xSemaphore_in);

    BaseType_t ret = xSemaphoreGive(xSemaphore->semaphore.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xSemaphoreGive_obj, mp_xSemaphoreGive);


mp_obj_t mp_xSemaphoreGiveRecursive(mp_obj_t xMutex_in)
{
    mp_obj_freertos_semaphore_t *xMutex = MP_OBJ_TO_PTR(xMutex_in);

    BaseType_t ret = xSemaphoreGiveRecursive(xMutex->semaphore.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xSemaphoreGiveRecursive_obj, mp_xSemaphoreGiveRecursive);


mp_obj_t mp_xSemaphoreGiveFromISR(mp_obj_t xSemaphore_in)
{
    mp_obj_freertos_semaphore_t *xSemaphore = MP_OBJ_TO_PTR(xSemaphore_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xSemaphoreGiveFromISR(xSemaphore->semaphore.handle, &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xSemaphoreGiveFromISR_obj, mp_xSemaphoreGiveFromISR);


mp_obj_t mp_xSemaphoreTakeFromISR(mp_obj_t xSemaphore_in)
{
    mp_obj_freertos_semaphore_t *xSemaphore = MP_OBJ_TO_PTR(xSemaphore_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xSemaphoreTakeFromISR(xSemaphore->semaphore.handle, &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xSemaphoreTakeFromISR_obj, mp_xSemaphoreTakeFromISR);


mp_obj_t mp_xSemaphoreCreateMutexStatic(void)
{
    mp_obj_freertos_semaphore_t *self = m_new_obj(mp_obj_freertos_semaphore_t);
    self->type = mp_freertos_semaphore_type;

    self->semaphore.handle = xSemaphoreCreateMutexStatic(&self->semaphore.buffer);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xSemaphoreCreateMutexStatic_obj, mp_xSemaphoreCreateMutexStatic);


mp_obj_t mp_xSemaphoreCreateRecursiveMutexStatic(void)
{
    mp_obj_freertos_semaphore_t *self = m_new_obj(mp_obj_freertos_semaphore_t);
    self->type = mp_freertos_semaphore_type;

    self->semaphore.handle = xSemaphoreCreateRecursiveMutexStatic(&self->semaphore.buffer);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xSemaphoreCreateRecursiveMutexStatic_obj, mp_xSemaphoreCreateRecursiveMutexStatic);


mp_obj_t mp_xSemaphoreCreateCountingStatic(mp_obj_t uxMaxCount_in, mp_obj_t uxInitialCount_in)
{
    mp_obj_freertos_semaphore_t *self = m_new_obj(mp_obj_freertos_semaphore_t);
    self->type = mp_freertos_semaphore_type;

    UBaseType_t uxMaxCount = (UBaseType_t)mp_obj_get_int_truncated(uxMaxCount_in);
    UBaseType_t uxInitialCount = (UBaseType_t)mp_obj_get_int_truncated(uxInitialCount_in);

    self->semaphore.handle = xSemaphoreCreateCountingStatic(uxMaxCount, uxInitialCount, &self->semaphore.buffer);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xSemaphoreCreateCountingStatic_obj, mp_xSemaphoreCreateCountingStatic);


// vSemaphoreDelete, vQueueDelete
// xSemaphoreGetMutexHolder, xQueueGetMutexHolder
// xSemaphoreGetMutexHolderFromISR, xQueueGetMutexHolderFromISR


mp_obj_t mp_uxSemaphoreGetCount(mp_obj_t xSemaphore_in)
{
    mp_obj_freertos_semaphore_t *xSemaphore = MP_OBJ_TO_PTR(xSemaphore_in);

    UBaseType_t ret = uxSemaphoreGetCount(xSemaphore->semaphore.handle);
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_uxSemaphoreGetCount_obj, mp_uxSemaphoreGetCount);


mp_obj_t mp_uxSemaphoreGetCountFromISR(mp_obj_t xSemaphore_in)
{
    mp_obj_freertos_semaphore_t *xSemaphore = MP_OBJ_TO_PTR(xSemaphore_in);

    UBaseType_t ret = uxSemaphoreGetCountFromISR(xSemaphore->semaphore.handle);
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_uxSemaphoreGetCountFromISR_obj, mp_uxSemaphoreGetCountFromISR);

// BaseType_t xSemaphoreGetStaticBuffer(SemaphoreHandle_t xSemaphore, StaticSemaphore_t **ppxSemaphoreBuffer)