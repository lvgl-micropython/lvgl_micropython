
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


#ifndef __QUEUE_H__
    #define __QUEUE_H__

    typedef struct _freertos_queue_t {
        QueueHandle_t handle;
        StaticQueue_t buffer;
    } freertos_queue_t;


    typedef struct _mp_obj_freertos_queue_t {
        freertos_queue_t queue;
        mp_freertos_types type;
        uint8_t *pucQueueStorage;
    } mp_obj_freertos_queue_t;


    typedef struct _freertos_queue_set_t {
        QueueSetHandle_t handle;
        StaticQueue_t buffer;
    } freertos_queue_set_t;


    typedef struct _mp_obj_freertos_queue_set_t {
        freertos_queue_set_t queue_set;
        mp_freertos_types type;

    } mp_obj_freertos_queue_set_t;


    typedef struct _freertos_queue_set_member_t {
        QueueSetMemberHandle_t handle;
        StaticQueue_t buffer;
    } freertos_queue_set_member_t;

    typedef struct _mp_obj_freertos_queue_set_member_t {
        freertos_queue_set_member_t queue_set_member;
        mp_freertos_types type;
    } mp_obj_freertos_queue_set_member_t;


    extern const mp_obj_fun_builtin_fixed_t mp_xQueueCreateStatic_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueSendToFront_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueSendToBack_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueSend_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueOverwrite_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueuePeek_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueuePeekFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueReceive_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxQueueMessagesWaiting_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxQueueSpacesAvailable_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vQueueDelete_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueSendToFrontFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueSendToBackFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueOverwriteFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueSendFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueGiveFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueReceiveFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueIsQueueEmptyFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueIsQueueFullFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_uxQueueMessagesWaitingFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueCreateMutexStatic_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueCreateMutexRecursiveStatic_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueCreateCountingSemaphoreStatic_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueSemaphoreTake_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueGetMutexHolder_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueGetMutexHolderFromISR_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueReset_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vQueueAddToRegistry_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_vQueueUnregisterQueue_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_pcQueueGetName_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueCreateSet_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueAddToSet_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueRemoveFromSet_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueSelectFromSet_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_xQueueSelectFromSetFromISR_obj;

#endif
