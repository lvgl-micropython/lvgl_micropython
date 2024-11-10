// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "threading.h"
#include "thread_thread.h"



static mp_obj_t thread_start(mp_obj_t self_in)
{
    mp_obj_thread_thread_t *self = MP_OBJ_TO_PTR(self_in);
    self->ident = mp_obj_new_int_from_uint(threading_create_thread(self));

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(thread_start_obj, thread_start);


static mp_obj_t thread_is_alive(mp_obj_t self_in)
{
    mp_obj_thread_thread_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(self->is_alive);
}

static MP_DEFINE_CONST_FUN_OBJ_1(thread_is_alive_obj, thread_is_alive);


static void thread_attr_func(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    mp_obj_thread_thread_t *self = MP_OBJ_TO_PTR(self_in);

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        if (attr == MP_QSTR_name) {
            dest[0] = self->name;
        } else if (attr == MP_QSTR_ident) {
            dest[0] = self->ident;
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


static mp_obj_t threading_thread_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_target, ARG_name, ARG_args, ARG_kwargs };
    const mp_arg_t make_new_args[] = {
        { MP_QSTR_target, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_name,   MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_args,   MP_ARG_OBJ, { .u_obj = mp_obj_new_tuple(0, NULL) } },
        { MP_QSTR_kwargs, MP_ARG_OBJ, { .u_obj = mp_obj_new_dict(0) } }
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
    mp_obj_tuple_t *t_args = (mp_obj_tuple_t *)MP_OBJ_TO_PTR(args[ARG_args].u_obj);

    mp_map_t *map = &(((mp_obj_dict_t *)args[ARG_kwargs].u_obj)->map);

    size_t len = sizeof(mp_obj_t) * (t_args->len + (2 * map->used));
    thread_entry_args_t *call_args = (thread_entry_args_t *)malloc(sizeof(thread_entry_args_t) + len);
    self->call_args = call_args;

    call_args->n_kw = map->used;
    // copy across the keyword arguments
    for (size_t i = 0, n = t_args->len; i < map->alloc; ++i) {
        if (mp_map_slot_is_filled(map, i)) {
            call_args->args[n++] = map->table[i].key;
            call_args->args[n++] = map->table[i].value;
        }
    }

    // copy across the positional arguments
    call_args->n_args = t_args->len;
    memcpy(call_args->args, t_args->items, t_args->len * sizeof(mp_obj_t));

    // pass our locals and globals into the new thread
    call_args->dict_locals = mp_locals_get();
    call_args->dict_globals = mp_globals_get();

    // set the stack size to use
    call_args->stack_size = thread_stack_size;

    // set the function for thread entry
    call_args->fun = args[ARG_target].u_obj;

    self->core_id = mp_get_current_process_core();

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
    locals_dict, &threading_thread_locals_dict
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);


static mp_obj_t multiprocessing_process_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_target, ARG_name, ARG_args, ARG_kwargs };
    const mp_arg_t make_new_args[] = {
        { MP_QSTR_target, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_name,   MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_args,   MP_ARG_OBJ, { .u_obj = mp_obj_new_tuple(0, NULL) } },
        { MP_QSTR_kwargs, MP_ARG_OBJ, { .u_obj = mp_obj_new_dict(0) } }
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

    uint16_t core_id = mp_get_current_process_core();
    int16_t new_core_id = -1;

    for (uint16_t i=0;i<2;i++) {
        if (core_id == i) {
            continue;
        }
        if (processes[i] == NULL) {
            new_core_id = (int16_t)i;
            break;
        }
    }

    if (new_core_id == -1) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("All cores have processes running on them."));
        return mp_const_none;
    }

    // create new object
    mp_obj_thread_thread_t *self = m_new_obj(mp_obj_thread_thread_t);
    self->base.type = &mp_type_multiprocessing_process_t;

    self->name = args[ARG_name].u_obj;
    mp_obj_tuple_t *t_args = (mp_obj_tuple_t *)MP_OBJ_TO_PTR(args[ARG_args].u_obj);

    mp_map_t *map = &(((mp_obj_dict_t *)args[ARG_kwargs].u_obj)->map);

    size_t len = sizeof(mp_obj_t) * (t_args->len + (2 * map->used));
    thread_entry_args_t *call_args = (thread_entry_args_t *)malloc(sizeof(thread_entry_args_t) + len);
    self->call_args = call_args;

    call_args->n_kw = map->used;
    // copy across the keyword arguments
    for (size_t i = 0, n = t_args->len; i < map->alloc; ++i) {
        if (mp_map_slot_is_filled(map, i)) {
            call_args->args[n++] = map->table[i].key;
            call_args->args[n++] = map->table[i].value;
        }
    }

    // copy across the positional arguments
    call_args->n_args = t_args->len;
    memcpy(call_args->args, t_args->items, t_args->len * sizeof(mp_obj_t));

    // pass our locals and globals into the new thread
    call_args->dict_locals = mp_locals_get();
    call_args->dict_globals = mp_globals_get();

    // set the stack size to use
    call_args->stack_size = thread_stack_size;

    // set the function for thread entry
    call_args->fun = args[ARG_target].u_obj;

    self->core_id = (uint8_t)new_core_id;

    mp_obj_t res = MP_OBJ_FROM_PTR(self);
    processes[new_core_id] = res;
    return res;
}


static const mp_rom_map_elem_t multiprocessing_process_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&thread_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_alive), MP_ROM_PTR(&thread_is_alive_obj) },
};

static MP_DEFINE_CONST_DICT(multiprocessing_process_locals_dict, multiprocessing_process_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_multiprocessing_process_t,
    MP_QSTR_Process,
    MP_TYPE_FLAG_NONE,
    // print, mp_lv_grad_t_print,
    make_new, multiprocessing_process_make_new,
    // binary_op, lv_struct_binary_op,
    // subscr, lv_struct_subscr,
    attr, thread_attr_func,
    locals_dict, &multiprocessing_process_locals_dict
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);
