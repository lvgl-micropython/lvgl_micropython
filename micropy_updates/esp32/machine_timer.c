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

#include <stdint.h>
#include <stdio.h>

#include "py/mphal.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "modmachine.h"
#include "py/gc.h"
#include "py/stackctrl.h"
#include "mphalport.h"

#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "esp_system.h"
#include "esp_cpu.h"

#include "machine_timer.h"


const mp_obj_type_t machine_timer_type;

static mp_obj_t machine_timer_init_helper(machine_timer_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);


void machine_timer_deinit_all(void)
{
    // Disable, deallocate and remove all timers from list
    machine_timer_obj_t **t = &MP_STATE_PORT(machine_timer_obj_head);
    while (*t != NULL) {
        machine_timer_disable(*t);
        machine_timer_obj_t *next = (*t)->next;
        m_del_obj(machine_timer_obj_t, *t);
        *t = next;
    }
}


static void machine_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    machine_timer_obj_t *self = self_in;
    qstr mode = self->repeat ? MP_QSTR_PERIODIC : MP_QSTR_ONE_SHOT;
    float period = (float)self->period / 1000.0f;
    mp_printf(print, "Timer(%u, mode=%q, period=%f)", self->id, mode, period);
}


machine_timer_obj_t *machine_timer_create(mp_uint_t timer)
{
    machine_timer_obj_t *self = NULL;

    // Check whether the timer is already initialized, if so use it
    for (machine_timer_obj_t *t = MP_STATE_PORT(machine_timer_obj_head); t; t = t->next) {
        if (t->id == (uint8_t)timer) {
            self = t;
            break;
        }
    }

    // The timer does not exist, create it.
    if (self == NULL) {
        self = mp_obj_malloc(machine_timer_obj_t, &machine_timer_type);
        self->handle = NULL;
        self->id = timer;

        // Add the timer to the linked-list of timers
        self->next = MP_STATE_PORT(machine_timer_obj_head);
        MP_STATE_PORT(machine_timer_obj_head) = self;
    }
    return self;
}


static mp_obj_t machine_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // Create the new timer.
    uint32_t timer_number = mp_obj_get_int(args[0]);
    machine_timer_obj_t *self = machine_timer_create(timer_number);

    if (n_args > 1 || n_kw > 0) {
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        machine_timer_init_helper(self, n_args - 1, args + 1, &kw_args);
    }

    return self;
}


void machine_timer_disable(machine_timer_obj_t *self)
{
    if (self->handle) {
        if (esp_timer_is_active(self->handle)) {
            esp_timer_stop(self->handle);
        }
    }
}


static void machine_timer_isr(void *self_in)
{
    machine_timer_obj_t *self = self_in;

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
        mp_obj_t args[1];
        args[0] = MP_OBJ_FROM_PTR(self);

        mp_call_function_n_kw(self->callback, 1, 0, args);
        nlr_pop();
    } else {
        ets_printf("Uncaught exception in IRQ callback handler!\n");
        mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
    }

    if (self->repeat) {
        self->period_time = esp_timer_get_time();
    }

    gc_unlock();
    mp_sched_unlock();

    mp_thread_set_state(old_state);
    mp_hal_wake_main_task_from_isr();
}


void machine_timer_enable(machine_timer_obj_t *self, void (*timer_isr))
{
    esp_err_t err;

    if (self->handle == NULL) {
        esp_timer_create_args_t timer_args = {
            .callback=timer_isr,
            .arg=self,
            .dispatch_method=ESP_TIMER_ISR
        };

        err = esp_timer_create(&timer_args, &self->handle);
        if (err != ESP_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Unable to create timer (%d)"), err);
        }

        if (self->repeat) err = esp_timer_start_periodic(self->handle, self->period);
        else err = esp_timer_start_once(self->handle, self->period);

        if (err != ESP_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Unable to start timer (%d)"), err);
        } else if (self->repeat) {
            self->period_time = esp_timer_get_time();
        }
    } else if (self->change_type) {
        self->change_type = 0;
        if (self->repeat) err = esp_timer_start_periodic(self->handle, self->period);
        else err = esp_timer_start_once(self->handle, self->period);

        if (err != ESP_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Unable to start timer (%d)"), err);
        }
    } else {
        err = esp_timer_restart(self->handle, self->period);
        if (err != ESP_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Unable to restart timer (%d)"), err);
        }
    }
}


static mp_obj_t machine_timer_init_helper(machine_timer_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, ARG_callback, ARG_period };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,         MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_callback,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none } },
        { MP_QSTR_period,       MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none } },
    };

    machine_timer_disable(self);

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int mode = (int)args[ARG_mode].u_int;

    if (mode == -1) {
        self->change_type = 0;
    } else if ((uint8_t)mode != self->repeat) {
        self->repeat = (uint8_t)mode;
        self->change_type = 1;
    }

    if (args[ARG_period].u_obj != mp_const_none) {
        if (mp_obj_is_float(args[ARG_period].u_obj)) {
            float f_period = (float)mp_obj_get_float(args[ARG_period].u_obj);
            self->period = (uint64_t)(f_period * 1000.0f);
        } else {
            self->period = (uint64_t)mp_obj_get_int_truncated(args[ARG_period].u_obj);
        }
    }
    if (args[ARG_callback].u_obj != mp_const_none) {
        self->callback = args[ARG_callback].u_obj;
    }

    machine_timer_enable(self, machine_timer_isr);

    return mp_const_none;
}


static mp_obj_t machine_timer__del__(mp_obj_t self_in)
{
    machine_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_timer_disable(self);

    if (self->handle != NULL) esp_timer_delete(self->handle);

    self->handle = NULL;
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(machine_timer__del__obj, machine_timer__del__);


static mp_obj_t machine_timer_deinit(mp_obj_t self_in)
{
    machine_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);

    machine_timer_disable(self);
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_deinit_obj, machine_timer_deinit);


static mp_obj_t machine_timer_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    return machine_timer_init_helper(args[0], n_args - 1, args + 1, kw_args);
}

static MP_DEFINE_CONST_FUN_OBJ_KW(machine_timer_init_obj, 1, machine_timer_init);


static mp_obj_t machine_timer_value(mp_obj_t self_in)
{
    machine_timer_obj_t *self = MP_OBJ_TO_PTR(self_in);
    float res;

    if (esp_timer_is_active(self->handle)) {
        uint64_t curr_time = esp_timer_get_time();
        if (self->repeat) {
            uint64_t diff = curr_time - self->period_time;
            res = (float)(self->period - diff) / 1000.0f;
        } else {
            uint64_t expire_time;
            esp_timer_get_expiry_time(self->handle, &expire_time);
            res = (float)(expire_time - curr_time) / 1000.0f;
        }
    } else {
        res = 0.0f;
    }

    if (res < 0.0f) res = 0.0f;

    return mp_obj_new_float(res);
}

static MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_value_obj, machine_timer_value);



static const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_timer__del__obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_timer_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_timer_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_ONE_SHOT), MP_ROM_INT(false) },
    { MP_ROM_QSTR(MP_QSTR_PERIODIC), MP_ROM_INT(true) },
};
static MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_timer_type,
    MP_QSTR_Timer,
    MP_TYPE_FLAG_NONE,
    make_new, machine_timer_make_new,
    print, machine_timer_print,
    locals_dict, &machine_timer_locals_dict
);

MP_REGISTER_ROOT_POINTER(struct _machine_timer_obj_t *machine_timer_obj_head);
