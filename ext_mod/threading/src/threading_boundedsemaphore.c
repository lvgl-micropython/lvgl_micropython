#include "../inc/threading_boundedsemaphore.h"


static const mp_obj_type_t mp_type_threading_boundedsemaphore_t;


static mp_obj_t mp_threading_boundedsemaphore_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_value };
    const mp_arg_t make_new_args[] = { { MP_QSTR_value, MP_ARG_INT, { .u_int = 1 } } };

    mp_arg_val_t args[MP_ARRAY_SIZE(make_new_args)];
    mp_arg_parse_all_kw_array(
        n_args,
        n_kw,
        all_args,
        MP_ARRAY_SIZE(make_new_args),
        make_new_args,
        args
    );

    // create new object
    mp_obj_threading_boundedsemaphore_t *self = m_new_obj(mp_obj_threading_boundedsemaphore_t);
    self->base.type = &mp_type_threading_boundedsemaphore_t;

    int32_t start_value = (mint32_t)args[ARG_value].u_int;

    if (start_value < 0) {
        mp_raise_msg(
            &mp_type_ValueError,
            MP_ERROR_TEXT("BoundedSemaphore: start value cannot be less than zero."),
        );
        return mp_const_none;
    }

    self->start_value = (uint16_t)start_value;
    self->mutex.handle = xSemaphoreCreateCountingStatic(self->start_value, self->start_value, &self->mutex.buffer);

    return MP_OBJ_FROM_PTR(self);
}




static void mp_threading_boundedsemaphore_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    mp_obj_threading_boundedsemaphore_t *self = MP_OBJ_TO_PTR(self_in);

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        if (attr == MP_QSTR__value) {
            dest[0] = mp_obj_new_int_from_uint(self->value);
        }
    }
}


static mp_obj_t threading_boundedsemaphore_acquire(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_blocking, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_blocking,     MP_ARG_BOOL, { .u_bool = true } },
        { MP_QSTR_timeout,      MP_ARG_OBJ, { .u_obj = mp_const_none } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_threading_boundedsemaphore_t *self = (mp_obj_threading_boundedsemaphore_t *)args[ARG_self].u_obj;

    bool blocking = (bool)args[ARG_blocking].u_bool;

    bool res;
    uint16_t count = uxSemaphoreGetCount(self->mutex.handle);

    lowers count

    if (!blocking) {
        if (count >= self->start_value) {
            res = false;
        } else {
            res = (bool)(pdTRUE == xSemaphoreTake(self->mutex.handle, 0));
        }
    } else {
        float timeout_f;

        if (args[ARG_timeout].u_obj != mp_const_none) {
            if (mp_obj_is_float(args[ARG_timeout].u_obj)) timeout_f = mp_obj_get_float_to_f(args[ARG_timeout].u_obj);
            else timeout_f = mp_obj_get_int(args[ARG_timeout].u_obj) * 1.0f;
        } else {
            timeout_f = -1.0f;
        }

        int timeout = (int)(timeout_f * 1000);
        if (count == 0) {
            self->waiting += 1;
        }

        res = (bool)(pdTRUE == xSemaphoreTake(self->mutex.handle, timeout < 0 ? portMAX_DELAY : pdMS_TO_TICKS(timeout)));
        if (res == true) {
            if (self->waiting > 0) {
                self->waiting -= 1;
            }
        }
    }

    if (res == true) {
        self->value += 1;
    }

    return mp_obj_new_bool(res);
}


MP_DEFINE_CONST_FUN_OBJ_KW(threading_boundedsemaphore_acquire_obj, 1, threading_boundedsemaphore_acquire);


static mp_obj_t threading_boundedsemaphore__enter__(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    return threading_boundedsemaphore_acquire(n_args, pos_args, kw_args);
}

MP_DEFINE_CONST_FUN_OBJ_KW(threading_boundedsemaphore__enter__obj, 1, threading_boundedsemaphore__enter__);


static mp_obj_t threading_boundedsemaphore_release(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_n };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_n,     MP_ARG_BOOL, { .u_int = 1 } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_threading_boundedsemaphore_t *self = (mp_obj_threading_boundedsemaphore_t *)args[ARG_self].u_obj;

    uint16_t n = (uint16_t)args[ARG_n].u_int;


    for (uint16_t i=0;i<n;i++) {
        if (self->value == 0 && self->waiting == 0) {
            mp_raise_msg(
                &mp_type_ValueError,
                MP_ERROR_TEXT("Unable to release a bounded semaphore that is not acquired"),
            );
            return mp_const_none;
        }
        if (self->value > 0) {
            self->value -= 1;
        }
        xSemaphoreGive(self->mutex.handle);
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(threading_boundedsemaphore_release_obj, 1, threading_boundedsemaphore_release);


static mp_obj_t threading_boundedsemaphore__exit__(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused

    mp_map_t *kw_args = NULL;
    const mp_obj_t pos_args[1];
    pos_args[0] = args[0];

    return threading_boundedsemaphore_release(1, pos_args, kw_args);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(threading_boundedsemaphore__exit__obj, 4, 4, threading_boundedsemaphore__exit__);


static const mp_rom_map_elem_t threading_boundedsemaphore_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acquire), MP_ROM_PTR(&threading_boundedsemaphore_acquire_obj) },
    { MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&threading_boundedsemaphore_release_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&threading_boundedsemaphore__enter__obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&threading_boundedsemaphore__exit__obj) },
};

static MP_DEFINE_CONST_DICT(threading_boundedsemaphore_locals_dict, threading_boundedsemaphore_locals_dict_table);


static MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_threading_boundedsemaphore_t,
    MP_QSTR_BoundedSemaphore,
    MP_TYPE_FLAG_NONE,
    // print, mp_lv_grad_t_print,
    make_new, mp_threading_boundedsemaphore_make_new,
    // binary_op, lv_struct_binary_op,
    // subscr, lv_struct_subscr,
    attr, mp_threading_boundedsemaphore_attr,
    locals_dict, &threading_boundedsemaphore_locals_dict,
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);




static const mp_rom_map_elem_t threading_boundedsemaphore_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acquire), MP_ROM_PTR(&threading_boundedsemaphore_acquire_obj) },
    { MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&threading_boundedsemaphore_release_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&threading_boundedsemaphore___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&threading_boundedsemaphore___exit___obj) },
    { MP_ROM_QSTR(MP_QSTR_locked), MP_ROM_PTR(&threading_boundedsemaphore_locked_obj) },
};

static MP_DEFINE_CONST_DICT(threading_boundedsemaphore_locals_dict, threading_boundedsemaphore_locals_dict_table);


static MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_threading_boundedsemaphore_obj,
    MP_QSTR_BoundedSemaphore,
    MP_TYPE_FLAG_NONE,
    locals_dict, &threading_boundedsemaphore_locals_dict
);