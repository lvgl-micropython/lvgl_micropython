// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "thread_common.h"
#include "thread_rlock.h"

#include "../inc/threading_lock.h"


static mp_obj_t threading_lock_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    THREAD_UNUSED(type);
    THREAD_UNUSED(n_args);
    THREAD_UNUSED(n_kw);
    THREAD_UNUSED(all_args);

    mp_obj_thread_lock_t *self = m_new_obj(mp_obj_thread_lock_t);
    self->base.type = &mp_type_threading_lock_t;

    mutex_init(&self->mutex);
    self->locked = false;
    return MP_OBJ_FROM_PTR(self);
}


static const mp_rom_map_elem_t threading_lock_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acquire), MP_ROM_PTR(&lock_acquire_obj) },
    { MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&lock_release_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&lock__enter__obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&lock__exit__obj) },
    { MP_ROM_QSTR(MP_QSTR_locked), MP_ROM_PTR(&lock_locked_obj) },
};

static MP_DEFINE_CONST_DICT(threading_lock_locals_dict, threading_lock_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_threading_lock_t,
    MP_QSTR_Lock,
    MP_TYPE_FLAG_NONE,
    // print, mp_lv_grad_t_print,
    make_new, threading_lock_make_new,
    // binary_op, lv_struct_binary_op,
    // subscr, lv_struct_subscr,
    // attr, mp_threading_semaphore_attr,
    locals_dict, &threading_lock_locals_dict,
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);
