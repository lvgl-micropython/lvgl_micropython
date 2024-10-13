// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "thread_common.h"
#include "thread_event.h"
#include "../inc/threading_event.h"


static mp_obj_t threading_event_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    THREAD_UNUSED(type);
    THREAD_UNUSED(n_args);
    THREAD_UNUSED(n_kw);
    THREAD_UNUSED(all_args);

    // create new object
    mp_obj_threading_event_t *self = m_new_obj(mp_obj_threading_event_t);
    self->base.type = &mp_type_threading_event_t;

    self->event.handle = xEventGroupCreateStatic(&self->event.buffer);
    return MP_OBJ_FROM_PTR(self);
}

static const mp_rom_map_elem_t threading_event_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_wait), MP_ROM_PTR(&event_wait_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_set), MP_ROM_PTR(&event_is_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_set), MP_ROM_PTR(&event_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&event_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&event__del__obj) },
};

static MP_DEFINE_CONST_DICT(threading_event_locals_dict, threading_event_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_threading_event_t,
    MP_QSTR_Event,
    MP_TYPE_FLAG_NONE,
    // print, mp_lv_grad_t_print,
    make_new, threading_event_make_new,
    // binary_op, lv_struct_binary_op,
    // subscr, lv_struct_subscr,
    // attr, mp_threading_semaphore_attr,
    locals_dict, &threading_event_locals_dict,
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);
