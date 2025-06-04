/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2015 Damien P. George
 * Copyright (c) 2016 Paul Sokolovsky
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef MICROPY_INCLUDED_ESP32_MACHINE_TIMER_H
#define MICROPY_INCLUDED_ESP32_MACHINE_TIMER_H

#include "esp_timer.h"

typedef struct _machine_timer_obj_t {
    mp_obj_base_t base;

    esp_timer_handle_t handle;
    uint8_t id;

    uint8_t change_type: 1;
    uint8_t repeat: 1;
    // ESP32 timers are 64-bit
    uint64_t period;
    uint64_t period_time;

    mp_obj_t callback;

    struct _machine_timer_obj_t *next;
} machine_timer_obj_t;

machine_timer_obj_t *machine_timer_create(mp_uint_t timer);
void machine_timer_enable(machine_timer_obj_t *self, void (*timer_isr));
void machine_timer_disable(machine_timer_obj_t *self);

#endif // MICROPY_INCLUDED_ESP32_MACHINE_TIMER_H
