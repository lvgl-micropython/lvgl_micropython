#include "common/rgb565_dither.h"
#include "common/lcd_common_types.h"
#include "common/sw_rotate_task_common.h"
#include "sw_rotate_task.h"
#include "common/sw_rotate.h"
#include "common/lcd_bus_utils.h"

#include "lcd_types.h"


void mp_lcd_sw_rotate_task(void *self_in)
{
    LCD_DEBUG_PRINT("mp_lcd_sw_rotate_task - STARTED\n")

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)self_in;

    mp_lcd_sw_rotation_t *sw_rot = &self->sw_rot;
    mp_lcd_sw_rotation_buffers_t *buffers = &sw_rot->buffers;
    mp_lcd_sw_rotation_data_t *data = &sw_rot->data;
    mp_lcd_sw_rotation_handles_t *handles = &sw_rot->handles;
    mp_lcd_sw_rotate_tx_params_t *tx_params = &sw_rot->tx_params;
    mp_lcd_sw_rotation_init_t *init = &sw_rot->init;
    LCD_DEBUG_PRINT("mp_lcd_sw_rotate_task - starting bus init\n")


    if (!init->cb(self_in)) {
        mp_lcd_lock_release(&handles->init_lock);
        return;
    }
    LCD_DEBUG_PRINT("mp_lcd_sw_rotate_task - finished bus init\n")

    uint8_t *idle_fb;
    uint8_t last_update;
    uint8_t count = 0;
    mp_lcd_sw_rotate_tx_param_t param;
    uint8_t dither = data->rgb565_dither;
    int cmd;

    mp_lcd_lock_acquire(&handles->copy_lock);

    init->err = LCD_OK;
    mp_lcd_lock_release(&handles->init_lock);

    bool exit = mp_lcd_event_isset(&handles->copy_task_exit);
    while (!exit) {
        mp_lcd_lock_acquire(&handles->copy_lock);

        if (tx_params->cb != NULL) {
            mp_lcd_lock_acquire(&tx_params->lock);
            count = 0;
            while (tx_params->len) {
                param = tx_params->params[count];
                tx_params->cb(self_in, param.cmd, param.params, param.params_len);
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
                    if (!self->sw_rotate && !data->rgb565_swap && buffers->active == NULL) {
                        mp_lcd_allocate_rotation_buffers(self);
                    }
                } else {
                    rgb565_dither_free();
                    if (!self->sw_rotate && !data->rgb565_swap && buffers->active != NULL) {
                        mp_lcd_free_rotation_buffers(self);
                    }
                }
            }

            cmd = data->cmd;
            last_update = data->last_update;

            if (self->sw_rotate || dither || data->rgb565_swap) {
                idle_fb = buffers->idle;

                mp_lcd_sw_rotate((void *)idle_fb, (void *)buffers->partial, data);
                self->trans_done = 1;

                mp_lcd_lock_release(&handles->tx_color_lock);
                if (self->callback != mp_const_none) mp_lcd_flush_ready_cb(self->callback, false);
            } else {
                idle_fb = buffers->partial;
                self->trans_done = 0;
                mp_lcd_lock_release(&handles->tx_color_lock);
            }

            sw_rot->flush_cb(self_in, cmd, last_update, idle_fb);
        }

        exit = mp_lcd_event_isset(&handles->copy_task_exit);
    }
    mp_lcd_lock_release(&handles->copy_lock);

    LCD_DEBUG_PRINT(&mp_plat_print, "mp_lcd_sw_rotate_task - STOPPED\n")
}
