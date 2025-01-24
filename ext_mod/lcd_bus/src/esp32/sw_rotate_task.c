#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/idf_additions.h"
#include "esp_task.h"

#include "common/rgb565_dither"
#include "common/lcd_common_types.h"
#include "common/sw_rotate.h"
#include "common/sw_rotate_task_common.h"

#include "lcd_types.h"
#include "sw_rotate_task.h"

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


static void sw_rotate_task(void *self_in)
{
    LCD_DEBUG_PRINT("mp_lcd_sw_rotate_task - STARTED\n")

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)self_in;

    mp_lcd_sw_rotation_t *sw_rot = &self->sw_rot;
    mp_lcd_sw_rotation_buffers_t *buffers = &sw_rot->buffers;
    mp_lcd_sw_rotation_data_t *data = &sw_rot->data;
    mp_lcd_sw_rotation_handles_t *handles = &sw_rot->handles;
    mp_lcd_sw_rotate_tx_params_t *tx_params = &sw_rot->tx_params;
    mp_lcd_sw_rotation_init_t *init = &sw_rot->init;

    if (!init->cb(self_in)) {
        mp_lcd_lock_release(&handles->init_lock);
        return;
    }

    uint8_t *idle_fb;
    uint8_t last_update;
    uint8_t count = 0;
    mp_lcd_sw_rotate_tx_param_t param;
    uint8_t dither = data->rgb565_dither;

    mp_lcd_lock_acquire(&handles->copy_lock);

    init->err = LCD_OK;
    mp_lcd_lock_release(&handles->init_lock);

    bool exit = mp_lcd_event_isset(&handles->copy_task_exit);
    while (!exit) {
        mp_lcd_lock_acquire(&handles->copy_lock);

        if (tx_params->tx_param_cb != NULL) {
            mp_lcd_lock_acquire(&tx_params->lock)
            count = 0;
            while (tx_params->len) {
                param = tx_params->params[count];
                tx_params->tx_param_cb(self_in, param.cmd, param.params, param.params_len);
                tx_params->len--;
                count++;
                if (param.flush_next) break;
            }

            for (uint8_t i=0;i<count;i++) free(tx_params->params[i - count].params);

            if (tx_params->len == 0){
                free(tx_params->params);
                tx_params->params = NULL;
            } else {
                memmove(tx_params->params, tx_params->params + count,
                        tx_params->len * sizeof(mp_lcd_sw_rotate_tx_param_t));

                tx_params->params = (mp_lcd_sw_rotate_tx_param_t *)realloc(
                    tx_params->params,
                    tx_params->len * sizeof(mp_lcd_sw_rotate_tx_param_t));
            }
            mp_lcd_lock_release(&tx_params->lock);
        }

        if (buffers->partial != NULL) {
            if (dither != data->rgb565_dither) {
                dither = data->rgb565_dither;
                if (dither) {
                    rgb565_dither_init();
                    if (!self->sw_rotate && !self->rgb565_byte_swap && buffers->active == NULL) {
                        mp_lcd_allocate_rotation_buffers(self);
                    }
                } else {
                    rgb565_dither_free();
                    if (!self->sw_rotate && !self->rgb565_byte_swap && buffers->active != NULL) {
                        mp_lcd_free_rotation_buffers(self);
                    }
                }
            }

            if (self->sw_rotate || dither || self->rgb565_byte_swap) {
                last_update = data->last_update;

                idle_fb = buffers->idle;

                mp_lcd_sw_rotate((void *)idle_fb, (void *)buffers->partial, data);
                self->trans_done = 1;

                mp_lcd_lock_release(&handles->tx_color_lock);
                if (self->callback != mp_const_none) flush_ready_cb(self->callback);

                sw_rot->flush_cb(self_in, last_update, idle_fb);
            } else {
                self->trans_done = 0;
                mp_lcd_lock_release(&handles->tx_color_lock);
                sw_rot->flush_cb(self_in, last_update, buffers->partial);
            }
        }

        exit = mp_lcd_event_isset(&handles->copy_task_exit);
    }

    LCD_DEBUG_PRINT(&mp_plat_print, "mp_lcd_sw_rotate_task - STOPPED\n")
}


bool mp_lcd_start_rotate_task(void *self_in)
{
    mp_lcd_obj_t *self = (mp_lcd_obj_t *)self_in;

    xTaskCreatePinnedToCore(
                sw_rotate_task, "rotate_task", LCD_DEFAULT_STACK_SIZE / sizeof(StackType_t),
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
