

#include "py/obj.h"
#include "py/runtime.h"
#include "mphalport.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <mach/boolean.h>
#include <stdlib.h>

#include <dispatch/dispatch.h>



typedef struct _machine_timer_obj_t {
    mp_obj_base_t base;

    mp_uint_t group;
    mp_uint_t index;

    uint8_t repeat;
    uint64_t period;

    mp_obj_t callback;

    dispatch_source_t tim;

    struct _machine_timer_obj_t *next;
} machine_timer_obj_t;


const mp_obj_type_t machine_timer_type;


static void machine_timer_disable(machine_timer_obj_t *self);
static void machine_timer_init_helper(machine_timer_obj_t *self, int16_t mode, mp_obj_t callback, int64_t period);


static mp_obj_t _run_scheduled_task(mp_obj_t self_in)
{
    printf("_run_scheduled_task\n");

    machine_timer_obj_t *self = (machine_timer_obj_t *)self_in;
	if (self->callback != mp_const_none) {
	    printf("_run_scheduled_task mp_call_function_1\n");
        mp_call_function_1(self->callback, self_in);
    }

    if (!self->repeat) {
	    printf("_run_scheduled_task machine_timer_disable\n");
	    machine_timer_disable(self);
	}

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(_run_scheduled_task_obj, _run_scheduled_task);


static inline void _timer_handler(void *arg)
{
    machine_timer_obj_t *self = (machine_timer_obj_t *)arg;
    printf("_timer_handler\n");
    if (self->tim != NULL) {
	   printf("_timer_handler mp_sched_schedule\n");
	   mp_sched_schedule(_run_scheduled_task, MP_OBJ_FROM_PTR(self));
	}
}


static mp_obj_t machine_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_mode, ARG_callback, ARG_period };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,           MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_mode,         MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 1 } },
        { MP_QSTR_callback,     MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_period,       MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 0 } }
    };
    printf("machine_timer_make_new\n");

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(
        n_args,
        n_kw,
        all_args,
        MP_ARRAY_SIZE(allowed_args),
        allowed_args,
        args
    );

    uint8_t id = (uint8_t)args[ARG_id].u_int;
    uint8_t mode = (uint8_t)args[ARG_mode].u_int;
    uint32_t period = (uint32_t)args[ARG_period].u_int;

    mp_uint_t group = (id >> 1) & 1;
    mp_uint_t index = id & 1;

    machine_timer_obj_t *self = NULL;

    // Check whether the timer is already initialized, if so use it
    for (machine_timer_obj_t *t = MP_STATE_PORT(machine_timer_obj_head); t; t = t->next) {
        if (t->group == group && t->index == index) {
            self = t;
            break;
        }
    }
    // The timer does not exist, create it.
    if (self == NULL) {
        printf("machine_timer_make_new m_new_obj\n");

        self = m_new_obj(machine_timer_obj_t);
        self->base.type = &machine_timer_type;

        self->group = group;
        self->index = index;

        // Add the timer to the linked-list of timers
        self->next = MP_STATE_PORT(machine_timer_obj_head);
        MP_STATE_PORT(machine_timer_obj_head) = self;
    }

    machine_timer_init_helper(self, (int16_t)mode, args[ARG_callback].u_obj, (int64_t)period);

    return self;
}


static void machine_timer_disable(machine_timer_obj_t *self)
{
    printf("machine_timer_disable\n");
	if (self->tim != NULL) {
	    printf("machine_timer_disable dispatch_suspend\n");
	    dispatch_suspend(self->tim);
	}
}


static void machine_timer_enable(machine_timer_obj_t *self)
{
    printf("machine_timer_enable\n");

    if (self->tim == NULL) {
	    self->tim = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
	    dispatch_set_context(self->tim, self);
	    dispatch_source_set_event_handler_f(self->tim, &_timer_handler);
	    // dispatch_source_set_cancel_handler_f(self->tim, &_timer_cancel);
	} else {
	    machine_timer_disable(self);
	}

	dispatch_source_set_timer(self->tim, DISPATCH_TIME_NOW, self->period, 0);
	dispatch_resume(self->tim);
}


static void machine_timer_init_helper(machine_timer_obj_t *self, int16_t mode, mp_obj_t callback, int64_t period)
{
    printf("machine_timer_init_helper\n");

    if (period != -1) self->period = ((uint64_t)period) * 1000000;
    if (mode != -1) self->repeat = (uint8_t)mode;
    if (callback != NULL) self->callback = callback;

    machine_timer_enable(self);
}


static mp_obj_t machine_timer_deinit(mp_obj_t self_in)
{
    printf("machine_timer_deinit\n");
    machine_timer_obj_t *self = (machine_timer_obj_t *)self_in;
    if (self->tim != NULL) {
        machine_timer_disable(self);
        dispatch_source_cancel(self->tim);
        dispatch_release(self->tim);
        self->tim = NULL;
    }
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_deinit_obj, machine_timer_deinit);


static mp_obj_t machine_timer_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{

    enum { ARG_self, ARG_mode, ARG_callback, ARG_period };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,         MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_mode,         MP_ARG_KW_ONLY  | MP_ARG_INT, { .u_int = -1 } },
        { MP_QSTR_callback,     MP_ARG_KW_ONLY  | MP_ARG_OBJ, { .u_obj = NULL } },
        { MP_QSTR_period,       MP_ARG_KW_ONLY  | MP_ARG_INT, { .u_int = -1 } }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    printf("machine_timer_init\n");

    machine_timer_init_helper(
        (machine_timer_obj_t *)args[ARG_self].u_obj,
        (int16_t)args[ARG_mode].u_int,
        args[ARG_callback].u_obj,
        (int64_t)args[ARG_period].u_int
    );

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_KW(machine_timer_init_obj, 1, machine_timer_init);


static const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_timer_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_timer_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_ONE_SHOT), MP_ROM_INT(false) },
    { MP_ROM_QSTR(MP_QSTR_PERIODIC), MP_ROM_INT(true) },
};

static MP_DEFINE_CONST_DICT(machine_timer_locals_dict, machine_timer_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    machine_timer_type,
    MP_QSTR_Timer,
    MP_TYPE_FLAG_NONE,
    make_new, machine_timer_make_new,
    locals_dict, &machine_timer_locals_dict
);


MP_REGISTER_ROOT_POINTER(struct _machine_timer_obj_t *machine_timer_obj_head);
