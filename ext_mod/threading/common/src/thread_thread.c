// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "thread_common.h"
#include "../inc/thread_thread.h"


void *thread_entry_cb(mp_obj_thread_thread_t *self) {
    // Execution begins here for a new thread.  We do not have the GIL.

    state_thread_t ts;
    threading_init_state(&ts, self->call_args->stack_size, self->call_args->dict_locals, self->call_args->dict_globals);

    #if MICROPY_ENABLE_PYSTACK
    // TODO threading and pystack is not fully supported, for now just make a small stack
    mp_obj_t mini_pystack[128];
    mp_pystack_init(mini_pystack, &mini_pystack[128]);
    #endif

    // signal that we are set up and running
    self->ready = 1;
    self->is_alive = true;

    // TODO set more thread-specific state here:
    //  cur_exception (root pointer)

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_call_function_n_kw(self->call_args->fun, self->call_args->n_args, self->call_args->n_kw, self->call_args->args);
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
            mp_obj_print_helper(MICROPY_ERROR_PRINTER, self->call_args->fun, PRINT_REPR);
            mp_printf(MICROPY_ERROR_PRINTER, "\n");
            mp_obj_print_exception(MICROPY_ERROR_PRINTER, MP_OBJ_FROM_PTR(exc));
        }
    }

    // signal that we are finished

    self->is_alive = false;
    self->ready = 0;
    return NULL;
}


static mp_obj_t thread_start(mp_obj_t self_in)
{
    mp_obj_thread_thread_t *self = MP_OBJ_TO_PTR(self_in);
    self->ident = mp_obj_new_int_from_uint(threading_thread_entry(thread_entry_cb, self));

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(thread_start_obj, thread_start);


static mp_obj_t thread_is_alive(mp_obj_t self_in)
{
    mp_obj_thread_thread_t *self = MP_OBJ_TO_PTR(self_in);
    return mp_obj_new_bool(self->is_alive);
}

MP_DEFINE_CONST_FUN_OBJ_1(thread_is_alive_obj, thread_is_alive);


void thread_attr_func(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    mp_obj_thread_thread_t *self = MP_OBJ_TO_PTR(self_in);

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        if (attr == MP_QSTR_name) {
            dest[0] =self->name;
        }
        if (attr == MP_QSTR_ident) {
            dest[0] = self->ident;
        }
    }
}
