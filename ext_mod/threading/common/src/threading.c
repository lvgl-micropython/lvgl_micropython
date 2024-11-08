
#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/stackctrl.h"
#include "py/mpthread.h"
#include "mpthreadport.h"
#include "py/gc.h"

#include "stdio.h"

#include "thread_port.h"
#include "threading.h"
#include "thread_thread.h"
#include "thread_lock.h"
#include "thread_rlock.h"
#include "thread_semaphore.h"
#include "thread_event.h"


static mp_obj_t get_ident(void)
{
    return mp_obj_new_int_from_uint((mp_uint_t)mp_get_current_thread_id());
}

static MP_DEFINE_CONST_FUN_OBJ_0(get_ident_obj, get_ident);


mp_obj_t mp_get_main_thread(void)
{
    return MP_OBJ_FROM_PTR(&_main_thread);
}


static MP_DEFINE_CONST_FUN_OBJ_0(threading_main_thread_obj, mp_get_main_thread);


mp_obj_t mp_enumerate_threads(void)
{
    mp_obj_t list = mp_obj_new_list(0, NULL);

    lock_acquire(&t_mutex, 1);

    for (mp_obj_thread_thread_t *th = t_thread; th != NULL; th = th->next) {
        if (!th->is_alive) {
            continue;
        }
        mp_obj_list_append(list, MP_OBJ_FROM_PTR(th));
    }

    lock_release(&t_mutex);
    return list;
}

static MP_DEFINE_CONST_FUN_OBJ_0(threading_enumerate_obj, mp_enumerate_threads);


static mp_obj_t threading_stack_size(size_t n_args, const mp_obj_t *args) {
    mp_obj_t ret = mp_obj_new_int_from_uint(thread_stack_size);
    if (n_args == 1) {
        thread_stack_size = mp_obj_get_int(args[0]);
    }

    return ret;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(threading_stack_size_obj, 0, 1, threading_stack_size);


static const mp_rom_map_elem_t mp_module_threading_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_threading) },
    { MP_ROM_QSTR(MP_QSTR_RLock), (mp_obj_t)&mp_type_threading_rlock_t },
    { MP_ROM_QSTR(MP_QSTR_Lock), (mp_obj_t)&mp_type_threading_lock_t },
    { MP_ROM_QSTR(MP_QSTR_Event), (mp_obj_t)&mp_type_threading_event_t },
    { MP_ROM_QSTR(MP_QSTR_Thread), (mp_obj_t)&mp_type_threading_thread_t },
    { MP_ROM_QSTR(MP_QSTR_Semaphore), (mp_obj_t)&mp_type_threading_semaphore_t },
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
