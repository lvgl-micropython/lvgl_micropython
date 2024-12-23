#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/idf_additions.h"


#ifndef __BUS_TASK_H__
    #define __BUS_TASK_H__

    #define DEFAULT_STACK_SIZE    (5 * 1024)

    typedef struct _bus_lock_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    } bus_lock_t;

    typedef struct _bus_event_t {
        EventGroupHandle_t handle;
        StaticEventGroup_t buffer;
    } bus_event_t;


    void bus_event_init(bus_event_t *event);
    void bus_event_delete(bus_event_t *event);
    bool bus_event_isset(bus_event_t *event);
    void bus_event_set(bus_event_t *event);
    void bus_event_clear(bus_event_t *event);
    void bus_event_clear_from_isr(bus_event_t *event);
    bool bus_event_isset_from_isr(bus_event_t *event);
    void bus_event_set_from_isr(bus_event_t *event);
    void bus_event_wait(bus_event_t *event);

    int  bus_lock_acquire(bus_lock_t *lock, int32_t wait_ms);
    void bus_lock_release(bus_lock_t *lock);
    void bus_lock_init(bus_lock_t *lock);
    void bus_lock_delete(bus_lock_t *lock);
    void bus_lock_release_from_isr(bus_lock_t *lock);

#endif


