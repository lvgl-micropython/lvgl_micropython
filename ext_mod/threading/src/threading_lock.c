#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "../inc/threading_lock.h"
#include "../inc/common.h"


static mp_obj_t threading_lock_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    THREAD_UNUSED(type);
    THREAD_UNUSED(n_args);
    THREAD_UNUSED(n_kw);
    THREAD_UNUSED(all_args);

    mp_obj_thread_lock_t *self = mp_obj_malloc(mp_obj_threading_lock_t, &mp_type_threading_lock_obj);
    mutex_init(&self->mutex);
    self->locked = false;
    return MP_OBJ_FROM_PTR(self);
}


static mp_obj_t threading_lock_acquire(size_t n_args, const mp_obj_t *args)
{
    mp_obj_threading_lock_t *self = MP_OBJ_TO_PTR(args[0]);
    bool wait = true;

    if (n_args > 1) {
        wait = mp_obj_get_int(args[1]);
        // TODO support timeout arg
    }

    int ret = mutex_lock(&self->mutex, wait);

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


static mp_obj_t threading_lock__enter__(size_t n_args, const mp_obj_t *args)
{
    return threading_lock_acquire(n_args, args);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(threading_lock__enter__obj, 1, 1, threading_lock__enter__);


static mp_obj_t threading_lock_release(mp_obj_t self_in)
{
    mp_obj_threading_lock_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->locked) {
        mp_raise_msg(&mp_type_RuntimeError, NULL);
    }
    self->locked = false;

    mutex_unlock(&self->mutex);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(threading_lock_release_obj, threading_lock_release);


static mp_obj_t threading_lock__exit__(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    return threading_lock_release(args[0]);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(threading_lock__exit__obj, 4, 4, threading_lock__exit__);


static mp_obj_t threading_lock_locked(mp_obj_t self_in)
{
    mp_obj_threading_lock_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(self->locked);
}

static MP_DEFINE_CONST_FUN_OBJ_1(threading_lock_locked_obj, threadg_lock_locked);


static const mp_rom_map_elem_t threading_lock_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acquire), MP_ROM_PTR(&threading_lock_acquire_obj) },
    { MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&threading_lock_release_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&threading_lock__enter__obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&threading_lock__exit__obj) },
    { MP_ROM_QSTR(MP_QSTR_locked), MP_ROM_PTR(&threading_lock_locked_obj) },
};

static MP_DEFINE_CONST_DICT(threading_lock_locals_dict, threading_lock_locals_dict_table);


static MP_DEFINE_CONST_OBJ_TYPE(
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
