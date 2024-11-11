#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"


#ifndef __THREAD_PORT_H__
    #define __THREAD_PORT_H__

    typedef struct _thread_lock_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
        volatile uint16_t ref_count;
    } thread_lock_t;

    typedef struct _thread_rlock_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
        volatile uint16_t ref_count;
    } thread_rlock_t;

    typedef struct _thread_semaphore_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
        volatile uint16_t ref_count;
    } thread_semaphore_t;

    typedef struct _thread_event_t {
        EventGroupHandle_t handle;
        StaticEventGroup_t buffer;
        volatile uint16_t ref_count;
        volatile bool is_set;
    } thread_event_t;

    typedef struct _thread_t {
        TaskHandle_t handle;
    } thread_t;

    void THREADING_FREERTOS_TASK_DELETE_HOOK(void *tcb);

#endif /* __THREAD_PORT_H__ */