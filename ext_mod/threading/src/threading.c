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

#if MICROPY_PY_THREAD

#include "py/mpthread.h"



#include "stdio.h"

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mpthread.h"
#include "py/mphal.h"
#include "mpthreadport.h"

#include "esp_task.h"



// the mutex controls access to the linked list
static threading_thread_entry_cb_t ext_threading_thread_entry = NULL;
static threading_mutex_t thread_mutex;
static mp_obj_threading_thread_t thread_entry0;
static mp_obj_threading_thread_t *thread = NULL; // root pointer, handled by threading_gc_others
size_t thread_stack_size = 0;


void threading_init(void *stack, uint32_t stack_len) {
    mp_thread_set_state(&mp_state_ctx.thread);
    // create the first entry in the linked list of all threads
    thread_entry0.id = xTaskGetCurrentTaskHandle();

    thread_entry0.ident = mp_obj_new_int_from_uint((mp_uint_t)thread_entry0.id);
    thread_entry0.ready = 1;
    thread_entry0.running = true;
    thread_entry0.arg = NULL;
    thread_entry0.stack = stack;
    thread_entry0.stack_len = stack_len;
    thread_entry0.next = NULL;
    mutex_init(&thread_mutex);

    // memory barrier to ensure above data is committed
    __sync_synchronize();

    // FREERTOS_TASK_DELETE_HOOK needs the thread ready after thread_mutex is ready
    thread = &thread_entry0;
}


void threading_gc_others(void) {
    mutex_lock(&thread_mutex, 1);
    for (mp_obj_threading_thread_t *th = thread; th != NULL; th = th->next) {
        gc_collect_root((void **)&th, 1);
        gc_collect_root(&th->arg, 1); // probably not needed
        if (th->id == xTaskGetCurrentTaskHandle()) {
            continue;
        }
        if (!th->ready) {
            continue;
        }
        gc_collect_root(th->stack, th->stack_len);
    }
    mutex_unlock(&thread_mutex);
}


mp_state_thread_t *mp_threading_get_state(void) {
    return pvTaskGetThreadLocalStoragePointer(NULL, 1);
}

void mp_threading_set_state(mp_state_thread_t *state) {
    vTaskSetThreadLocalStoragePointer(NULL, 1, state);
}




static void threading_freertos_entry(void *arg) {
    if (ext_threading_thread_entry) {
        mp_obj_threading_thread_t * th = (mp_obj_threading_thread_t *)arg;
        ext_threading_thread_entry(th);
    }
    vTaskDelete(NULL);
    for (;;) {;
    }
}


mp_uint_t threading_thread_create_ex(threading_thread_entry_cb_t entry, mp_obj_threading_thread_t *th, int priority, char *name) {
    // store thread entry function into a global variable so we can access it
    ext_threading_thread_entry = entry;

    if (th->call_args->stack_size == 0) {
        th->call_args->stack_size = THREADING_DEFAULT_STACK_SIZE; // default stack size
    } else if (th->call_args->stack_size < THREADING_MIN_STACK_SIZE) {
        th->call_args->stack_size = THREADING_MIN_STACK_SIZE; // minimum stack size
    }

    // Allocate linked-list node (must be outside thread_mutex lock)

    mutex_lock(&thread_mutex, 1);

    BaseType_t core_id = xPortGetCoreID();
    // create thread
    BaseType_t result = xTaskCreatePinnedToCore(threading_freertos_entry, name, *stack_size / sizeof(StackType_t), th, priority, &th->id, core_id);

    if (result != pdPASS) {
        mutex_unlock(&thread_mutex);
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("can't create thread"));
    }

    // add thread to linked list of all threads
    th->ready = 0;
    th->arg = th->call_args;
    th->stack = pxTaskGetStackStart(th->id);
    th->stack_len = th->call_args->stack_size / sizeof(uintptr_t);
    th->next = thread;
    thread = th;

    // adjust the stack_size to provide room to recover from hitting the limit
    th->call_args->stack_size -= 1024;

    mutex_unlock(&thread_mutex);

    return (mp_uint_t)th->id;
}


mp_uint_t threading_thread_create(void *(*entry)(void *), mp_obj_threading_thread_t *th) {
    return threading_thread_create_ex(entry, th, MP_THREAD_PRIORITY, "mp_thread");
}



void FREERTOS_TASK_DELETE_HOOK(void *tcb) {
    if (thread == NULL) {
        // threading not yet initialised
        return;
    }
    mp_obj_threading_thread_t *prev = NULL;
    mutex_lock(&thread_mutex, 1);
    for (mp_obj_threading_thread_t *th = thread; th != NULL; prev = th, th = th->next) {
        if ((void *)th->id == tcb) {
            if (prev != NULL) {
                prev->next = th->next;
            } else {
                thread = th->next;
            }
            break;
        }
    }

    mutex_unlock(&thread_mutex);
}


void threading_deinit(void) {
    for (;;) {
        // Find a task to delete
        TaskHandle_t id = NULL;
        mutex_lock(&thread_mutex, 1);
        for (mp_obj_threading_thread_t *th = thread; th != NULL; th = th->next) {
            // Don't delete the current task
            if (th->id != xTaskGetCurrentTaskHandle()) {
                id = th->id;
                break;
            }
        }
        mutex_unlock(&thread_mutex);

        if (id == NULL) {
            // No tasks left to delete
            break;
        } else {
            // Call FreeRTOS to delete the task (it will call FREERTOS_TASK_DELETE_HOOK)
            vTaskDelete(id);
        }
    }
}


static mp_obj_t threading_get_ident(void)
{
    return mp_obj_new_int_from_uint((mp_uint_t)xTaskGetCurrentTaskHandle());
}

static MP_DEFINE_CONST_FUN_OBJ_0(threading_get_ident_obj, threading_get_ident);


static mp_obj_t threading_main_thread(void)
{
    return MP_OBJ_FROM_PTR(thread_entry0);
}

static MP_DEFINE_CONST_FUN_OBJ_0(threading_main_thread_obj, threading_main_thread);


static mp_obj_t threading_enumerate(void)
{
    mp_obj_t list = mp_obj_new_list(0, NULL);

    mutex_lock(&thread_mutex, 1);

    for (mp_obj_threading_thread_t *th = thread; th != NULL; th = th->next) {
        if (!th->ready) {
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

MP_REGISTER_MODULE(MP_QSTR__threading, mp_module_threading);
