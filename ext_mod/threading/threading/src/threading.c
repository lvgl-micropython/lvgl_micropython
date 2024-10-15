/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George on behalf of Pycom Ltd
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

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/mpthread.h"
#include "stdio.h"

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mpthread.h"
#include "py/mphal.h"
#include "mpthreadport.h"

static mp_obj_t threading_get_ident(void)
{
    return mp_obj_new_int_from_uint((mp_uint_t)xTaskGetCurrentTaskHandle());
}

static MP_DEFINE_CONST_FUN_OBJ_0(threading_get_ident_obj, threading_get_ident);


mp_obj_t threading_main_thread(void)
{
    return MP_OBJ_FROM_PTR(thread_entry0);
}

static MP_DEFINE_CONST_FUN_OBJ_0(threading_main_thread_obj, threading_main_thread);


mp_obj_t threading_enumerate(void)
{
    mp_obj_t list = mp_obj_new_list(0, NULL);

    mutex_lock(&thread_mutex, 1);

    for (mp_obj_thread_thread_t *th = thread; th != NULL; th = th->next) {
        if (!th->is_alive) {
            continue;
        }
        mp_obj_list_append(list, MP_OBJ_FROM_PTR(th));
    }

    mutex_unlock(&thread_mutex);
    return list;
}

static MP_DEFINE_CONST_FUN_OBJ_0(threading_enumerate_obj, threading_enumerate);


static mp_obj_t threading_stack_size(size_t n_args, const mp_obj_t *args) {
    mp_obj_t ret = mp_obj_new_int_from_uint(stack_size);
    if (n_args == 0) {
        stack_size = 0;
    } else {
        stack_size = mp_obj_get_int(args[0]);
    }
    return ret;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(threading_stack_size_obj, 0, 1, threading_stack_size);


static const mp_rom_map_elem_t mp_module_threading_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_threading) },
    { MP_ROM_QSTR(MP_QSTR_RLock), MP_ROM_PTR(&mp_type_threading_rlock_t) },
    { MP_ROM_QSTR(MP_QSTR_Lock), MP_ROM_PTR(&mp_type_threading_lock_t) },
    { MP_ROM_QSTR(MP_QSTR_Event), MP_ROM_PTR(&mp_type_threading_event_t) },
    { MP_ROM_QSTR(MP_QSTR_Thread), MP_ROM_PTR(&mp_type_threading_thread_t) },
    { MP_ROM_QSTR(MP_QSTR_Semaphore), MP_ROM_PTR(&mp_type_threading_semaphore_t) },


    { MP_ROM_QSTR(MP_QSTR_get_ident), MP_ROM_PTR(&threading_get_ident_obj) },
    { MP_ROM_QSTR(MP_QSTR_enumerate), MP_ROM_PTR(&threading_enumerate_obj) },
    { MP_ROM_QSTR(MP_QSTR_main_thread), MP_ROM_PTR(&threading_main_thread_obj) },
    { MP_ROM_QSTR(MP_QSTR_stack_size), MP_ROM_PTR(&threading_stack_size_obj) },

};

static MP_DEFINE_CONST_DICT(mp_module_threading_globals, mp_module_threading_globals_table);


const mp_obj_module_t mp_module_threading = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_threading_globals,
};

MP_REGISTER_MODULE(MP_QSTR_threading, mp_module_threading);
