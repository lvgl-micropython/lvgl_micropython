
#include <stdint.h>
#include <stdlib.h>

#include "mpy_queue.h"
#include "mpy_semphr.h"
#include "mpy_event_groups.h"
#include "mpy_task.h"


#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"


static mp_obj_t mp_xQueueCreateStatic(mp_obj_t uxQueueLength_in)
{
    mp_obj_freertos_queue_t *self = m_new_obj(mp_obj_freertos_queue_t);
    self->type = mp_freertos_queue_type;

    UBaseType_t uxQueueLength = (UBaseType_t)mp_obj_get_int_truncated(uxQueueLength_in);

    self->pucQueueStorage = malloc(sizeof(void *) * (size_t)uxQueueLength);

    self->queue.handle = xQueueCreateStatic(uxQueueLength, (UBaseType_t)sizeof(void *),
                                            self->pucQueueStorage, &self->queue.buffer);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xQueueCreateStatic_obj, mp_xQueueCreateStatic);



// BaseType_t xQueueGenericSend( QueueHandle_t xQueue, const void * const pvItemToQueue, TickType_t xTicksToWait, const BaseType_t xCopyPosition );
static mp_obj_t mp_xQueueSendToFront(mp_obj_t xQueue_in, mp_obj_t pvItemToQueue_in,
                                     mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    void *pvItemToQueue = MP_OBJ_TO_PTR(pvItemToQueue_in);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);

    BaseType_t ret = xQueueSendToFront(xQueue->queue.handle, pvItemToQueue, xTicksToWait);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xQueueSendToFront_obj, mp_xQueueSendToFront);


static mp_obj_t mp_xQueueSendToBack(mp_obj_t xQueue_in, mp_obj_t pvItemToQueue_in,
                                    mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    void *pvItemToQueue = MP_OBJ_TO_PTR(pvItemToQueue_in);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);

    BaseType_t ret = xQueueSendToBack(xQueue->queue.handle, pvItemToQueue, xTicksToWait);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xQueueSendToBack_obj, mp_xQueueSendToBack);


static mp_obj_t mp_xQueueSend(mp_obj_t xQueue_in, mp_obj_t pvItemToQueue_in,
                              mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    void *pvItemToQueue = MP_OBJ_TO_PTR(pvItemToQueue_in);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);

    BaseType_t ret = xQueueSend(xQueue->queue.handle, pvItemToQueue, xTicksToWait);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_xQueueSend_obj, mp_xQueueSend);


static mp_obj_t mp_xQueueOverwrite(mp_obj_t xQueue_in, mp_obj_t pvItemToQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    void *pvItemToQueue = MP_OBJ_TO_PTR(pvItemToQueue_in);

    BaseType_t ret = xQueueOverwrite(xQueue->queue.handle, pvItemToQueue);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xQueueOverwrite_obj, mp_xQueueOverwrite);


static mp_obj_t mp_xQueuePeek(mp_obj_t xQueue_in, mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    void *pvBuffer = NULL;
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);

    BaseType_t ret = xQueuePeek(xQueue->queue.handle, pvBuffer, xTicksToWait);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_const_none,
    };

    if (pvBuffer != NULL) tuple[1] = MP_OBJ_FROM_PTR(pvBuffer);

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xQueuePeek_obj, mp_xQueuePeek);


static mp_obj_t mp_xQueuePeekFromISR(mp_obj_t xQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    void *pvBuffer = NULL;

    BaseType_t ret = xQueuePeekFromISR(xQueue->queue.handle, pvBuffer);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_const_none,
    };

    if (pvBuffer != NULL) tuple[1] = MP_OBJ_FROM_PTR(pvBuffer);

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xQueuePeekFromISR_obj, mp_xQueuePeekFromISR);


static mp_obj_t mp_xQueueReceive(mp_obj_t xQueue_in, mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    void *pvBuffer = NULL;
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);

    BaseType_t ret = xQueueReceive(xQueue->queue.handle, pvBuffer, xTicksToWait);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_const_none,
    };

    if (pvBuffer != NULL) tuple[1] = MP_OBJ_FROM_PTR(pvBuffer);

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xQueueReceive_obj, mp_xQueueReceive);


static mp_obj_t mp_uxQueueMessagesWaiting(mp_obj_t xQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);

    UBaseType_t ret = uxQueueMessagesWaiting(xQueue->queue.handle);
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_uxQueueMessagesWaiting_obj, mp_uxQueueMessagesWaiting);


static mp_obj_t mp_uxQueueSpacesAvailable(mp_obj_t xQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);

    UBaseType_t ret = uxQueueSpacesAvailable(xQueue->queue.handle);
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_uxQueueSpacesAvailable_obj, mp_uxQueueSpacesAvailable);


static mp_obj_t mp_vQueueDelete(mp_obj_t xQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    vQueueDelete(xQueue->queue.handle);

    free(xQueue->pucQueueStorage);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vQueueDelete_obj, mp_vQueueDelete);

// BaseType_t xQueueGenericSendFromISR( QueueHandle_t xQueue, const void * const pvItemToQueue, BaseType_t * const pxHigherPriorityTaskWoken, const BaseType_t xCopyPosition );

static mp_obj_t mp_xQueueSendToFrontFromISR(mp_obj_t xQueue_in, mp_obj_t pvItemToQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    void *pvItemToQueue = MP_OBJ_TO_PTR(pvItemToQueue_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xQueueSendToFrontFromISR(xQueue->queue.handle, pvItemToQueue,
                                              &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken),
    };

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xQueueSendToFrontFromISR_obj, mp_xQueueSendToFrontFromISR);


static mp_obj_t mp_xQueueSendToBackFromISR(mp_obj_t xQueue_in, mp_obj_t pvItemToQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    void *pvItemToQueue = MP_OBJ_TO_PTR(pvItemToQueue_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xQueueSendToBackFromISR(xQueue->queue.handle, pvItemToQueue,
                                             &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken),
    };

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xQueueSendToBackFromISR_obj, mp_xQueueSendToBackFromISR);


static mp_obj_t mp_xQueueOverwriteFromISR(mp_obj_t xQueue_in, mp_obj_t pvItemToQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    void *pvItemToQueue = MP_OBJ_TO_PTR(pvItemToQueue_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xQueueOverwriteFromISR(xQueue->queue.handle, pvItemToQueue,
                                            &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken),
    };

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xQueueOverwriteFromISR_obj, mp_xQueueOverwriteFromISR);


static mp_obj_t mp_xQueueSendFromISR(mp_obj_t xQueue_in, mp_obj_t pvItemToQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    void *pvItemToQueue = MP_OBJ_TO_PTR(pvItemToQueue_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xQueueSendFromISR(xQueue->queue.handle, pvItemToQueue,
                                       &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken),
    };

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xQueueSendFromISR_obj, mp_xQueueSendFromISR);


static mp_obj_t mp_xQueueGiveFromISR(mp_obj_t xQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xQueueGiveFromISR(xQueue->queue.handle, &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[2] = {
        mp_obj_new_int((int)ret),
        mp_obj_new_int((int)pxHigherPriorityTaskWoken),
    };

    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xQueueGiveFromISR_obj, mp_xQueueGiveFromISR);


static mp_obj_t mp_xQueueReceiveFromISR(mp_obj_t xQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    void *pvBuffer = NULL;
    BaseType_t pxHigherPriorityTaskWoken = 0;

    BaseType_t ret = xQueueReceiveFromISR(xQueue->queue.handle, pvBuffer,
                                          &pxHigherPriorityTaskWoken);

    mp_obj_t tuple[3] = {
        mp_obj_new_int((int)ret),
        mp_const_none,
        mp_obj_new_int((int)pxHigherPriorityTaskWoken),
    };

    if (pvBuffer != NULL) tuple[1] = MP_OBJ_FROM_PTR(pvBuffer);

    return mp_obj_new_tuple(3, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xQueueReceiveFromISR_obj, mp_xQueueReceiveFromISR);


static mp_obj_t mp_xQueueIsQueueEmptyFromISR(mp_obj_t xQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    BaseType_t ret = xQueueIsQueueEmptyFromISR(xQueue->queue.handle);

    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xQueueIsQueueEmptyFromISR_obj, mp_xQueueIsQueueEmptyFromISR);


static mp_obj_t mp_xQueueIsQueueFullFromISR(mp_obj_t xQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    BaseType_t ret = xQueueIsQueueFullFromISR(xQueue->queue.handle);

    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xQueueIsQueueFullFromISR_obj, mp_xQueueIsQueueFullFromISR);


static mp_obj_t mp_uxQueueMessagesWaitingFromISR(mp_obj_t xQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    UBaseType_t ret = uxQueueMessagesWaitingFromISR(xQueue->queue.handle);

    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_uxQueueMessagesWaitingFromISR_obj,
                          mp_uxQueueMessagesWaitingFromISR);


static mp_obj_t mp_xQueueCreateMutexStatic(void)
{
    mp_obj_freertos_queue_t *self = m_new_obj(mp_obj_freertos_queue_t);
    self->type = mp_freertos_semaphore_type;
    
    self->queue.handle = xQueueCreateMutexStatic(queueQUEUE_TYPE_MUTEX, &self->queue.buffer);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xQueueCreateMutexStatic_obj, mp_xQueueCreateMutexStatic);


static mp_obj_t mp_xQueueCreateMutexRecursiveStatic(void)
{
    mp_obj_freertos_queue_t *self = m_new_obj(mp_obj_freertos_queue_t);
    self->type = mp_freertos_semaphore_type;
    
    self->queue.handle = xQueueCreateMutexStatic(queueQUEUE_TYPE_RECURSIVE_MUTEX,
                                                 &self->queue.buffer);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xQueueCreateMutexRecursiveStatic_obj,
                          mp_xQueueCreateMutexRecursiveStatic);


static mp_obj_t mp_xQueueCreateCountingSemaphoreStatic(mp_obj_t uxMaxCount_in,
                                                       mp_obj_t uxInitialCount_in)
{
    mp_obj_freertos_queue_t *self = m_new_obj(mp_obj_freertos_queue_t);
    self->type = mp_freertos_semaphore_type;

    UBaseType_t uxMaxCount = (UBaseType_t)mp_obj_get_int_truncated(uxMaxCount_in);
    UBaseType_t uxInitialCount = (UBaseType_t)mp_obj_get_int_truncated(uxInitialCount_in);

    self->queue.handle = xQueueCreateCountingSemaphoreStatic(uxMaxCount, uxInitialCount,
                                                             &self->queue.buffer);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xQueueCreateCountingSemaphoreStatic_obj,
                          mp_xQueueCreateCountingSemaphoreStatic);



static mp_obj_t mp_xQueueSemaphoreTake(mp_obj_t xQueue_in, mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);
    BaseType_t ret = xQueueSemaphoreTake(xQueue->queue.handle, xTicksToWait);

    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xQueueSemaphoreTake_obj, mp_xQueueSemaphoreTake);


static mp_obj_t mp_xQueueGetMutexHolder(mp_obj_t xSemaphore_in)
{
    mp_obj_freertos_queue_t *xSemaphore = MP_OBJ_TO_PTR(xSemaphore_in);
    TaskHandle_t task_handle = xQueueGetMutexHolder(xSemaphore->queue.handle);

    mp_obj_freertos_task_t *mp_task = (mp_obj_freertos_task_t *)task_handle;
    return MP_OBJ_FROM_PTR(mp_task);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xQueueGetMutexHolder_obj, mp_xQueueGetMutexHolder);


static mp_obj_t mp_xQueueGetMutexHolderFromISR(mp_obj_t xSemaphore_in)
{
    mp_obj_freertos_queue_t *xSemaphore = MP_OBJ_TO_PTR(xSemaphore_in);

    TaskHandle_t task_handle = xQueueGetMutexHolderFromISR(xSemaphore->queue.handle);
    mp_obj_freertos_task_t *mp_task = (mp_obj_freertos_task_t *)task_handle;

    return MP_OBJ_FROM_PTR(mp_task);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xQueueGetMutexHolderFromISR_obj, mp_xQueueGetMutexHolderFromISR);


static mp_obj_t mp_xQueueReset(mp_obj_t xQueue_in)
{
    mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);

    BaseType_t ret = xQueueReset(xQueue->queue.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xQueueReset_obj, mp_xQueueReset);


#if ( configQUEUE_REGISTRY_SIZE > 0 )
    static mp_obj_t mp_vQueueAddToRegistry(mp_obj_t xQueue_in, mp_obj_t pcQueueName_in)
    {
        mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);

        size_t str_len;
        const char *pcQueueName = mp_obj_str_get_data(pcQueueName_in, &str_len);

        vQueueAddToRegistry(xQueue->queue.handle, pcQueueName);

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_2(mp_vQueueAddToRegistry_obj, mp_vQueueAddToRegistry);

    static mp_obj_t mp_vQueueUnregisterQueue(mp_obj_t xQueue_in)
    {
        mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);

        vQueueUnregisterQueue(xQueue->queue.handle);
        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_1(mp_vQueueUnregisterQueue_obj, mp_vQueueUnregisterQueue);

    static mp_obj_t mp_pcQueueGetName(mp_obj_t xQueue_in)
    {
        mp_obj_freertos_queue_t *xQueue = MP_OBJ_TO_PTR(xQueue_in);

        const char *ret = pcQueueGetName(xQueue->queue.handle);

        return mp_obj_new_str_from_cstr(ret);
    }

    MP_DEFINE_CONST_FUN_OBJ_1(mp_pcQueueGetName_obj, mp_pcQueueGetName);

#endif


// BaseType_t xQueueGenericGetStaticBuffers( QueueHandle_t xQueue, uint8_t ** ppucQueueStorage, StaticQueue_t ** ppxStaticQueue );


static mp_obj_t mp_xQueueCreateSet(mp_obj_t uxEventQueueLength_in)
{
    mp_obj_freertos_queue_set_t *self = m_new_obj(mp_obj_freertos_queue_set_t);
    self->type = mp_freertos_queue_set_type;

    UBaseType_t uxEventQueueLength = (UBaseType_t)mp_obj_get_int_truncated(uxEventQueueLength_in);

    self->queue_set.handle = xQueueCreateSet(uxEventQueueLength);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xQueueCreateSet_obj, mp_xQueueCreateSet);


static mp_obj_t mp_xQueueAddToSet(mp_obj_t xQueueOrSemaphore_in, mp_obj_t xQueueSet_in)
{
    mp_obj_freertos_queue_set_member_t *xQueueOrSemaphore = MP_OBJ_TO_PTR(xQueueOrSemaphore_in);
    mp_obj_freertos_queue_set_t *xQueueSet = MP_OBJ_TO_PTR(xQueueSet_in);

    BaseType_t ret = xQueueAddToSet(xQueueOrSemaphore->queue_set_member.handle,
                                    xQueueSet->queue_set.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xQueueAddToSet_obj, mp_xQueueAddToSet);


static mp_obj_t mp_xQueueRemoveFromSet(mp_obj_t xQueueOrSemaphore_in,
                                       mp_obj_t xQueueSet_in)
{
    mp_obj_freertos_queue_set_member_t *xQueueOrSemaphore = MP_OBJ_TO_PTR(xQueueOrSemaphore_in);
    mp_obj_freertos_queue_set_t *xQueueSet = MP_OBJ_TO_PTR(xQueueSet_in);

    BaseType_t ret = xQueueRemoveFromSet(xQueueOrSemaphore->queue_set_member.handle,
                                         xQueueSet->queue_set.handle);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xQueueRemoveFromSet_obj, mp_xQueueRemoveFromSet);


static mp_obj_t mp_xQueueSelectFromSet(mp_obj_t xQueueSet_in, mp_obj_t xTicksToWait_in)
{
    mp_obj_freertos_queue_set_t *xQueueSet = MP_OBJ_TO_PTR(xQueueSet_in);
    TickType_t xTicksToWait = (TickType_t)mp_obj_get_int_truncated(xTicksToWait_in);

    QueueSetMemberHandle_t member_handle = xQueueSelectFromSet(xQueueSet->queue_set.handle,
                                                               xTicksToWait);

    mp_obj_freertos_queue_set_member_t *mp_member = (mp_obj_freertos_queue_set_member_t *)member_handle;

    if (mp_member->type == mp_freertos_event_group_type) {
        mp_obj_freertos_event_group_t *event_group = (mp_obj_freertos_event_group_t *)mp_member;
        return MP_OBJ_FROM_PTR(event_group);
    } else if (mp_member->type == mp_freertos_queue_type) {
        mp_obj_freertos_queue_t *queue = (mp_obj_freertos_queue_t *)mp_member;
        return MP_OBJ_FROM_PTR(queue);
    } else if (mp_member->type == mp_freertos_queue_set_type) {
        mp_obj_freertos_queue_set_t *queue_set = (mp_obj_freertos_queue_set_t *)mp_member;
        return MP_OBJ_FROM_PTR(queue_set);
    } else if (mp_member->type == mp_freertos_semaphore_type) {
        mp_obj_freertos_semaphore_t *semaphore = (mp_obj_freertos_semaphore_t *)mp_member;
        return MP_OBJ_FROM_PTR(semaphore);
    } else {
        return mp_const_none;
    }
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xQueueSelectFromSet_obj, mp_xQueueSelectFromSet);


static mp_obj_t mp_xQueueSelectFromSetFromISR(mp_obj_t xQueueSet_in)
{
    mp_obj_freertos_queue_set_t *xQueueSet = MP_OBJ_TO_PTR(xQueueSet_in);
    QueueSetMemberHandle_t member_handle = xQueueSelectFromSetFromISR(xQueueSet->queue_set.handle);

    mp_obj_freertos_queue_set_member_t *mp_member = (mp_obj_freertos_queue_set_member_t *)member_handle;

    if (mp_member->type == mp_freertos_event_group_type) {
        mp_obj_freertos_event_group_t *event_group = (mp_obj_freertos_event_group_t *)mp_member;
        return MP_OBJ_FROM_PTR(event_group);
    } else if (mp_member->type == mp_freertos_queue_type) {
        mp_obj_freertos_queue_t *queue = (mp_obj_freertos_queue_t *)mp_member;
        return MP_OBJ_FROM_PTR(queue);
    } else if (mp_member->type == mp_freertos_queue_set_type) {
        mp_obj_freertos_queue_set_t *queue_set = (mp_obj_freertos_queue_set_t *)mp_member;
        return MP_OBJ_FROM_PTR(queue_set);
    } else if (mp_member->type == mp_freertos_semaphore_type) {
        mp_obj_freertos_semaphore_t *semaphore = (mp_obj_freertos_semaphore_t *)mp_member;
        return MP_OBJ_FROM_PTR(semaphore);
    } else {
        return mp_const_none;
    }
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_xQueueSelectFromSetFromISR_obj, mp_xQueueSelectFromSetFromISR);
