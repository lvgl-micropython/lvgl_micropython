#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/idf_additions.h"


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


bool mp_lcd_lock_acquire(mp_lcd_lock_t *lock)
{
    return pdTRUE == xSemaphoreTake(lock->handle, portMAX_DELAY);
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


void mp_lcd_sw_rotate_task(void *self_in)
{
    LCD_DEBUG_PRINT("mp_lcd_sw_rotate_task - STARTED\n")

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)self_in;

    if (!self->sw_rot->init.cb(self_in)) {
        mp_lcd_lock_release(&self->sw_rot->handles.init_lock);
        return;
    }

    uint8_t *idle_fb;
    uint8_t last_update;
    uint8_t count = 0;
    mp_lcd_sw_rotate_tx_param_t param;

    uint8_t bytes_per_pixel = self->sw_rot->data.bytes_per_pixel;
    mp_lcd_lock_acquire(&self->sw_rot->handles.copy_lock);

    self->sw_rot->init.err = LCD_OK;
    mp_lcd_lock_release(&self->sw_rot->handles.init_lock);

    bool exit = mp_lcd_event_isset(&self->sw_rot->handles.copy_task_exit);
    while (!exit) {
        mp_lcd_lock_acquire(&self->sw_rot->handles.copy_lock);

        if (self->sw_rot->tx_params.tx_param_cb != NULL) {
            mp_lcd_lock_acquire(&self->sw_rot->tx_params.lock)
            count = 0;
            while (self->sw_rot->tx_params.len) {
                param = self->sw_rot->tx_params.params[count];
                self->sw_rot->tx_params.tx_param_cb(self_in, param.cmd, param.params, param.params_len);
                self->sw_rot->tx_params.len--;
                count++;
                if (param.flush_next) break;
            }

            for (uint8_t i=0;i<count;i++) free(self->sw_rot->tx_params.params[i - count].params);

            if (self->sw_rot->tx_params.len == 0){
                free(self->sw_rot->tx_params.params);
                self->sw_rot->tx_params.params = NULL;
            } else {
                memmove(self->sw_rot->tx_params.params, self->sw_rot->tx_params.params + count,
                        self->sw_rot->tx_params.len * sizeof(mp_lcd_sw_rotate_tx_param_t));

                self->sw_rot->tx_params.params = (mp_lcd_sw_rotate_tx_param_t *)realloc(
                    self->sw_rot->tx_params.params,
                    self->sw_rot->tx_params.len * sizeof(mp_lcd_sw_rotate_tx_param_t));
            }
            mp_lcd_lock_release(&self->sw_rot->tx_params.lock);
        }
        if (self->sw_rot->buffers.partial != NULL) {
            if (self->sw_rotate || self->sw_rot->data->rgb565_dither) {
                last_update = self->sw_rot->data.last_update;

                idle_fb = self->sw_rot->buffers.idle;

                mp_lcd_sw_rotate((void *)idle_fb, (void *)self->sw_rot->buffers.partial, &self->sw_rot->data);
                self->trans_done = 1;

                mp_lcd_lock_release(&self->sw_rot->handles.tx_color_lock);
                if (self->callback != mp_const_none) flush_ready_cb(self->callback);

                self->sw_rot->flush_cb(self_in, last_update, idle_fb);
            } else {
                self->trans_done = 0;
                mp_lcd_lock_release(&self->sw_rot->handles.tx_color_lock);
                self->sw_rot->flush_cb(self_in, last_update, self->sw_rot->buffers.partial);
            }
        }

        exit = mp_lcd_event_isset(&self->sw_rot->handles.copy_task_exit);
    }

    LCD_DEBUG_PRINT(&mp_plat_print, "mp_lcd_sw_rotate_task - STOPPED\n")
}
