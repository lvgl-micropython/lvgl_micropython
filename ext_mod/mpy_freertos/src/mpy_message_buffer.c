#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "mpy_message_buffer.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"
#include "freertos/message_buffer.h"


static mp_obj_t mp_xMessageBufferCreateStatic(mp_obj_t xBufferSizeBytes_in)
{
    mp_obj_freertos_message_buffer_t *self = m_new_obj(mp_obj_freertos_message_buffer_t);
    self->type = mp_freertos_message_buffer_type;

    size_t xBufferSizeBytes = (size_t)mp_obj_get_int(xBufferSizeBytes_in);

    self->pucStreamBufferStorageArea = malloc(xBufferSizeBytes);
    if (self->pucStreamBufferStorageArea == NULL) {
        free(self);
        return mp_const_none;
    } else {
        self->message_buffer.handle = xMessageBufferCreateStatic(xBufferSizeBytes,
                                                               self->pucStreamBufferStorageArea,
                                                               &self->message_buffer.buffer);
        return MP_OBJ_FROM_PTR(self);
    }
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xMessageBufferCreateStatic_obj,
                          mp_xMessageBufferCreateStatic);


static mp_obj_t mp_xMessageBufferSend(mp_obj_t xMessageBuffer_in, mp_obj_t pvTxData_in,
                                     mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_message_buffer_t *xMessageBuffer = MP_OBJ_TO_PTR(xMessageBuffer_in);
    mp_buffer_info_t pvTxData;
    mp_get_buffer_raise(pvTxData_in, &pvTxData, MP_BUFFER_READ);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);

    size_t ret = xMessageBufferSend(xMessageBuffer->message_buffer.handle, pvTxData.buf,
                                   pvTxData.len, xTicksToWait);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xMessageBufferSend_obj, mp_xMessageBufferSend);


static mp_obj_t mp_xMessageBufferSendFromISR(mp_obj_t xMessageBuffer_in,
                                            mp_obj_t pvTxData_in)
{
    mp_obj_freertos_message_buffer_t *xMessageBuffer = MP_OBJ_TO_PTR(xMessageBuffer_in);
    mp_buffer_info_t pvTxData;
    mp_get_buffer_raise(pvTxData_in, &pvTxData, MP_BUFFER_READ);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    size_t ret = xMessageBufferSendFromISR(xMessageBuffer->message_buffer.handle,
                                          pvTxData.buf, pvTxData.len,
                                          &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xMessageBufferSendFromISR_obj,
                          mp_xMessageBufferSendFromISR);


static mp_obj_t mp_xMessageBufferReceive(mp_obj_t xMessageBuffer_in, mp_obj_t pvRxData_in,
                                        mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_message_buffer_t *xMessageBuffer = MP_OBJ_TO_PTR(xMessageBuffer_in);
    mp_buffer_info_t pvRxData;
    mp_get_buffer_raise(pvRxData_in, &pvRxData, MP_BUFFER_WRITE);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);

    size_t ret = xMessageBufferReceive(xMessageBuffer->message_buffer.handle,
                                   pvRxData.buf, pvRxData.len, xTicksToWait);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xMessageBufferReceive_obj, mp_xMessageBufferReceive);


static mp_obj_t mp_xMessageBufferReceiveFromISR(mp_obj_t xMessageBuffer_in,
                                               mp_obj_t pvRxData_in)
{
    mp_obj_freertos_message_buffer_t *xMessageBuffer = MP_OBJ_TO_PTR(xMessageBuffer_in);
    mp_buffer_info_t pvRxData;
    mp_get_buffer_raise(pvRxData_in, &pvRxData, MP_BUFFER_READ);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    size_t ret = xMessageBufferReceiveFromISR(xMessageBuffer->message_buffer.handle,
                                             pvRxData.buf, pvRxData.len,
                                             &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xMessageBufferReceiveFromISR_obj,
                          mp_xMessageBufferReceiveFromISR);


static mp_obj_t mp_vMessageBufferDelete(mp_obj_t xMessageBuffer_in)
{
    mp_obj_freertos_message_buffer_t *xMessageBuffer = MP_OBJ_TO_PTR(xMessageBuffer_in);

    vMessageBufferDelete(xMessageBuffer->message_buffer.handle);

    if (xMessageBuffer->pucStreamBufferStorageArea != NULL) {
        free(xMessageBuffer->pucStreamBufferStorageArea);
        xMessageBuffer->pucStreamBufferStorageArea = NULL;
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vMessageBufferDelete_obj, mp_vMessageBufferDelete);



#define xMessageBufferIsFull( xMessageBuffer ) \
    xStreamBufferIsFull( xMessageBuffer )
static mp_obj_t mp_xMessageBufferIsFull(mp_obj_t xMessageBuffer_in)
{
    mp_obj_freertos_message_buffer_t *xMessageBuffer = MP_OBJ_TO_PTR(xMessageBuffer_in);

    BaseType_t ret = xMessageBufferIsFull(xMessageBuffer->message_buffer.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xMessageBufferIsFull_obj, mp_xMessageBufferIsFull);


static mp_obj_t mp_xMessageBufferIsEmpty(mp_obj_t xMessageBuffer_in)
{
    mp_obj_freertos_message_buffer_t *xMessageBuffer = MP_OBJ_TO_PTR(xMessageBuffer_in);

    BaseType_t ret = xStreamBufferIsEmpty(xMessageBuffer->message_buffer.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xMessageBufferIsEmpty_obj, mp_xMessageBufferIsEmpty);


static mp_obj_t mp_xMessageBufferReset(mp_obj_t xMessageBuffer_in)
{
    mp_obj_freertos_message_buffer_t *xMessageBuffer = MP_OBJ_TO_PTR(xMessageBuffer_in);

    BaseType_t ret = xMessageBufferReset(xMessageBuffer->message_buffer.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xMessageBufferReset_obj, mp_xMessageBufferReset);


static mp_obj_t mp_xMessageBufferSpacesAvailable(mp_obj_t xMessageBuffer_in)
{
    mp_obj_freertos_message_buffer_t *xMessageBuffer = MP_OBJ_TO_PTR(xMessageBuffer_in);

    size_t ret = xMessageBufferSpacesAvailable(xMessageBuffer->message_buffer.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xMessageBufferSpacesAvailable_obj,
                          mp_xMessageBufferSpacesAvailable);



static mp_obj_t mp_xMessageBufferNextLengthBytes(mp_obj_t xMessageBuffer_in)
{
    mp_obj_freertos_message_buffer_t *xMessageBuffer = MP_OBJ_TO_PTR(xMessageBuffer_in);

    size_t ret = xMessageBufferNextLengthBytes(xMessageBuffer->message_buffer.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xMessageBufferNextLengthBytes_obj,
                          mp_xMessageBufferNextLengthBytes);


static mp_obj_t mp_xMessageBufferSendCompletedFromISR(mp_obj_t xMessageBuffer_in)
{
    mp_obj_freertos_message_buffer_t *xMessageBuffer = MP_OBJ_TO_PTR(xMessageBuffer_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xMessageBufferSendCompletedFromISR(xMessageBuffer->message_buffer.handle,
                                                       &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xMessageBufferSendCompletedFromISR_obj,
                          mp_xMessageBufferSendCompletedFromISR);


static mp_obj_t mp_xMessageBufferReceiveCompletedFromISR(mp_obj_t xMessageBuffer_in)
{
    mp_obj_freertos_message_buffer_t *xMessageBuffer = MP_OBJ_TO_PTR(xMessageBuffer_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xMessageBufferReceiveCompletedFromISR(xMessageBuffer->message_buffer.handle,
                                                          &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken)
    };

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xMessageBufferReceiveCompletedFromISR_obj,
                          mp_xMessageBufferReceiveCompletedFromISR);