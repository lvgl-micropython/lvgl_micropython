// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "lcd_bus_task.h"
#include "lcd_types.h"


void lcd_bus_event_init(lcd_bus_event_t *event) { }

void lcd_bus_event_delete(lcd_bus_event_t *event) { }


bool lcd_bus_event_isset(lcd_bus_event_t *event) { return false; }


bool lcd_bus_event_isset_from_isr(lcd_bus_event_t *event) { return false; }


void lcd_bus_lock_init(lcd_bus_lock_t *lock) { }


static void rgb565_byte_swap(void *buf, size_t buf_size_px)
{
    uint16_t *buf16 = (uint16_t *)buf;

    for (size_t i=0;i<buf_size_px; i++) {
        buf16[i] = (buf16[i] << 8) | (buf16[i] >> 8);
    }
}


void lcd_bus_task(void *self_in) { }

