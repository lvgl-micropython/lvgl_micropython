// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "thread_lock.h"
#include "threading.h"


static mp_obj_t lock_acquire(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_blocking, ARG_timeout };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,    MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_blocking, MP_ARG_BOOL, { .u_bool = true } },
        { MP_QSTR_timeout, MP_ARG_OBJ, { .u_obj = mp_const_none } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_thread_lock_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);
    float timeout_f;

    if (args[ARG_timeout].u_obj != mp_const_none) {
        if (mp_obj_is_float(args[ARG_timeout].u_obj)) timeout_f = mp_obj_get_float_to_f(args[ARG_timeout].u_obj);
        else timeout_f = mp_obj_get_int(args[ARG_timeout].u_obj) * 1.0f;
    } else {
        timeout_f = -1.0f;
    }

    bool blocking = (bool)args[ARG_blocking].u_bool;
    int32_t timeout = (int32_t)(timeout_f * 1000);

    if (!blocking) {
        timeout = 0;
    }

    self->lock.ref_count += 1;
    int ret = threading_lock_acquire(&self->lock, timeout);

    if (ret == 0) {
        self->lock.ref_count -= 1;
        return mp_const_false;
    } else if (ret == 1) {
        return mp_const_true;
    } else {
        self->lock.ref_count -= 1;
        mp_raise_OSError(-ret);
        return mp_const_none;
    }
}

static MP_DEFINE_CONST_FUN_OBJ_KW(lock_acquire_obj, 3, lock_acquire);


static mp_obj_t lock__enter__(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    mp_obj_thread_lock_t *self = MP_OBJ_TO_PTR(args[0]);

    self->lock.ref_count += 1;
    threading_lock_acquire(&self->lock, -1);
    return mp_const_none
}


static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lock__enter__obj, 1, 1, lock__enter__);


static mp_obj_t lock_release(mp_obj_t self_in)
{
    mp_obj_thread_lock_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->lock.ref_count == 0) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("Lock is already released");
    } else {
        threading_lock_release(&self->mutex);
    }

    self->lock.ref_count -= 1;

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(lock_release_obj, lock_release);


static mp_obj_t lock__exit__(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    lock_release(args[0]);
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(lock__exit__obj, 4, 4, lock__exit__);


static mp_obj_t lock_locked(mp_obj_t self_in)
{
    mp_obj_thread_lock_t *self = MP_OBJ_TO_PTR(self_in);

    return mp_obj_new_bool(self->lock.ref_count != 0);
}

static MP_DEFINE_CONST_FUN_OBJ_1(lock_locked_obj, lock_locked);



static mp_obj_t thread_lock_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    THREAD_UNUSED(type);
    THREAD_UNUSED(n_args);
    THREAD_UNUSED(n_kw);
    THREAD_UNUSED(all_args);

    mp_obj_thread_lock_t *self = m_new_obj(mp_obj_thread_lock_t);
    self->base.type = &mp_type_threading_lock_t;

    threading_lock_init(&self->lock;
    self->locked = false;
    return MP_OBJ_FROM_PTR(self);
}


static const mp_rom_map_elem_t lock_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acquire), MP_ROM_PTR(&thread_lock_acquire_obj) },
    { MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&thread_lock_release_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&thread_lock__enter__obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&thread_lock__exit__obj) },
    { MP_ROM_QSTR(MP_QSTR_locked), MP_ROM_PTR(&thread_lock_locked_obj) },
};

static MP_DEFINE_CONST_DICT(lock_locals_dict, lock_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_threading_lock_t,
    MP_QSTR_Lock,
    MP_TYPE_FLAG_NONE,
    // print, mp_lv_grad_t_print,
    make_new, thread_lock_make_new,
    // binary_op, lv_struct_binary_op,
    // subscr, lv_struct_subscr,
    // attr, mp_threading_semaphore_attr,
    locals_dict, &lock_locals_dict
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);


static mp_obj_t multiprocess_lock_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    THREAD_UNUSED(type);
    THREAD_UNUSED(n_args);
    THREAD_UNUSED(n_kw);
    THREAD_UNUSED(all_args);

    mp_obj_thread_lock_t *self = m_new_obj(mp_obj_thread_lock_t);
    self->base.type = &mp_type_multiprocessing_lock_t;

    threading_lock_init(&self->lock);
    self->lock.ref_count = 0;
    return MP_OBJ_FROM_PTR(self);
}


MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_multiprocessing_lock_t,
    MP_QSTR_Lock,
    MP_TYPE_FLAG_NONE,
    // print, mp_lv_grad_t_print,
    make_new, multiprocess_lock_make_new,
    // binary_op, lv_struct_binary_op,
    // subscr, lv_struct_subscr,
    // attr, mp_threading_semaphore_attr,
    locals_dict, &lock_locals_dict
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);
