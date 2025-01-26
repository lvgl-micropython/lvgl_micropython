#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/idf_additions.h"
#include "esp_task.h"

#include "common/lcd_common_types.h"
#include "common/sw_rotate_task_common.h"
#include "sw_rotate_task.h"
#include "common/sw_rotate.h"

#include "lcd_types.h"


#define LCD_DEFAULT_STACK_SIZE    (5 * 1024)
#define LCDBUS_BIT_0 (1 << 0)


void mp_lcd_event_init(mp_lcd_event_t *event)
{
    event->handle = xEventGroupCreateStatic(&event->buffer);
}


void mp_lcd_event_delete(mp_lcd_event_t *event)
{
    xEventGroupSetBits(event->handle, LCDBUS_BIT_0);
    vEventGroupDelete(event->handle);

}

void mp_lcd_event_wait(mp_lcd_event_t *event)
{
    xEventGroupWaitBits(event->handle, LCDBUS_BIT_0, pdFALSE, pdTRUE, portMAX_DELAY);
}


bool mp_lcd_event_isset(mp_lcd_event_t *event)
{
    return (bool)(xEventGroupGetBits(event->handle) & LCDBUS_BIT_0);
}


bool mp_lcd_event_isset_from_isr(mp_lcd_event_t *event)
{
    return (bool)(xEventGroupGetBitsFromISR(event->handle) & LCDBUS_BIT_0);
}


void mp_lcd_event_set(mp_lcd_event_t *event)
{
    xEventGroupSetBits(event->handle, LCDBUS_BIT_0);
}


void mp_lcd_event_clear(mp_lcd_event_t *event)
{
    xEventGroupClearBits(event->handle, LCDBUS_BIT_0);
}


void mp_lcd_event_clear_from_isr(mp_lcd_event_t *event)
{
    xEventGroupClearBitsFromISR(event->handle, LCDBUS_BIT_0);
}


void mp_lcd_event_set_from_isr(mp_lcd_event_t *event)
{
    xEventGroupSetBitsFromISR(event->handle, LCDBUS_BIT_0, pdFALSE);
}


void mp_lcd_lock_acquire(mp_lcd_lock_t *lock)
{
    xSemaphoreTake(lock->handle, portMAX_DELAY);
}


void mp_lcd_lock_release(mp_lcd_lock_t *lock)
{
    xSemaphoreGive(lock->handle);
}


void mp_lcd_lock_release_from_isr(mp_lcd_lock_t *lock)
{
    xSemaphoreGiveFromISR(lock->handle, pdFALSE);
}


void mp_lcd_lock_init(mp_lcd_lock_t *lock)
{
    lock->handle = xSemaphoreCreateBinaryStatic(&lock->buffer);
    xSemaphoreGive(lock->handle);
}


void mp_lcd_lock_delete(mp_lcd_lock_t *lock)
{
    vSemaphoreDelete(lock->handle);
}


bool mp_lcd_start_rotate_task(void *self_in)
{
    mp_lcd_obj_t *self = (mp_lcd_obj_t *)self_in;

    xTaskCreatePinnedToCore(
                mp_lcd_sw_rotate_task, "rotate_task", LCD_DEFAULT_STACK_SIZE / sizeof(StackType_t),
                self, ESP_TASK_PRIO_MAX - 1, &self->sw_rot.handles.task_handle, 0);

    mp_lcd_lock_acquire(&self->sw_rot.handles.init_lock);
    mp_lcd_lock_release(&self->sw_rot.handles.init_lock);
    mp_lcd_lock_delete(&self->sw_rot.handles.init_lock);

    if (self->sw_rot.init.err != LCD_OK) {
        mp_raise_msg_varg(&mp_type_OSError, self->sw_rot.init.err_msg, self->sw_rot.init.err);
        return false;
    } else {
        return true;
    }
}
