#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"
#include "mpy_idf_additions.h"
#include "mpy_stream_buffer.h"
#include "mpy_message_buffer.h"
#include "mpy_task.h"
#include "mpy_semphr.h"
#include "mpy_event_groups.h"
#include "mpy_queue.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"

static mp_obj_t mp_xTaskCreateStaticPinnedToCore(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused

    mp_obj_freertos_task_t *self = m_new_obj(mp_obj_freertos_task_t);
    self->type = mp_freertos_task_type;

    self->callback = args[0];

    size_t str_len;
    const char *pcName = mp_obj_str_get_data(args[1], &str_len);

    uint32_t ulStackDepth = (uint32_t)mp_obj_get_int_truncated(args[2]);
    self->pvParameters = args[3];

    UBaseType_t uxPriority = (UBaseType_t)mp_obj_get_int_truncated(args[4]);

    if (args[5] != mp_const_none) {
        self->puxStackBuffer = malloc(ulStackDepth * sizeof(StackType_t));

        mp_obj_tuple_t *puxStackBuffer = MP_OBJ_TO_PTR(args[5]);

        for (uint32_t i=0; i<ulStackDepth; i++) {
            StackType_t *stack_item = (StackType_t *)MP_OBJ_FROM_PTR(puxStackBuffer->items[i]);
            self->puxStackBuffer[i] = *stack_item;
        }
    } else {
        self->puxStackBuffer = NULL;
    }

    BaseType_t xCoreID = (BaseType_t)mp_obj_get_int(args[6]);

    self->task.handle = xTaskCreateStaticPinnedToCore(&xTask_cb, pcName, ulStackDepth, self,
                                                      uxPriority, self->puxStackBuffer,
                                                      &self->task.buffer, xCoreID);


    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_xTaskCreateStaticPinnedToCore_obj, 7, 7,
                                    mp_xTaskCreateStaticPinnedToCore);


static mp_obj_t mp_xTaskGetCoreID(mp_obj_t xTask_in)
{

    mp_obj_freertos_task_t *xTask = (mp_obj_freertos_task_t *)MP_OBJ_TO_PTR(xTask_in);
    BaseType_t ret = xTaskGetCoreID(xTask->task.handle);

    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xTaskGetCoreID_obj, mp_xTaskGetCoreID);



#if !CONFIG_FREERTOS_SMP
    #if INCLUDE_xTaskGetIdleTaskHandle == 1

        static mp_obj_t mp_xTaskGetIdleTaskHandleForCore(mp_obj_t xCoreID_in)
        {
            BaseType_t xCoreID = (BaseType_t)mp_obj_get_int(xCoreID_in);
            mp_obj_freertos_task_t *task = (mp_obj_freertos_task_t *)xTaskGetIdleTaskHandleForCore(xCoreID);

            return MP_OBJ_FROM_PTR(task);
        }

        MP_DEFINE_CONST_FUN_OBJ_1(mp_xTaskGetIdleTaskHandleForCore_obj,
                                  mp_xTaskGetIdleTaskHandleForCore);


        // #if configGENERATE_RUN_TIME_STATS == 1
        //     configRUN_TIME_COUNTER_TYPE ulTaskGetIdleRunTimeCounterForCore( BaseType_t xCoreID );
        //
        //     configRUN_TIME_COUNTER_TYPE ulTaskGetIdleRunTimePercentForCore( BaseType_t xCoreID );
        // #endif

    #endif

    #if INCLUDE_xTaskGetIdleTaskHandle == 1 || configUSE_MUTEXES == 1
        static mp_obj_t mp_xTaskGetCurrentTaskHandleForCore(mp_obj_t xCoreID_in)
        {
            BaseType_t xCoreID = (BaseType_t)mp_obj_get_int(xCoreID_in);
            mp_obj_freertos_task_t *task = (mp_obj_freertos_task_t *)xTaskGetCurrentTaskHandleForCore(xCoreID);

            return MP_OBJ_FROM_PTR(task);
        }

        MP_DEFINE_CONST_FUN_OBJ_1(mp_xTaskGetCurrentTaskHandleForCore_obj,
                                  mp_xTaskGetCurrentTaskHandleForCore);

    #endif

#endif

// uint8_t * pxTaskGetStackStart( TaskHandle_t xTask );


// #if CONFIG_FREERTOS_TLSP_DELETION_CALLBACKS
//     typedef void (* TlsDeleteCallbackFunction_t)( int, void * );
//
//     void vTaskSetThreadLocalStoragePointerAndDelCallback( TaskHandle_t xTaskToSet,
//                                                           BaseType_t xIndex,
//                                                           void * pvValue,
//                                                           TlsDeleteCallbackFunction_t pvDelCallback );
// #endif


static mp_obj_t mp_xTaskCreatePinnedToCoreWithCaps(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused

    mp_obj_freertos_task_t *self = m_new_obj(mp_obj_freertos_task_t);
    self->type = mp_freertos_task_type;

    self->callback = args[0];

    size_t str_len;
    const char *pcName = mp_obj_str_get_data(args[1], &str_len);

    uint32_t usStackDepth = (uint32_t)mp_obj_get_int_truncated(args[2]);
    self->pvParameters = args[3];
    UBaseType_t uxPriority = (UBaseType_t)mp_obj_get_int_truncated(args[4]);
    BaseType_t xCoreID = (BaseType_t)mp_obj_get_int(args[5]);
    UBaseType_t uxMemoryCaps = (UBaseType_t)mp_obj_get_int_truncated(args[6]);

    BaseType_t ret = xTaskCreatePinnedToCoreWithCaps(&xTask_cb, pcName, usStackDepth, self,
                                                     uxPriority, &self->task.handle,
                                                     xCoreID, uxMemoryCaps);

    if (ret != pdTRUE) {
        m_free(self);
        return mp_const_none;
    } else {
        return MP_OBJ_FROM_PTR(self);
    }
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_xTaskCreatePinnedToCoreWithCaps_obj, 7, 7,
                                    mp_xTaskCreatePinnedToCoreWithCaps);


static mp_obj_t mp_xTaskCreateWithCaps(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused

    mp_obj_freertos_task_t *self = m_new_obj(mp_obj_freertos_task_t);
    self->type = mp_freertos_task_type;

    self->callback = args[0];

    size_t str_len;
    const char *pcName = mp_obj_str_get_data(args[1], &str_len);

    uint32_t ulStackDepth = (uint32_t)mp_obj_get_int_truncated(args[2]);
    self->pvParameters = args[3];
    UBaseType_t uxPriority = (UBaseType_t)mp_obj_get_int_truncated(args[4]);
    UBaseType_t uxMemoryCaps = (UBaseType_t)mp_obj_get_int_truncated(args[5]);

    BaseType_t ret = xTaskCreateWithCaps(&xTask_cb, pcName, ulStackDepth, self, uxPriority,
                                         &self->task.handle, uxMemoryCaps);

    if (ret != pdTRUE) {
        m_free(self);
        return mp_const_none;
    } else {
        return MP_OBJ_FROM_PTR(self);
    }
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_xTaskCreateWithCaps_obj, 6, 6, mp_xTaskCreateWithCaps);


static mp_obj_t mp_vTaskDeleteWithCaps(mp_obj_t xTaskToDelete_in)
{
    mp_obj_freertos_task_t *xTaskToDelete = MP_OBJ_TO_PTR(xTaskToDelete_in);

    vTaskDeleteWithCaps(xTaskToDelete->task.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vTaskDeleteWithCaps_obj, mp_vTaskDeleteWithCaps);



mp_obj_t mp_xQueueCreateWithCaps(mp_obj_t uxQueueLength_in, mp_obj_t uxMemoryCaps_in)
{
    mp_obj_freertos_queue_t *self = m_new_obj(mp_obj_freertos_queue_t);
    self->type = mp_freertos_queue_type;

    UBaseType_t uxQueueLength = (UBaseType_t)mp_obj_get_int_truncated(uxQueueLength_in);
    UBaseType_t uxMemoryCaps = (UBaseType_t)mp_obj_get_int_truncated(uxMemoryCaps_in);

    self->queue.handle = xQueueCreateWithCaps(uxQueueLength, (UBaseType_t)sizeof(void *),
                                              uxMemoryCaps);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xQueueCreateWithCaps_obj, mp_xQueueCreateWithCaps);


static mp_obj_t mp_vQueueDeleteWithCaps(mp_obj_t xQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);

    vQueueDeleteWithCaps(xQueue->queue.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vQueueDeleteWithCaps_obj, mp_vQueueDeleteWithCaps);



// SemaphoreHandle_t xSemaphoreCreateGenericWithCaps( UBaseType_t uxMaxCount,
//                                                    UBaseType_t uxInitialCount,
//                                                    const uint8_t ucQueueType,
//                                                    UBaseType_t uxMemoryCaps );


mp_obj_t mp_xSemaphoreCreateBinaryWithCaps(mp_obj_t uxMemoryCaps_in)
{
    mp_obj_freertos_semaphore_t *self = m_new_obj(mp_obj_freertos_semaphore_t);
    self->type = mp_freertos_semaphore_type;

    UBaseType_t uxMemoryCaps = (UBaseType_t)mp_obj_get_int_truncated(uxMemoryCaps_in);

    self->semaphore.handle = xSemaphoreCreateBinaryWithCaps(uxMemoryCaps);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xSemaphoreCreateBinaryWithCaps_obj,
                          mp_xSemaphoreCreateBinaryWithCaps);


mp_obj_t mp_xSemaphoreCreateCountingWithCaps(mp_obj_t uxMaxCount_in, mp_obj_t
                                             uxInitialCount_in,
                                             mp_obj_t uxMemoryCaps_in)
{
    mp_obj_freertos_semaphore_t *self = m_new_obj(mp_obj_freertos_semaphore_t);
    self->type = mp_freertos_semaphore_type;

    UBaseType_t uxMaxCount = (UBaseType_t)mp_obj_get_int_truncated(uxMaxCount_in);
    UBaseType_t uxInitialCount = (UBaseType_t)mp_obj_get_int_truncated(uxInitialCount_in);
    UBaseType_t uxMemoryCaps = (UBaseType_t)mp_obj_get_int_truncated(uxMemoryCaps_in);

    self->semaphore.handle = xSemaphoreCreateCountingWithCaps(uxMaxCount, uxInitialCount,
                                                          uxMemoryCaps);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xSemaphoreCreateCountingWithCaps_obj,
                          mp_xSemaphoreCreateCountingWithCaps);


mp_obj_t mp_xSemaphoreCreateMutexWithCaps(mp_obj_t uxMemoryCaps_in)
{
    mp_obj_freertos_semaphore_t *self = m_new_obj(mp_obj_freertos_semaphore_t);
    self->type = mp_freertos_semaphore_type;

    UBaseType_t uxMemoryCaps = (UBaseType_t)mp_obj_get_int_truncated(uxMemoryCaps_in);

    self->semaphore.handle = xSemaphoreCreateMutexWithCaps(uxMemoryCaps);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xSemaphoreCreateMutexWithCaps_obj,
                          mp_xSemaphoreCreateMutexWithCaps);


mp_obj_t mp_xSemaphoreCreateRecursiveMutexWithCaps(mp_obj_t uxMemoryCaps_in)
{
    mp_obj_freertos_semaphore_t *self = m_new_obj(mp_obj_freertos_semaphore_t);
    self->type = mp_freertos_semaphore_type;

    UBaseType_t uxMemoryCaps = (UBaseType_t)mp_obj_get_int_truncated(uxMemoryCaps_in);

    self->semaphore.handle = xSemaphoreCreateRecursiveMutexWithCaps(uxMemoryCaps);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xSemaphoreCreateRecursiveMutexWithCaps_obj,
                          mp_xSemaphoreCreateRecursiveMutexWithCaps);


static mp_obj_t mp_vSemaphoreDeleteWithCaps(mp_obj_t xSemaphore_in)
{
    mp_obj_freertos_semaphore_t *xSemaphore = MP_OBJ_TO_PTR(xSemaphore_in);

    vSemaphoreDeleteWithCaps(xSemaphore->semaphore.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vSemaphoreDeleteWithCaps_obj, mp_vSemaphoreDeleteWithCaps);

// StreamBufferHandle_t xStreamBufferGenericCreateWithCaps( size_t xBufferSizeBytes,
//                                                         size_t xTriggerLevelBytes,
//                                                         BaseType_t xIsMessageBuffer,
//                                                         UBaseType_t uxMemoryCaps );

// void vStreamBufferGenericDeleteWithCaps( StreamBufferHandle_t xStreamBuffer,
//                                         BaseType_t xIsMessageBuffer );


mp_obj_t mp_xStreamBufferCreateWithCaps(mp_obj_t xBufferSizeBytes_in,
                                        mp_obj_t xTriggerLevelBytes_in,
                                        mp_obj_t uxMemoryCaps_in)
{
    mp_obj_freertos_stream_buffer_t *self = m_new_obj(mp_obj_freertos_stream_buffer_t);
    self->type = mp_freertos_stream_buffer_type;

    size_t xBufferSizeBytes = (size_t)mp_obj_get_int_truncated(xBufferSizeBytes_in);
    size_t xTriggerLevelBytes = (size_t)mp_obj_get_int_truncated(xTriggerLevelBytes_in);
    UBaseType_t uxMemoryCaps = (UBaseType_t)mp_obj_get_int_truncated(uxMemoryCaps_in);

    self->stream_buffer.handle = xStreamBufferCreateWithCaps(xBufferSizeBytes,
                                                             xTriggerLevelBytes,
                                                             uxMemoryCaps);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xStreamBufferCreateWithCaps_obj, mp_xStreamBufferCreateWithCaps);


static mp_obj_t mp_vStreamBufferDeleteWithCaps(mp_obj_t xStreamBuffer_in)
{
    mp_obj_freertos_stream_buffer_t *xStreamBuffer = MP_OBJ_TO_PTR(xStreamBuffer_in);

    vStreamBufferDeleteWithCaps(xStreamBuffer->stream_buffer.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vStreamBufferDeleteWithCaps_obj, mp_vStreamBufferDeleteWithCaps);


mp_obj_t mp_xMessageBufferCreateWithCaps(mp_obj_t xBufferSizeBytes_in, mp_obj_t uxMemoryCaps_in)
{
    mp_obj_freertos_stream_buffer_t *self = m_new_obj(mp_obj_freertos_stream_buffer_t);
    self->type = mp_freertos_stream_buffer_type;

    size_t xBufferSizeBytes = (size_t)mp_obj_get_int_truncated(xBufferSizeBytes_in);
    UBaseType_t uxMemoryCaps = (UBaseType_t)mp_obj_get_int_truncated(uxMemoryCaps_in);

    self->stream_buffer.handle = xMessageBufferCreateWithCaps(xBufferSizeBytes, uxMemoryCaps);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xMessageBufferCreateWithCaps_obj, mp_xMessageBufferCreateWithCaps);


static mp_obj_t mp_vMessageBufferDeleteWithCaps(mp_obj_t xMessageBuffer_in)
{
    mp_obj_freertos_message_buffer_t *xMessageBuffer = MP_OBJ_TO_PTR(xMessageBuffer_in);

    vMessageBufferDeleteWithCaps(xMessageBuffer->message_buffer.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vMessageBufferDeleteWithCaps_obj, mp_vMessageBufferDeleteWithCaps);


mp_obj_t mp_xEventGroupCreateWithCaps(mp_obj_t uxMemoryCaps_in)
{
    mp_obj_freertos_event_group_t *self = m_new_obj(mp_obj_freertos_event_group_t);
    self->type = mp_freertos_event_group_type;

    UBaseType_t uxMemoryCaps = (UBaseType_t)mp_obj_get_int_truncated(uxMemoryCaps_in);

    self->event_group.handle = xEventGroupCreateWithCaps(uxMemoryCaps);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xEventGroupCreateWithCaps_obj, mp_xEventGroupCreateWithCaps);


static mp_obj_t mp_vEventGroupDeleteWithCaps(mp_obj_t xEventGroup_in)
{
    mp_obj_freertos_event_group_t *xEventGroup = MP_OBJ_TO_PTR(xEventGroup_in);

    vEventGroupDeleteWithCaps(xEventGroup->event_group.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vEventGroupDeleteWithCaps_obj, mp_vEventGroupDeleteWithCaps);