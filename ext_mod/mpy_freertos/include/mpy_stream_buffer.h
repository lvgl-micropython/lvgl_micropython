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


#ifndef __STREAM_BUFFER_H__
    #define __STREAM_BUFFER_H__

    typedef struct _freertos_stream_buffer_t {
        StreamBufferHandle_t handle;
        StaticStreamBuffer_t buffer;
    } freertos_stream_buffer_t;

    typedef struct _mp_obj_freertos_stream_buffer_t {
        freertos_stream_buffer_t stream_buffer;
        mp_freertos_types type;
        uint8_t *pucStreamBufferStorageArea;
    } mp_obj_freertos_stream_buffer_t;

    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferCreateStatic_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferSend_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferSendFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferReceive_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferReceiveFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vStreamBufferDelete_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferIsFull_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferIsEmpty_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferReset_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferSpacesAvailable_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferBytesAvailable_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferSetTriggerLevel_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferSendCompletedFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xStreamBufferReceiveCompletedFromISR_obj;


#endif