#include "freertos/task.h"
#include "freertos/idf_additions.h"

#include "../inc/threading_thread.h"
#include "../inc/common.h"


static const mp_obj_type_t mp_type_threading_thread_obj;



mp_const_empty_map

def __init__(group=None, target=None, name=None, args=(), kwargs={})


void vTaskDelete(TaskHandle_t xTaskToDelete)



void *threading_thread_entry(threading_thread_entry_args_t *args) {
    // Execution begins here for a new thread.  We do not have the GIL.

    state_thread_t ts;
    threading_init_state(&ts, args->stack_size, args->dict_locals, args->dict_globals);

    #if MICROPY_ENABLE_PYSTACK
    // TODO threading and pystack is not fully supported, for now just make a small stack
    mp_obj_t mini_pystack[128];
    mp_pystack_init(mini_pystack, &mini_pystack[128]);
    #endif

    // signal that we are set up and running

    mutex_lock(&thread_mutex, 1);
    for (mp_obj_threading_thread_t *th = thread; th != NULL; th = th->next) {
        if (th->id == xTaskGetCurrentTaskHandle()) {
            th->ready = 1;
            break;
        }
    }
    mutex_unlock(&thread_mutex);

    // TODO set more thread-specific state here:
    //  cur_exception (root pointer)

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_call_function_n_kw(args->fun, args->n_args, args->n_kw, args->args);
        nlr_pop();
    } else {
        // uncaught exception
        // check for SystemExit
        mp_obj_base_t *exc = (mp_obj_base_t *)nlr.ret_val;
        if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(exc->type), MP_OBJ_FROM_PTR(&mp_type_SystemExit))) {
            // swallow exception silently
        } else {
            // print exception out
            mp_printf(MICROPY_ERROR_PRINTER, "Unhandled exception in thread started by ");
            mp_obj_print_helper(MICROPY_ERROR_PRINTER, args->fun, PRINT_REPR);
            mp_printf(MICROPY_ERROR_PRINTER, "\n");
            mp_obj_print_exception(MICROPY_ERROR_PRINTER, MP_OBJ_FROM_PTR(exc));
        }
    }

    // signal that we are finished

    mutex_lock(&thread_mutex, 1);
    for (mp_obj_threading_thread_t *th = thread; th != NULL; th = th->next) {
        if (th->id == xTaskGetCurrentTaskHandle()) {
            th->ready = 0;
            break;
        }
    }
    mutex_unlock(&thread_mutex);

    return NULL;
}



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
    mp_obj_threading_boundedsemaphore_t *self = m_new_obj(mp_obj_threading_boundedsemaphore_t);
    self->base.type = &mp_type_threading_boundedsemaphore_t;

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


static mp_obj_t threading_thread__del__(mp_obj_t self_in)
{
    mp_obj_threading_thread_t *self = MP_OBJ_TO_PTR(self_in);

    xEventGroupSetBits(self->event.handle, 0);
    vEventGroupDelete(self->event.handle);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(threading_thread__del__obj, threading_thread__del__);


static mp_obj_t threading_thread_start(mp_obj_t self_in)
{
    mp_obj_threading_event_t *self = MP_OBJ_TO_PTR(self_in);

    BaseType_t core_id = xPortGetCoreID();

    BaseType_t xTaskCreatePinnedToCore(TaskFunction_t pxTaskCode, const char *const pcName, const uint32_t ulStackDepth, self, UBaseType_t uxPriority, &self->id, core_id)


    self->ident = mp_obj_new_int_from_uint(threading_thread_create(threading_thread_entry, self));

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(threading_thread_start_obj, threading_thread_start);


static mp_obj_t threading_thread_is_alive(mp_obj_t self_in)
{
    mp_obj_threading_event_t *self = MP_OBJ_TO_PTR(self_in);
    xEventGroupClearBits(self->event.handle, 0);
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(threading_thread_is_alive_obj, threading_thread_is_alive);


static void mp_threading_thread_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    mp_obj_threading_semaphore_t *self = MP_OBJ_TO_PTR(self_in);

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        if (attr == MP_QSTR_name) {
            dest[0] = mp_obj_new_int_from_uint(self->value);
        }
        if (attr == MP_QSTR_ident) {
            dest[0] = mp_obj_new_int_from_uint(self->value);
        }
        if (attr == MP_QSTR_native_id) {
            dest[0] = mp_obj_new_int_from_uint(self->value);
        }
    }
}



static const mp_rom_map_elem_t threading_thread_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&threading_thread_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&threading_thread_run_obj) },
    { MP_ROM_QSTR(MP_QSTR_join), MP_ROM_PTR(&threading_thread_join_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_alive), MP_ROM_PTR(&threading_thread_is_alive_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&threading_thread__del__obj) },
};

static MP_DEFINE_CONST_DICT(threading_thread_locals_dict, threading_thread_locals_dict_table);


static MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_threading_thread_t,
    MP_QSTR_Thread,
    MP_TYPE_FLAG_NONE,
    // print, mp_lv_grad_t_print,
    make_new, threading_thread_make_new,
    // binary_op, lv_struct_binary_op,
    // subscr, lv_struct_subscr,
    // attr, mp_threading_semaphore_attr,
    locals_dict, &threading_thread_locals_dict,
    // buffer, mp_blob_get_buffer,
    // parent, &mp_lv_base_struct_type
);
