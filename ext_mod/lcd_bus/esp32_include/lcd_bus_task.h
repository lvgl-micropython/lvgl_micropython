#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/idf_additions.h"


#ifndef __LCD_BUS_TASK_H__
    #define __LCD_BUS_TASK_H__

    typedef struct _lcd_bus_lock_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    } lcd_bus_lock_t;

    typedef struct _lcd_bus_event_t {
        EventGroupHandle_t handle;
        StaticEventGroup_t buffer;
    } lcd_bus_event_t;

    void lcd_bus_event_init(lcd_bus_event_t *event);


    #define lcd_bus_event_wait(event) xEventGroupWaitBits(event.handle, (1 << 0), pdFALSE, pdTRUE, portMAX_DELAY)
    #define lcd_bus_event_set(event) xEventGroupSetBits(event.handle, (1 << 0))
    #define lcd_bus_event_clear(event) xEventGroupClearBits(event.handle, (1 << 0))
    #define lcd_bus_event_clear_from_isr(event) xEventGroupClearBitsFromISR(event.handle, (1 << 0))
    #define lcd_bus_event_set_from_isr(event) xEventGroupSetBitsFromISR(event.handle, (1 << 0), pdFALSE)

    void lcd_bus_event_delete(lcd_bus_event_t *event);
    bool lcd_bus_event_isset(lcd_bus_event_t *event);
    bool lcd_bus_event_isset_from_isr(lcd_bus_event_t *event);


    #define lcd_bus_lock_acquire(lock) xSemaphoreTake(lock.handle, portMAX_DELAY)
    #define lcd_bus_lock_release(lock) xSemaphoreGive(lock.handle)
    #define lcd_bus_lock_release_from_isr(lock) xSemaphoreGiveFromISR(lock.handle, pdFALSE)
    #define lcd_bus_lock_delete(lock) vSemaphoreDelete(lock.handle)

    void lcd_bus_lock_init(lcd_bus_lock_t *lock);

    void lcd_bus_task(void *self_in);

#endif