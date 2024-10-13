// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "thread_common.h"
#include "thread_semaphore.h"

#include "../inc/threading_semaphore.h"


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
    mp_obj_threading_semaphore_t *self = m_new_obj(mp_obj_threading_semaphore_t);
    self->base.type = &mp_type_threading_semaphore_t;

    int32_t start_value = (mint32_t)args[ARG_value].u_int;

    if (start_value < 0) {
        mp_raise_msg(
            &mp_type_ValueError,
            MP_ERROR_TEXT("Semaphore: start value cannot be less than zero."),
        );
        return mp_const_none;
    }

    self->start_value = (uint16_t)start_value;
    self->mutex.handle = xSemaphoreCreateCountingStatic(self->start_value, self->start_value, &self->mutex.buffer);

    return MP_OBJ_FROM_PTR(self);
}


static const mp_rom_map_elem_t threading_semaphore_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acquire), MP_ROM_PTR(&semaphore_acquire_obj) },
    { MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&semaphore_release_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&semaphore__enter__obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&semaphore__exit__obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&semaphore__del__obj) },
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
    locals_dict, &threading_semaphore_locals_dict,
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);

