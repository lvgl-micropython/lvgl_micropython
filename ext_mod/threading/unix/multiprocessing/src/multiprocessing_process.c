// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "thread_common.h"
#include "thread_thread.h"

#include "multiprocessing.h"
#include "multiprocessing_process.h"

#include <string.h>
#include <pthread.h>
#include <sys/sysinfo.h>

static cpu_set_t used_cores;

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
    if (processes == NULL) {
        process_count = (uint8_t)sysconf(_SC_NPROCESSORS_ONLN);
        processes = (mp_obj_t *)malloc(sizeof(mp_obj_t) * process_count);
        mp_obj_t main_t = threading_main_thread();
        processes[((mp_obj_thread_thread_t *)MP_OBJ_TO_PTR(main_t))->core_id] = main_t;
    }

    int new_core_id = -1;
    for (uint8_t i=0; i<process_count;i++) {
        if (processes[i] == NULL) {
            new_core_id = j;
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
