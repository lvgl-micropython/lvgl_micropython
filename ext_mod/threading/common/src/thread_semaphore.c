// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>

#include "thread_semaphore.h"


static void semaphore_attr_func(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    mp_obj_thread_semaphore_t *self = MP_OBJ_TO_PTR(self_in);

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        if (attr == MP_QSTR__value) {
            dest[0] = mp_obj_new_int_from_uint(self->value);
        } else {
            const mp_obj_type_t *type = mp_obj_get_type(self_in);

            while (MP_OBJ_TYPE_HAS_SLOT(type, locals_dict)) {
                // generic method lookup
                // this is a lookup in the object (ie not class or type)
                assert(MP_OBJ_TYPE_GET_SLOT(type, locals_dict)->base.type == &mp_type_dict); // MicroPython restriction, for now
                mp_map_t *locals_map = &MP_OBJ_TYPE_GET_SLOT(type, locals_dict)->map;
                mp_map_elem_t *elem = mp_map_lookup(locals_map, MP_OBJ_NEW_QSTR(attr), MP_MAP_LOOKUP);
                if (elem != NULL) {
                    mp_convert_member_lookup(self_in, type, elem->value, dest);
                    break;
                }
                if (MP_OBJ_TYPE_GET_SLOT_OR_NULL(type, parent) == NULL) {
                    break;
                }
                // search parents
                type = MP_OBJ_TYPE_GET_SLOT(type, parent);
            }
        }
    }
}


static mp_obj_t thread_semaphore_acquire(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_blocking, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_blocking,     MP_ARG_BOOL, { .u_bool = true } },
        { MP_QSTR_timeout,      MP_ARG_OBJ, { .u_obj = mp_const_none } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_thread_semaphore_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);

    bool blocking = (bool)args[ARG_blocking].u_bool;

    bool res;

    uint16_t
    bool threading_semaphore_acquire(thread_semaphore_t *sem, int32_t wait_ms);
    void threading_semaphore_release(thread_semaphore_t *sem);
    void threading_semaphore_init(thread_semaphore_t *sem);
    void threading_semaphore_delete(thread_semaphore_t *sem);

    uint16_t count = threading_semaphore_get_count(&self->sem);

    if (!blocking) {
        if (count >= self->start_value) {
            res = false;
        } else {
            res =  threading_semaphore_acquire(&self->sem, 0);
        }
    } else {
        float timeout_f;

        if (args[ARG_timeout].u_obj != mp_const_none) {
            if (mp_obj_is_float(args[ARG_timeout].u_obj)) timeout_f = mp_obj_get_float_to_f(args[ARG_timeout].u_obj);
            else timeout_f = mp_obj_get_int(args[ARG_timeout].u_obj) * 1.0f;
        } else {
            timeout_f = -1.0f;
        }

        int32_t timeout = (int32_t)(timeout_f * 1000);
        if (count == 0) {
            self->waiting += 1;
        }

        res = threading_semaphore_acquire(&self->sem, timeout);

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

static MP_DEFINE_CONST_FUN_OBJ_KW(thread_semaphore_acquire_obj, 1, thread_semaphore_acquire);


static mp_obj_t thread_semaphore__enter__(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    return thread_semaphore_acquire(n_args, pos_args, kw_args);
}

static MP_DEFINE_CONST_FUN_OBJ_KW(thread_semaphore__enter__obj, 1, thread_semaphore__enter__);


static mp_obj_t thread_semaphore_release(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_n };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,  MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_n,     MP_ARG_BOOL, { .u_int = 1 } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_thread_semaphore_t *self = (mp_obj_thread_semaphore_t *)args[ARG_self].u_obj;

    uint16_t n = (uint16_t)args[ARG_n].u_int;

    for (uint16_t i=0;i<n;i++) {
        if (self->value == 0 && self->waiting == 0) {
            mp_raise_msg(
                &mp_type_ValueError,
                MP_ERROR_TEXT("Unable to release a bounded semaphore that is not acquired")
            );
            return mp_const_none;
        }
        if (self->value > 0) { 
            self->value -= 1;
        }
        threading_semaphore_release(&self->sem);
    }
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_KW(thread_semaphore_release_obj, 1, thread_semaphore_release);


static mp_obj_t thread_semaphore__exit__(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    
    mp_map_t *kw_args = NULL;
    const mp_obj_t pos_args[1] = { args[0], };
    return thread_semaphore_release(1, pos_args, kw_args);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(thread_semaphore__exit__obj, 4, 4, thread_semaphore__exit__);


static mp_obj_t thread_semaphore__del__(mp_obj_t self_in)
{
    mp_obj_thread_semaphore_t *self = MP_OBJ_TO_PTR(self_in);

    for (uint16_t i=self->value;i<self->start_value;i++) {
        threading_semaphore_release(&self->sem);
    }

    threading_semaphore_delete(&self->sem);
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(thread_semaphore__del__obj, thread_semaphore__del__);


static mp_obj_t threading_semaphore_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
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
    mp_obj_thread_semaphore_t *self = m_new_obj(mp_obj_thread_semaphore_t);
    self->base.type = &mp_type_threading_semaphore_t;

    int32_t start_value = (int32_t)args[ARG_value].u_int;

    if (start_value < 0) {
        mp_raise_msg(
            &mp_type_ValueError,
            MP_ERROR_TEXT("Semaphore: start value cannot be less than zero.")
        );
        return mp_const_none;
    }

    threading_semaphore_init(&self->sem, (uint16_t)start_value)
    self->start_value = (uint16_t)start_value;

    return MP_OBJ_FROM_PTR(self);
}



static const mp_rom_map_elem_t threading_semaphore_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acquire), MP_ROM_PTR(&thread_semaphore_acquire_obj) },
    { MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&thread_semaphore_release_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&thread_semaphore__enter__obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&thread_semaphore__exit__obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&thread_semaphore__del__obj) },
};

static MP_DEFINE_CONST_DICT(threading_semaphore_locals_dict, threading_semaphore_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_threading_semaphore_t,
    MP_QSTR_Semaphore,
    MP_TYPE_FLAG_NONE,
    // print, mp_lv_grad_t_print,
    make_new, threading_semaphore_make_new,
    // binary_op, lv_struct_binary_op,
    // subscr, lv_struct_subscr,
    attr, semaphore_attr_func,
    locals_dict, &threading_semaphore_locals_dict
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);


static mp_obj_t multiprocessing_semaphore_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
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
    mp_obj_thread_semaphore_t *self = m_new_obj(mp_obj_thread_semaphore_t);
    self->base.type = &mp_type_multiprocessing_semaphore_t;

    int32_t start_value = (int32_t)args[ARG_value].u_int;

    if (start_value < 0) {
        mp_raise_msg(
            &mp_type_ValueError,
            MP_ERROR_TEXT("Semaphore: start value cannot be less than zero.")
        );
        return mp_const_none;
    }

    threading_semaphore_init(&self->sem, (uint16_t)start_value);
    self->start_value = (uint16_t)start_value;

    return MP_OBJ_FROM_PTR(self);
}


static const mp_rom_map_elem_t multiprocessing_semaphore_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acquire), MP_ROM_PTR(&thread_semaphore_acquire_obj) },
    { MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&thread_semaphore_release_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&thread_semaphore__enter__obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&thread_semaphore__exit__obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&thread_semaphore__del__obj) },
};

static MP_DEFINE_CONST_DICT(multiprocessing_semaphore_locals_dict, multiprocessing_semaphore_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_multiprocessing_semaphore_t,
    MP_QSTR_Semaphore,
    MP_TYPE_FLAG_NONE,
    // print, mp_lv_grad_t_print,
    make_new, multiprocessing_semaphore_make_new,
    // binary_op, lv_struct_binary_op,
    // subscr, lv_struct_subscr,
    attr, semaphore_attr_func,
    locals_dict, &multiprocessing_semaphore_locals_dict
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);

