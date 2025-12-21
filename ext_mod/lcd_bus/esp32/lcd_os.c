// Copyright (c) 2024 - 2025 Kevin G. Schlosser

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/stackctrl.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "freertos/idf_additions.h"
#include "rom/ets_sys.h"
#include "esp_system.h"
#include "esp_cpu.h"

#include "esp_lcd_panel_ops.h"


#include "lcd_os.h"

#include <string.h>


#define _EVENT_BIT_0 (1 << 0)


struct lcd_lock_t {
    SemaphoreHandle_t handle;
    StaticSemaphore_t buffer;
};

struct lcd_event_t {
    EventGroupHandle_t handle;
    StaticEventGroup_t buffer;
};


void lcd_event_init(lcd_event_t *event)
{
    event->handle = xEventGroupCreateStatic(&event->buffer);
}


void lcd_event_delete(lcd_event_t *event)
{
    xEventGroupSetBits(event->handle, _EVENT_BIT_0);
    vEventGroupDelete(event->handle);

}


void lcd_event_wait(lcd_event_t *event)
{
    xEventGroupWaitBits(event->handle, _EVENT_BIT_0, pdFALSE, pdTRUE, portMAX_DELAY);
}


bool lcd_event_isset(lcd_event_t *event)
{
    return (bool)(xEventGroupGetBits(event->handle) & _EVENT_BIT_0);
}


bool lcd_event_isset_from_isr(lcd_event_t *event)
{
    return (bool)(xEventGroupGetBitsFromISR(event->handle) & _EVENT_BIT_0);
}


void lcd_event_set(lcd_event_t *event)
{
    xEventGroupSetBits(event->handle, _EVENT_BIT_0);
}


void lcd_event_clear(lcd_event_t *event)
{
    xEventGroupClearBits(event->handle, _EVENT_BIT_0);
}


void lcd_event_clear_from_isr(lcd_event_t *event)
{
    xEventGroupClearBitsFromISR(event->handle, _EVENT_BIT_0);
}


void lcd_event_set_from_isr(lcd_event_t *event)
{
    xEventGroupSetBitsFromISR(event->handle, _EVENT_BIT_0, pdFALSE);
}


void lcd_lock_acquire(lcd_lock_t *lock)
{
    xSemaphoreTake(lock->handle, portMAX_DELAY);
}


void lcd_lock_release(lcd_lock_t *lock)
{
    xSemaphoreGive(lock->handle);
}


void lcd_lock_release_from_isr(lcd_lock_t *lock)
{
    xSemaphoreGiveFromISR(lock->handle, pdFALSE);
}


void lcd_lock_init(lcd_lock_t *lock)
{
    lock->handle = xSemaphoreCreateBinaryStatic(&lock->buffer);
    xSemaphoreGive(lock->handle);
}


void lcd_lock_delete(lcd_lock_t *lock)
{
    vSemaphoreDelete(lock->handle);
}


bool rgb_bus_init(mp_lcd_bus_obj_t *self_in)
{
    mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)self_in;
    esp_lcd_rgb_panel_event_callbacks_t callbacks = { .on_vsync = lcd_trans_done_cb };

    self->task_init->init_err = esp_lcd_new_rgb_panel(&self->panel_io_config, &self->panel_handle);
    if (self->task_init->init_err != 0) {
        self->task_init->init_err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_rgb_panel)");
        return false;
    }

    self->task_init->init_err = esp_lcd_rgb_panel_register_event_callbacks(
                                         self->panel_handle, &callbacks, self);

    if (self->task_init->init_err != 0) {
        self->task_init->init_err_msg = MP_ERROR_TEXT(
                          "%d(esp_lcd_rgb_panel_register_event_callbacks)");

        return false;
    }

    self->task_init->init_err = esp_lcd_panel_reset(self->panel_handle);
    if (self->task_init->init_err != 0) {
        self->task_init->init_err_msg = MP_ERROR_TEXT("%d(esp_lcd_panel_reset)");
        return false;
    }

    self->task_init->init_err = esp_lcd_panel_init(self->panel_handle);
    if (self->task_init->init_err != 0) {
        self->task_init->init_err_msg = MP_ERROR_TEXT("%d(esp_lcd_panel_init)");
        return false;
    }

    rgb_panel_t *rgb_panel = __containerof((esp_lcd_panel_t *)self->panel_handle,
                                           rgb_panel_t, base);

    self->buffers.active_fb = rgb_panel->fbs[0];
    self->buffers.idle_fb = rgb_panel->fbs[1];
    return true;


}


static void make_callback(mp_obj_t callback)
{
    volatile uint32_t sp = (uint32_t)esp_cpu_get_sp();

    void *old_state = mp_thread_get_state();

    mp_state_thread_t ts;
    mp_thread_set_state(&ts);
    mp_stack_set_top((void*)sp);
    mp_stack_set_limit(CONFIG_FREERTOS_IDLE_TASK_STACKSIZE - 1024);
    mp_locals_set(mp_state_ctx.thread.dict_locals);
    mp_globals_set(mp_state_ctx.thread.dict_globals);

    mp_sched_lock();
    gc_lock();

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_call_function_n_kw(callback, 0, 0, NULL);
        nlr_pop();
    } else {
        ets_printf("Uncaught exception in IRQ callback handler!\n");
        mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
    }

    gc_unlock();
    mp_sched_unlock();

    mp_thread_set_state(old_state);
}


static void rgb_write_cmd(mp_lcd_bus_obj_t *self_in, int cmd, void *param, size_t param_size)
{
    LCD_UNUSED(cmd);
    LCD_UNUSED(param_size);
}


static void rgb_write_display(mp_lcd_bus_obj_t *self_in, int cmd, int x_start,
                                      int y_start, int x_end, int y_end, uint8_t *idle_buf,
                                      uint8_t *active_buf, size_t buf_size)
{
    LCD_UNUSED(cmd);
    LCD_UNUSED(buf_size);
    mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)self_in;

    mp_lcd_err_t err = esp_lcd_panel_draw_bitmap(self->panel_handle, 0, 0,
                                     self->rot_data.dst_width - 1, self->rot_data.dst_height - 1,
                                     buf);

    if (err != LCD_OK) {
        mp_printf(&mp_plat_print, "esp_lcd_panel_draw_bitmap error (%d)\n", err);
    } else {
        lcd_event_clear(&self->buffers.swap_bufs);
        lcd_event_wait(&self->buffers.swap_bufs);
        memcpy(idle_buf, active_buf, self->rot_data.dst_width * self->rot_data.dst_height * self->rot_data.bytes_per_pixel);
    }
}



void lcd_rotate_task(void * self_in)
{
    LCD_DEBUG_PRINT("lcd_copy_task - STARTED\n")
    
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)self_in;
    if (!self->task_init->init(self)) {
        lcd_lock_release(&self->task_init->init_func);
        for (;;) {}
    }

    uint8_t *active_fb = self->buffers.active_fb;
    uint8_t *idle_fb = self->buffers.idle_fb;
    uint8_t *partial_buf;
    
    int x_start;
    int y_start;
    int x_end;
    int y_end;

    uint16_t dst_width = self->rot_data.dst_width;
    uint16_t dst_height = self->rot_data.dst_height;

    int write_cmd;
    size_t write_buf_size;
    uint8_t rotation;
    uint8_t bytes_per_pixel;
    uint8_t last_update;
    uint8_t dither;
    uint8_t sw_rotate;
    uint8_t byteswap = self->rot_data.byteswap;
    uint8_t bytes_per_pixel = self->rot_data.bytes_per_pixel;
    bool flush_is_next = false;

    lcd_lock_acquire(&self->buffers.copy_lock);

    self->task_init->init_err = LCD_OK;
    lcd_lock_release(&self->task_init->init_lock);

    bool exit = lcd_event_isset(&self->task.copy_task_exit);
    
    while (!exit) {
        lcd_lock_acquire(&self->buffers.copy_lock);

        partial_buf = self->buffers.partial_buf;
        last_update = self->rot_data.last_update;

        idle_fb = self->buffers.idle_fb;
        active_fb = self->buffers.active_fb;

        x_start = self->rot_data.x_start;
        y_start = self->rot_data.y_start;
        x_end = self->rot_data.x_end;
        y_end = self->rot_data.y_end;
        rotation = self->rot_data.rotation;
        dither = self->rot_data.dither;
        sw_rotate = self->rot_data.dither;
        write_cmd = self->rot_data.write_cmd;
        write_buf_size = self->rot_data.write_buf_size;

        lcd_lock_release(&self->task.tx_color_lock);

        if (partial_buf == NULL || !flush_is_next) {
            lcd_lock_acquire(&self->tx_cmds.lock);
            lcd_tx_cmd_t *tx_cmd;
            uint16_t i = 0;

            for (;i<self->tx_cmds.num_cmds;i++) {
                tx_cmd = self->tx_cmds.cmds[i];
                flush_is_next = tx_cmd->flush_is_next;
                self->tx_cmds.write_cmd(self, tx_cmd->cmd, tx_cmd->params, tx_cmd->param_size);

                if (tx_cmd->params != NULL) free(tx_cmd->params);
                free(tx_cmd);
                self->tx_cmds.cmds[i] = NULL;

                if (flush_is_next) break;
            }

            i++;
            self->tx_cmds.num_cmds -= i;
            if (self->tx_cmds.num_cmds != 0) {
                memcpy(self->tx_cmds.cmds, self->tx_cmds.cmds[i], sizeof(lcd_tx_cmd_t *) * (size_t)self->tx_cmds.num_cmds);
            }

            self->tx_cmds.cmds = (lcd_tx_cmd_t **)realloc(self->tx_cmds.cmds, sizeof(lcd_tx_cmd_t *) * (size_t)self->tx_cmds.num_cmds);
        }

        if (partial_buf != NULL) {
            flush_is_next = false;

            if (!sw_rotate) {
                rotation = LCD_ROTATION_0;
                last_update = 1;
            }

            if (sw_rotate || dither || byteswap) {
                copy_pixels(
                    (void *)idle_fb, (void *)partial_buf,
                    x_start, y_start, x_end, y_end,
                    dst_width, dst_height, bytes_per_pixel,
                    rotation, dither, byteswap);
            } else {
                idle_fb = partial_buf;
            }

            if (self->callback != mp_const_none) {
                make_callback(self->callback);
            }

            if (last_update) {
                self->task.write_display(self, write_cmd, x_start, y_start, x_end, y_end, idle_buf, active_buf, write_buf_size);
            }
        }

        exit = lcd_event_isset(&self->task.copy_task_exit);
    }

    LCD_DEBUG_PRINT("lcd_copy_task - STOPPED\n")
}
