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

#include "lcd_bus_task.h"
#include "rgb565_dither.h"

#include <string.h>


void lcd_bus_event_delete(lcd_bus_event_t *event)
{
    xEventGroupSetBits(event->handle, (1 << 0));
    vEventGroupDelete(event->handle);

}


bool lcd_bus_event_isset(lcd_bus_event_t *event)
{
    return (bool)(xEventGroupGetBits(event->handle) & (1 << 0));
}


bool lcd_bus_event_isset_from_isr(lcd_bus_event_t *event)
{
    return (bool)(xEventGroupGetBitsFromISR(event->handle) & (1 << 0));
}


void lcd_bus_lock_init(lcd_bus_lock_t *lock)
{
    lock->handle = xSemaphoreCreateBinaryStatic(&lock->buffer);
    xSemaphoreGive(lock->handle);
}


static void rgb565_byte_swap(void *buf, size_t buf_size_px)
{
    uint16_t *buf16 = (uint16_t *)buf;

    for (size_t i=0;i<buf_size_px; buf16++) {
        *buf16 =  (*buf16 << 8) | (*buf16 >> 8);
    }
}


void lcd_bus_task(void *self_in) {
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)self_in;

    if (!self->init_func(self)) {
        lcd_bus_lock_release(self->init_lock);
        return;
    }

    uint8_t *idle_fb;
    bool last_update;

    uint8_t bytes_per_pixel = self->bytes_per_pixel;
    lcd_bus_lock_acquire(self->copy_lock);

    self->init_err = LCD_OK;
    lcd_bus_lock_release(self->init_lock);

    rotation_data_t *r_data = malloc(sizeof(rotation_data_t));
    rotation_data_t *original_r_data = malloc(sizeof(rotation_data_t));

    bool exit = lcd_bus_event_isset(&self->copy_task_exit);
    while (!exit) {
        lcd_bus_lock_acquire(self->copy_lock);

        idle_fb = self->idle_fb;

        memcpy(r_data, &self->r_data, sizeof(rotation_data_t))
        memcpy(original_r_data, &self->r_data, sizeof(rotation_data_t))

        if (self->partial_buf != NULL) {

            copy_pixels((void *)idle_fb, (void *)self->partial_buf, r_data);

            lcd_bus_lock_release(self->tx_color_lock);

            self->flush_func(self, r_data, original_r_data, idle_fb, last_update);
        } else if (r_data->sw_rotation != 1) {
            if (r_data->swap) {
                rgb565_byte_swap((uint16_t *)idle_fb, r_data->color_size / 2);
            }
            self->flush_cb(self, r_data, original_r_data, idle_fb, last_update);
        }

        exit = rgb_bus_event_isset(&self->copy_task_exit);
    }
}

