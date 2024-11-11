// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "threading.h"
#include "thread_event.h"



static mp_obj_t event_is_set(mp_obj_t self_in)
{
    mp_obj_event_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(self->event.is_set);
}

static MP_DEFINE_CONST_FUN_OBJ_1(event_is_set_obj, event_is_set);



static mp_obj_t event_set(mp_obj_t self_in)
{
    mp_obj_event_t *self = MP_OBJ_TO_PTR(self_in);
    self->event.is_set = true;
    threading_event_set(&self->event);
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(event_set_obj, event_set);



static mp_obj_t event_clear(mp_obj_t self_in)
{
    mp_obj_event_t *self = MP_OBJ_TO_PTR(self_in);
    self->event.is_set = false;
    threading_event_clear(&self->event);
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(event_clear_obj, event_clear);


static mp_obj_t event_wait(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,    MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_timeout, MP_ARG_OBJ, { .u_obj = mp_const_none } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_event_t *self = (mp_obj_event_t *)args[ARG_self].u_obj;

    float timeout_f;

    if (args[ARG_timeout].u_obj != mp_const_none) {
        if (mp_obj_is_float(args[ARG_timeout].u_obj)) timeout_f = mp_obj_get_float_to_f(args[ARG_timeout].u_obj);
        else timeout_f = mp_obj_get_int(args[ARG_timeout].u_obj) * 1.0f;
    } else {
        timeout_f = -1.0f;
    }

    int32_t timeout = (int32_t)(timeout_f * 1000);
    threading_event_wait(&self->event, timeout);
    return mp_const_none;
}


static MP_DEFINE_CONST_FUN_OBJ_KW(event_wait_obj, 1, event_wait);


static mp_obj_t event__del__(mp_obj_t self_in)
{
    mp_obj_event_t *self = MP_OBJ_TO_PTR(self_in);
    threading_event_delete(&self->event);
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(event__del__obj, event__del__);


static mp_obj_t thread_event_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    THREAD_UNUSED(type);
    THREAD_UNUSED(n_args);
    THREAD_UNUSED(n_kw);
    THREAD_UNUSED(all_args);

    // create new object
    mp_obj_event_t *self = m_new_obj(mp_obj_event_t);
    self->base.type = &mp_type_threading_event_t;
    self->event.is_set = false;
    threading_event_init(&self->event);
    return MP_OBJ_FROM_PTR(self);
}


static const mp_rom_map_elem_t event_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_wait), MP_ROM_PTR(&event_wait_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_set), MP_ROM_PTR(&event_is_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&event_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&event_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&event__del__obj) },
};

static MP_DEFINE_CONST_DICT(event_locals_dict, event_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_threading_event_t,
    MP_QSTR_Event,
    MP_TYPE_FLAG_NONE,
    // print, mp_lv_grad_t_print,
    make_new, thread_event_make_new,
    // binary_op, lv_struct_binary_op,
    // subscr, lv_struct_subscr,
    // attr, mp_threading_semaphore_attr,
    locals_dict, &event_locals_dict
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);


static mp_obj_t multiprocess_event_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    THREAD_UNUSED(type);
    THREAD_UNUSED(n_args);
    THREAD_UNUSED(n_kw);
    THREAD_UNUSED(all_args);

    // create new object
    mp_obj_event_t *self = m_new_obj(mp_obj_event_t);
    self->base.type = &mp_type_multiprocessing_event_t;
    self->event.is_set = false;
    threading_event_init(&self->event);
    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_multiprocessing_event_t,
    MP_QSTR_Event,
    MP_TYPE_FLAG_NONE,
    // print, mp_lv_grad_t_print,
    make_new, multiprocess_event_make_new,
    // binary_op, lv_struct_binary_op,
    // subscr, lv_struct_subscr,
    // attr, mp_threading_semaphore_attr,
    locals_dict, &event_locals_dict
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);
