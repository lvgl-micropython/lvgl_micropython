// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "thread_common.h"
#include "../inc/threading_thread.h"
#include "../inc/threading.h"


static mp_obj_t threading_thread_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_target, ARG_name, ARG_args, ARG_kwargs };
    const mp_arg_t make_new_args[] = {
        { MP_QSTR_target, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_name,   MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_args,   MP_ARG_OBJ, { .u_obj = mp_const_empty_tuple } },
        { MP_QSTR_kwargs, MP_ARG_OBJ, { .u_obj = mp_const_empty_dict_obj } }
    };

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
    mp_obj_thread_thread_t *self = m_new_obj(mp_obj_thread_thread_t);
    self->base.type = &mp_type_threading_thread_t;

    self->name = args[ARG_name].u_obj;
    mp_obj_tuple_t args = (mp_obj_tuple_t)args[ARG_args].u_obj;

    size_t len = sizeof(mp_obj_t) * (args.len + (2 * map->used));
    threading_thread_entry_args_t *call_args = (threading_thread_entry_args_t *)malloc(sizeof(threading_thread_entry_args_t) + len);
    self->call_args = call_args;

    for (uint8_t i=0;i<args.len;i++) {
        call_args->args[i] = args.items[i];
    }

    for (size_t i=0;i<;i++) {
        call_args->args[i] = args.items[i];
    }

    mp_map_t *map = &((mp_obj_dict_t *)MP_OBJ_TO_PTR((mp_obj_dict_t)args[ARG_kwargs].u_obj)->map;

    call_args->n_kw = map->used;
    // copy across the keyword arguments
    for (size_t i = 0, n = args.len; i < map->alloc; ++i) {
        if (mp_map_slot_is_filled(map, i)) {
            call_args->args[n++] = map->table[i].key;
            call_args->args[n++] = map->table[i].value;
        }
    }

    // copy across the positional arguments
    th_args->n_args = pos_args_len;
    memcpy(call_args->args, args.items, args.len * sizeof(mp_obj_t));

    // pass our locals and globals into the new thread
    call_args->dict_locals = mp_locals_get();
    call_args->dict_globals = mp_globals_get();

    // set the stack size to use
    call_args->stack_size = thread_stack_size;

    // set the function for thread entry
    call_args->fun = args[ARG_target].u_obj;

    return MP_OBJ_FROM_PTR(self);
}


static const mp_rom_map_elem_t threading_thread_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&thread_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_alive), MP_ROM_PTR(&thread_is_alive_obj) },
};

static MP_DEFINE_CONST_DICT(threading_thread_locals_dict, threading_thread_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_threading_thread_t,
    MP_QSTR_Thread,
    MP_TYPE_FLAG_NONE,
    // print, mp_lv_grad_t_print,
    make_new, threading_thread_make_new,
    // binary_op, lv_struct_binary_op,
    // subscr, lv_struct_subscr,
    attr, thread_attr_func,
    locals_dict, &threading_thread_locals_dict,
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);
