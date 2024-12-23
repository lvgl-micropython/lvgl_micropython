#include "bus_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/idf_additions.h"


#define RGB_BIT_0 (1 << 0)


void bus_event_init(bus_event_t *event)
{
    event->handle = xEventGroupCreateStatic(&event->buffer);
}


void bus_event_delete(bus_event_t *event)
{
    xEventGroupSetBits(event->handle, RGB_BIT_0);
    vEventGroupDelete(event->handle);

}

void bus_event_wait(bus_event_t *event)
{
    xEventGroupWaitBits(event->handle, RGB_BIT_0, pdFALSE, pdTRUE, portMAX_DELAY);
}


bool bus_event_isset(bus_event_t *event)
{
    return (bool)(xEventGroupGetBits(event->handle) & RGB_BIT_0);
}


bool bus_event_isset_from_isr(bus_event_t *event)
{
    return (bool)(xEventGroupGetBitsFromISR(event->handle) & RGB_BIT_0);
}


void bus_event_set(bus_event_t *event)
{
    xEventGroupSetBits(event->handle, RGB_BIT_0);
}


void bus_event_clear(bus_event_t *event)
{
    xEventGroupClearBits(event->handle, RGB_BIT_0);
}


void bus_event_clear_from_isr(bus_event_t *event)
{
    xEventGroupClearBitsFromISR(event->handle, RGB_BIT_0);
}


void bus_event_set_from_isr(bus_event_t *event)
{
    xEventGroupSetBitsFromISR(event->handle, RGB_BIT_0, pdFALSE);
}


int bus_lock_acquire(bus_lock_t *lock, int32_t wait_ms)
{
    return pdTRUE == xSemaphoreTake(lock->handle, wait_ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS((uint16_t)wait_ms));
}


void bus_lock_release(bus_lock_t *lock)
{
    xSemaphoreGive(lock->handle);
}


void bus_lock_release_from_isr(bus_lock_t *lock)
{
    xSemaphoreGiveFromISR(lock->handle, pdFALSE);
}


void bus_lock_init(bus_lock_t *lock)
{
    lock->handle = xSemaphoreCreateBinaryStatic(&lock->buffer);
    xSemaphoreGive(lock->handle);
}


void bus_lock_delete(bus_lock_t *lock)
{
    vSemaphoreDelete(lock->handle);
}
