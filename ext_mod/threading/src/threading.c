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

#if MICROPY_PY_THREAD

#define THREADING_MIN_STACK_SIZE                        (4 * 1024)
#define THREADING_DEFAULT_STACK_SIZE                    (THREADING_MIN_STACK_SIZE + 1024)
#define THREADING_PRIORITY                              (ESP_TASK_PRIO_MIN + 1)

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 2, 0) && !CONFIG_FREERTOS_ENABLE_STATIC_TASK_CLEAN_UP
#define THREADING_FREERTOS_TASK_DELETE_HOOK                       vTaskPreDeletionHook
#else
#define THREADING_FREERTOS_TASK_DELETE_HOOK                       vPortCleanUpTCB
#endif

// this structure forms a linked list, one node per active thread
typedef struct _threading_t {
    TaskHandle_t id;        // system id of thread
    int ready;              // whether the thread is ready and running
    void *arg;              // thread Python args, a GC root pointer
    void *stack;            // pointer to the stack
    size_t stack_len;       // number of words in the stack
    struct _threading_t *next;
} threading_t;

// the mutex controls access to the linked list
static mp_thread_mutex_t thread_mutex;
static threading_t threading_entry0;
static threading_t *thread = NULL; // root pointer, handled by threading_gc_others


void threading_init(void *stack, uint32_t stack_len) 
{
    threading_set_state(&mp_state_ctx.thread);
    // create the first entry in the linked list of all threads
    threading_entry0.id = xTaskGetCurrentTaskHandle();
    threading_entry0.ready = 1;
    threading_entry0.arg = NULL;
    threading_entry0.stack = stack;
    threading_entry0.stack_len = stack_len;
    threading_entry0.next = NULL;
    threading_mutex_init(&thread_mutex);

    // memory barrier to ensure above data is committed
    __sync_synchronize();

    // THREADING_FREERTOS_TASK_DELETE_HOOK needs the thread ready after thread_mutex is ready
    thread = &threading_entry0;
}


void threading_gc_others(void) 
{
    threading_mutex_lock(&thread_mutex, 1);
    for (threading_t *th = thread; th != NULL; th = th->next) {
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
    threading_mutex_unlock(&thread_mutex);
}


mp_state_thread_t *threading_get_state(void) 
{
    return pvTaskGetThreadLocalStoragePointer(NULL, 1);
}


void threading_set_state(mp_state_thread_t *state) 
{
    vTaskSetThreadLocalStoragePointer(NULL, 1, state);
}


mp_uint_t threading_get_id(void) 
{
    return (mp_uint_t)xTaskGetCurrentTaskHandle();
}


void threading_start(void) 
{
    threading_mutex_lock(&thread_mutex, 1);
    for (threading_t *th = thread; th != NULL; th = th->next) {
        if (th->id == xTaskGetCurrentTaskHandle()) {
            th->ready = 1;
            break;
        }
    }
    threading_mutex_unlock(&thread_mutex);
}


static void *(*ext_threading_entry)(void *) = NULL;


static void freertos_entry(void *arg) 
{
    if (ext_threading_entry) {
        ext_threading_entry(arg);
    }
    vTaskDelete(NULL);
    for (;;) {;
    }
}


mp_uint_t threading_create_ex(void *(*entry)(void *), void *arg, size_t *stack_size, int priority, char *name) 
{
    // store thread entry function into a global variable so we can access it
    ext_threading_entry = entry;

    if (*stack_size == 0) {
        *stack_size = THREADING_DEFAULT_STACK_SIZE; // default stack size
    } else if (*stack_size < THREADING_MIN_STACK_SIZE) {
        *stack_size = THREADING_MIN_STACK_SIZE; // minimum stack size
    }

    // Allocate linked-list node (must be outside thread_mutex lock)
    threading_t *th = m_new_obj(threading_t);

    threading_mutex_lock(&thread_mutex, 1);

    // create thread
    BaseType_t result = xTaskCreatePinnedToCore(freertos_entry, name, *stack_size / sizeof(StackType_t), arg, priority, &th->id, MP_TASK_COREID);
    if (result != pdPASS) {
        threading_mutex_unlock(&thread_mutex);
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("can't create thread"));
    }

    // add thread to linked list of all threads
    th->ready = 0;
    th->arg = arg;
    th->stack = pxTaskGetStackStart(th->id);
    th->stack_len = *stack_size / sizeof(uintptr_t);
    th->next = thread;
    thread = th;

    // adjust the stack_size to provide room to recover from hitting the limit
    *stack_size -= 1024;

    threading_mutex_unlock(&thread_mutex);

    return (mp_uint_t)th->id;
}


mp_uint_t threading_create(void *(*entry)(void *), void *arg, size_t *stack_size) 
{
    return threading_create_ex(entry, arg, stack_size, THREADING_PRIORITY, "mp_thread");
}


void threading_finish(void) 
{
    threading_mutex_lock(&thread_mutex, 1);
    for (threading_t *th = thread; th != NULL; th = th->next) {
        if (th->id == xTaskGetCurrentTaskHandle()) {
            th->ready = 0;
            break;
        }
    }
    threading_mutex_unlock(&thread_mutex);
}


// This is called from the FreeRTOS idle task and is not within Python context,
// so MP_STATE_THREAD is not valid and it does not have the GIL.
void THREADING_FREERTOS_TASK_DELETE_HOOK(void *tcb) 
{
    if (thread == NULL) {
        // threading not yet initialised
        return;
    }
    threading_t *prev = NULL;
    threading_mutex_lock(&thread_mutex, 1);
    for (threading_t *th = thread; th != NULL; prev = th, th = th->next) {
        // unlink the node from the list
        if ((void *)th->id == tcb) {
            if (prev != NULL) {
                prev->next = th->next;
            } else {
                // move the start pointer
                thread = th->next;
            }
            // The "th" memory will eventually be reclaimed by the GC.
            break;
        }
    }
    threading_mutex_unlock(&thread_mutex);
}


void threading_mutex_init(mp_thread_mutex_t *mutex) 
{
    // Need a binary semaphore so a lock can be acquired on one Python thread
    // and then released on another.
    mutex->handle = xSemaphoreCreateBinaryStatic(&mutex->buffer);
    xSemaphoreGive(mutex->handle);
}


int threading_mutex_lock(mp_thread_mutex_t *mutex, int wait) 
{
    return pdTRUE == xSemaphoreTake(mutex->handle, wait ? portMAX_DELAY : 0);
}


void threading_mutex_unlock(mp_thread_mutex_t *mutex) 
{
    xSemaphoreGive(mutex->handle);
}


void threading_deinit(void) 
{
    for (;;) {
        // Find a task to delete
        TaskHandle_t id = NULL;
        threading_mutex_lock(&thread_mutex, 1);
        for (threading_t *th = thread; th != NULL; th = th->next) {
            // Don't delete the current task
            if (th->id != xTaskGetCurrentTaskHandle()) {
                id = th->id;
                break;
            }
        }
        threading_mutex_unlock(&thread_mutex);

        if (id == NULL) {
            // No tasks left to delete
            break;
        } else {
            // Call FreeRTOS to delete the task (it will call THREADING_FREERTOS_TASK_DELETE_HOOK)
            vTaskDelete(id);
        }
    }
}


/****************************************************************/
// Lock object

static const mp_obj_type_t mp_type_threading_lock;


typedef struct _mp_obj_threading_lock_t {
    mp_obj_base_t base;
    mp_thread_mutex_t mutex;
    volatile bool locked;
} mp_obj_threading_lock_t;


static mp_obj_threading_lock_t *mp_obj_new_threading_lock(void) 
{
    mp_obj_threading_lock_t *self = mp_obj_malloc(mp_obj_threading_lock_t, &mp_type_threading_lock);
    threading_mutex_init(&self->mutex);
    self->locked = false;
    return self;
}


static mp_obj_t threading_lock_acquire(size_t n_args, const mp_obj_t *args) 
{
    mp_obj_threading_lock_t *self = MP_OBJ_TO_PTR(args[0]);
    bool wait = true;
    if (n_args > 1) {
        wait = mp_obj_get_int(args[1]);
        // TODO support timeout arg
    }
    MP_THREAD_GIL_EXIT();
    int ret = threading_mutex_lock(&self->mutex, wait);
    MP_THREAD_GIL_ENTER();
    if (ret == 0) {
        return mp_const_false;
    } else if (ret == 1) {
        self->locked = true;
        return mp_const_true;
    } else {
        mp_raise_OSError(-ret);
    }
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(threading_lock_acquire_obj, 1, 3, threading_lock_acquire);


static mp_obj_t threading_lock_release(mp_obj_t self_in) 
{
    mp_obj_threading_lock_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->locked) {
        mp_raise_msg(&mp_type_RuntimeError, NULL);
    }
    self->locked = false;
    MP_THREAD_GIL_EXIT();
    threading_mutex_unlock(&self->mutex);
    MP_THREAD_GIL_ENTER();
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(threading_lock_release_obj, threading_lock_release);


static mp_obj_t threading_lock_locked(mp_obj_t self_in) 
{
    mp_obj_threading_lock_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(self->locked);
}

static MP_DEFINE_CONST_FUN_OBJ_1(threading_lock_locked_obj, threading_lock_locked);


static mp_obj_t threading_lock___exit__(size_t n_args, const mp_obj_t *args) 
{
    (void)n_args; // unused
    return threading_lock_release(args[0]);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(threading_lock___exit___obj, 4, 4, threading_lock___exit__);


static const mp_rom_map_elem_t threading_lock_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acquire), MP_ROM_PTR(&threading_lock_acquire_obj) },
    { MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&threading_lock_release_obj) },
    { MP_ROM_QSTR(MP_QSTR_locked), MP_ROM_PTR(&threading_lock_locked_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&threading_lock_acquire_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&threading_lock___exit___obj) },
};

static MP_DEFINE_CONST_DICT(threading_lock_locals_dict, threading_lock_locals_dict_table);


static MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_threading_lock,
    MP_QSTR_lock,
    MP_TYPE_FLAG_NONE,
    locals_dict, &threading_lock_locals_dict
);

/****************************************************************/
// _thread module

static size_t threading_stack_size = 0;


static mp_obj_t mod_threading_get_ident(void) 
{
    return mp_obj_new_int_from_uint(threading_get_id());
}

static MP_DEFINE_CONST_FUN_OBJ_0(mod_threading_get_ident_obj, mod_threading_get_ident);


static mp_obj_t mod_threading_stack_size(size_t n_args, const mp_obj_t *args) 
{
    mp_obj_t ret = mp_obj_new_int_from_uint(threading_stack_size);
    if (n_args == 0) {
        threading_stack_size = 0;
    } else {
        threading_stack_size = mp_obj_get_int(args[0]);
    }
    return ret;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_threading_stack_size_obj, 0, 1, mod_threading_stack_size);


typedef struct _threading_entry_args_t {
    mp_obj_dict_t *dict_locals;
    mp_obj_dict_t *dict_globals;
    size_t stack_size;
    mp_obj_t fun;
    size_t n_args;
    size_t n_kw;
    mp_obj_t args[];
} threading_entry_args_t;


static void *threading_entry(void *args_in) 
{
    // Execution begins here for a new thread.  We do not have the GIL.

    threading_entry_args_t *args = (threading_entry_args_t *)args_in;

    mp_state_thread_t ts;
    threading_init_state(&ts, args->stack_size, args->dict_locals, args->dict_globals);

    #if MICROPY_ENABLE_PYSTACK
    // TODO threading and pystack is not fully supported, for now just make a small stack
    mp_obj_t mini_pystack[128];
    mp_pystack_init(mini_pystack, &mini_pystack[128]);
    #endif

    MP_THREAD_GIL_ENTER();

    // signal that we are set up and running
    threading_start();

    // TODO set more thread-specific state here:
    //  cur_exception (root pointer)

    DEBUG_printf("[thread] start ts=%p args=%p stack=%p\n", &ts, &args, MP_STATE_THREAD(stack_top));

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_call_function_n_kw(args->fun, args->n_args, args->n_kw, args->args);
        nlr_pop();
    } else {
        // uncaught exception
        // check for SystemExit
        mp_obj_base_t *exc = (mp_obj_base_t *)nlr.ret_val;
        if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(exc->type), MP_OBJ_FROM_PTR(&mp_type_SystemExit))) {
            // swallow exception silently
        } else {
            // print exception out
            mp_printf(MICROPY_ERROR_PRINTER, "Unhandled exception in thread started by ");
            mp_obj_print_helper(MICROPY_ERROR_PRINTER, args->fun, PRINT_REPR);
            mp_printf(MICROPY_ERROR_PRINTER, "\n");
            mp_obj_print_exception(MICROPY_ERROR_PRINTER, MP_OBJ_FROM_PTR(exc));
        }
    }

    DEBUG_printf("[thread] finish ts=%p\n", &ts);

    // signal that we are finished
    threading_finish();

    MP_THREAD_GIL_EXIT();

    return NULL;
}


static mp_obj_t mod_threading_start_new_thread(size_t n_args, const mp_obj_t *args) 
{
    // This structure holds the Python function and arguments for thread entry.
    // We copy all arguments into this structure to keep ownership of them.
    // We must be very careful about root pointers because this pointer may
    // disappear from our address space before the thread is created.
    threading_entry_args_t *th_args;

    // get positional arguments
    size_t pos_args_len;
    mp_obj_t *pos_args_items;
    mp_obj_get_array(args[1], &pos_args_len, &pos_args_items);

    // check for keyword arguments
    if (n_args == 2) {
        // just position arguments
        th_args = m_new_obj_var(threading_entry_args_t, args, mp_obj_t, pos_args_len);
        th_args->n_kw = 0;
    } else {
        // positional and keyword arguments
        if (mp_obj_get_type(args[2]) != &mp_type_dict) {
            mp_raise_TypeError(MP_ERROR_TEXT("expecting a dict for keyword args"));
        }
        mp_map_t *map = &((mp_obj_dict_t *)MP_OBJ_TO_PTR(args[2]))->map;
        th_args = m_new_obj_var(threading_entry_args_t, args, mp_obj_t, pos_args_len + 2 * map->used);
        th_args->n_kw = map->used;
        // copy across the keyword arguments
        for (size_t i = 0, n = pos_args_len; i < map->alloc; ++i) {
            if (mp_map_slot_is_filled(map, i)) {
                th_args->args[n++] = map->table[i].key;
                th_args->args[n++] = map->table[i].value;
            }
        }
    }

    // copy across the positional arguments
    th_args->n_args = pos_args_len;
    memcpy(th_args->args, pos_args_items, pos_args_len * sizeof(mp_obj_t));

    // pass our locals and globals into the new thread
    th_args->dict_locals = mp_locals_get();
    th_args->dict_globals = mp_globals_get();

    // set the stack size to use
    th_args->stack_size = threading_stack_size;

    // set the function for thread entry
    th_args->fun = args[0];

    // spawn the thread!
    return mp_obj_new_int_from_uint(threading_create(threading_entry, th_args, &th_args->stack_size));
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_threading_start_new_thread_obj, 2, 3, mod_threading_start_new_thread);


static mp_obj_t mod_threading_exit(void) 
{
    mp_raise_type(&mp_type_SystemExit);
}

static MP_DEFINE_CONST_FUN_OBJ_0(mod_threading_exit_obj, mod_threading_exit);


static mp_obj_t mod_threading_allocate_lock(void) 
{
    return MP_OBJ_FROM_PTR(mp_obj_new_threading_lock());
}

static MP_DEFINE_CONST_FUN_OBJ_0(mod_threading_allocate_lock_obj, mod_threading_allocate_lock);


static const mp_rom_map_elem_t mp_module_threading_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR__thread) },
    { MP_ROM_QSTR(MP_QSTR_LockType), MP_ROM_PTR(&mp_type_threading_lock) },
    { MP_ROM_QSTR(MP_QSTR_get_ident), MP_ROM_PTR(&mod_threading_get_ident_obj) },
    { MP_ROM_QSTR(MP_QSTR_stack_size), MP_ROM_PTR(&mod_threading_stack_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_new_thread), MP_ROM_PTR(&mod_threading_start_new_thread_obj) },
    { MP_ROM_QSTR(MP_QSTR_exit), MP_ROM_PTR(&mod_threading_exit_obj) },
    { MP_ROM_QSTR(MP_QSTR_allocate_lock), MP_ROM_PTR(&mod_threading_allocate_lock_obj) },
};

static MP_DEFINE_CONST_DICT(mp_module_threading_globals, mp_module_threading_globals_table);


const mp_obj_module_t mp_module_threading = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_threading_globals,
};

MP_REGISTER_MODULE(MP_QSTR__threading, mp_module_threading);

#endif // MICROPY_PY_THREAD


__excepthook__
get_ident()
get_native_id()
enumerate()
main_thread()
settrace(func)
settrace_all_threads(func)
gettrace()
setprofile(func)
setprofile_all_threads(func)
getprofile()
stack_size(size=None)
TIMEOUT_MAX

class local()

class Thread:

    def __init__(group=None, target=None, name=None, args=(), kwargs={})

    def start()
    def run()
    def join(timeout=None)
    name
    ident
    native_id
    is_alive()
    daemon


class Lock:

    def acquire(blocking=True, timeout=-1)
    def release()
    def locked()

class RLock:

    def acquire(blocking=True, timeout=-1)
    def release()

class Condition:

    def __init__(lock=None)
    def acquire(*args)
    def release()
    def wait(timeout=None)
    def wait_for(predicate, timeout=None)
    def notify(n=1)
    def notify_all()

class Semaphore:

    def __init__(value=1)
    def acquire(blocking=True, timeout=None)
    def release(n=1)


class BoundedSemaphore(Semaphore)

class Event:

    def is_set()
    def set()
    def clear()
    def wait(timeout=None)

