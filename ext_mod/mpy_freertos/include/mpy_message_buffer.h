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
#include "freertos/message_buffer.h"


#ifndef __MESSAGE_BUFFER_H__
    #define __MESSAGE_BUFFER_H__

    typedef struct _freertos_message_buffer_t {
        MessageBufferHandle_t handle;
        StaticMessageBuffer_t buffer;
    } freertos_message_buffer_t;

    typedef struct _mp_obj_freertos_message_buffer_t {
        freertos_message_buffer_t message_buffer;
        mp_freertos_types type;
        uint8_t *pucStreamBufferStorageArea;
    } mp_obj_freertos_message_buffer_t;

    extern const mp_obj_fun_builtin_fixed_t mp_xMessageBufferCreateStatic_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xMessageBufferSend_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xMessageBufferSendFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xMessageBufferReceive_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xMessageBufferReceiveFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vMessageBufferDelete_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xMessageBufferIsFull_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xMessageBufferIsEmpty_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xMessageBufferReset_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xMessageBufferSpacesAvailable_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xMessageBufferNextLengthBytes_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xMessageBufferSendCompletedFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xMessageBufferReceiveCompletedFromISR_obj;


#endif