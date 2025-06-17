
#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "mpy_stream_buffer.h"


#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"


static mp_obj_t mp_xStreamBufferCreateStatic(mp_obj_t xBufferSizeBytes_in,
                                             mp_obj_t xTriggerLevelBytes_in)
{
    mp_obj_freertos_stream_buffer_t *self = m_new_obj(mp_obj_freertos_stream_buffer_t);
    self->type = mp_freertos_stream_buffer_type;

    size_t xBufferSizeBytes = (size_t)mp_obj_get_int(xBufferSizeBytes_in);
    size_t xTriggerLevelBytes = (size_t)mp_obj_get_int(xTriggerLevelBytes_in);

    self->pucStreamBufferStorageArea = malloc(xBufferSizeBytes);
    if (self->pucStreamBufferStorageArea == NULL) {
        free(self);
        return mp_const_none;
    } else {
        self->stream_buffer.handle = xStreamBufferCreateStatic(xBufferSizeBytes, xTriggerLevelBytes,
                                                               self->pucStreamBufferStorageArea,
                                                               &self->stream_buffer.buffer);
        return MP_OBJ_FROM_PTR(self);
    }
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xStreamBufferCreateStatic_obj,
                          mp_xStreamBufferCreateStatic);


static mp_obj_t mp_xStreamBufferSend(mp_obj_t xStreamBuffer_in, mp_obj_t pvTxData_in,
                                     mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);
    mp_buffer_info_t pvTxData;
    mp_get_buffer_raise(pvTxData_in, &pvTxData, MP_BUFFER_READ);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);

    size_t ret = xStreamBufferSend(xStreamBuffer->stream_buffer.handle, pvTxData.buf,
                                   pvTxData.len, xTicksToWait);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xStreamBufferSend_obj, mp_xStreamBufferSend);


static mp_obj_t mp_xStreamBufferSendFromISR(mp_obj_t xStreamBuffer_in,
                                            mp_obj_t pvTxData_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);
    mp_buffer_info_t pvTxData;
    mp_get_buffer_raise(pvTxData_in, &pvTxData, MP_BUFFER_READ);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    size_t ret = xStreamBufferSendFromISR(xStreamBuffer->stream_buffer.handle,
                                          pvTxData.buf, pvTxData.len,
                                          &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xStreamBufferSendFromISR_obj,
                          mp_xStreamBufferSendFromISR);


static mp_obj_t mp_xStreamBufferReceive(mp_obj_t xStreamBuffer_in, mp_obj_t pvRxData_in,
                                        mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);
    mp_buffer_info_t pvRxData;
    mp_get_buffer_raise(pvRxData_in, &pvRxData, MP_BUFFER_WRITE);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);

    size_t ret = xStreamBufferSend(xStreamBuffer->stream_buffer.handle,
                                   pvRxData.buf, pvRxData.len, xTicksToWait);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xStreamBufferReceive_obj, mp_xStreamBufferReceive);


static mp_obj_t mp_xStreamBufferReceiveFromISR(mp_obj_t xStreamBuffer_in,
                                               mp_obj_t pvRxData_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);
    mp_buffer_info_t pvRxData;
    mp_get_buffer_raise(pvRxData_in, &pvRxData, MP_BUFFER_READ);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    size_t ret = xStreamBufferReceiveFromISR(xStreamBuffer->stream_buffer.handle,
                                             pvRxData.buf, pvRxData.len,
                                             &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xStreamBufferReceiveFromISR_obj,
                          mp_xStreamBufferReceiveFromISR);


static mp_obj_t mp_vStreamBufferDelete(mp_obj_t xStreamBuffer_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);

    vStreamBufferDelete(xStreamBuffer->stream_buffer.handle);

    if (xStreamBuffer->pucStreamBufferStorageArea != NULL) {
        free(xStreamBuffer->pucStreamBufferStorageArea);
        xStreamBuffer->pucStreamBufferStorageArea = NULL;
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vStreamBufferDelete_obj, mp_vStreamBufferDelete);


static mp_obj_t mp_xStreamBufferIsFull(mp_obj_t xStreamBuffer_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);

    BaseType_t ret = xStreamBufferIsFull(xStreamBuffer->stream_buffer.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xStreamBufferIsFull_obj, mp_xStreamBufferIsFull);


static mp_obj_t mp_xStreamBufferIsEmpty(mp_obj_t xStreamBuffer_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);

    BaseType_t ret = xStreamBufferIsEmpty(xStreamBuffer->stream_buffer.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xStreamBufferIsEmpty_obj, mp_xStreamBufferIsEmpty);


static mp_obj_t mp_xStreamBufferReset(mp_obj_t xStreamBuffer_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);

    BaseType_t ret = xStreamBufferReset(xStreamBuffer->stream_buffer.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xStreamBufferReset_obj, mp_xStreamBufferReset);


static mp_obj_t mp_xStreamBufferSpacesAvailable(mp_obj_t xStreamBuffer_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);

    size_t ret = xStreamBufferSpacesAvailable(xStreamBuffer->stream_buffer.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xStreamBufferSpacesAvailable_obj,
                          mp_xStreamBufferSpacesAvailable);


static mp_obj_t mp_xStreamBufferBytesAvailable(mp_obj_t xStreamBuffer_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);

    size_t ret = xStreamBufferBytesAvailable(xStreamBuffer->stream_buffer.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xStreamBufferBytesAvailable_obj,
                          mp_xStreamBufferBytesAvailable);


static mp_obj_t mp_xStreamBufferSetTriggerLevel(mp_obj_t xStreamBuffer_in,
                                                mp_obj_t xTriggerLevel_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);
    size_t xTriggerLevel = (size_t)mp_obj_get_int(xTriggerLevel_in);

    BaseType_t ret = xStreamBufferSetTriggerLevel(xStreamBuffer->stream_buffer.handle,
                                                  xTriggerLevel);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xStreamBufferSetTriggerLevel_obj,
                          mp_xStreamBufferSetTriggerLevel);


static mp_obj_t mp_xStreamBufferSendCompletedFromISR(mp_obj_t xStreamBuffer_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xStreamBufferSendCompletedFromISR(xStreamBuffer->stream_buffer.handle,
                                                       &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xStreamBufferSendCompletedFromISR_obj,
                          mp_xStreamBufferSendCompletedFromISR);


static mp_obj_t mp_xStreamBufferReceiveCompletedFromISR(mp_obj_t xStreamBuffer_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xStreamBufferReceiveCompletedFromISR(xStreamBuffer->stream_buffer.handle,
                                                          &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xStreamBufferReceiveCompletedFromISR_obj,
                          mp_xStreamBufferReceiveCompletedFromISR);
