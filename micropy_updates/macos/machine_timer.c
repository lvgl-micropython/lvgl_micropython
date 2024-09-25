

#include "py/obj.h"
#include "py/runtime.h"
#include "mphalport.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <mach/boolean.h>
#include <stdlib.h>

#include <dispatch/dispatch.h>


typedef struct {
	dispatch_queue_t tim_queue;
	dispatch_source_t tim_timer;
} macos_timer;


typedef struct _machine_timer_obj_t {
    mp_obj_base_t base;

    mp_uint_t group;
    mp_uint_t index;

    uint8_t repeat;
    uint64_t period;

    mp_obj_t callback;

    macos_timer *tim;

    struct _machine_timer_obj_t *next;
} machine_timer_obj_t;


const mp_obj_type_t machine_timer_type;


static void machine_timer_disable(machine_timer_obj_t *self);
static mp_obj_t machine_timer_init_helper(machine_timer_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);


static inline void _timer_cancel(void *arg)
{
    machine_timer_obj_t *self = arg;
	dispatch_release(self->tim->tim_timer);
	dispatch_release(self->tim->tim_queue);
	self->tim->tim_timer = NULL;
	self->tim->tim_queue = NULL;
	free(self->tim);
	self->tim = NULL;
}


static inline void _timer_handler(void *arg)
{
    machine_timer_obj_t *self = arg;

	if (self->callback != mp_const_none) {
	    mp_sched_schedule(self->callback, self);
	}

	if (self->repeat) {
	    dispatch_time_t start;
        start = dispatch_time(DISPATCH_TIME_NOW, self->period);

	    dispatch_source_set_timer(self->tim->tim_timer, start, self->period, 0);
	    dispatch_resume(self->tim->tim_timer);
	} else {
	    machine_timer_disable(self);
	}
}


static mp_obj_t machine_timer_make_new(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_id, ARG_mode, ARG_callback, ARG_period };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,           MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_mode,         MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 1 } },
        { MP_QSTR_callback,     MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_period,       MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 0 } }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

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
        self = m_new_obj(machine_timer_obj_t);
        self->base.type = &machine_timer_type;

        self->group = group;
        self->index = index;

        // Add the timer to the linked-list of timers
        self->next = MP_STATE_PORT(machine_timer_obj_head);
        MP_STATE_PORT(machine_timer_obj_head) = self;
    }

    mp_obj_t table[] = {
        MP_ROM_QSTR(MP_QSTR_mode);
        mp_obj_new_int_from_uint((mp_uint_t)mode);
        MP_ROM_QSTR(MP_QSTR_callback);
        args[ARG_callback].u_obj,
        MP_ROM_QSTR(MP_QSTR_period);
        mp_obj_new_int_from_uint((mp_uint_t)period);
    };

    mp_map_t k_args;
    mp_map_init_fixed_table(&k_args, n_args - 1, table);
    machine_timer_init_helper(self, n_args - 1, pos_args + 1, &k_args);
    return self;
}


static void machine_timer_disable(machine_timer_obj_t *self)
{
	if (self->tim != NULL) {
		dispatch_source_cancel(self->tim->tim_timer);
	}
}


static void machine_timer_enable(machine_timer_obj_t *self)
{
	struct macos_timer *tim;
	tim = (struct macos_timer *) malloc(sizeof (struct macos_timer));

	tim->tim_queue = dispatch_queue_create("org.openzfsonosx.timerqueue", 0);
	tim->tim_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, tim->tim_queue);

	self->tim = tim;

	/* Opting to use pure C instead of Block versions */
	dispatch_set_context(tim->tim_timer, self);
	dispatch_source_set_event_handler_f(tim->tim_timer, _timer_handler);
	dispatch_source_set_cancel_handler_f(tim->tim_timer, _timer_cancel);

	dispatch_time_t start;
    start = dispatch_time(DISPATCH_TIME_NOW, self->period);
	dispatch_source_set_timer(tim->tim_timer, start, self->period, 0);
	dispatch_resume(tim->tim_timer);
}


static mp_obj_t machine_timer_init_helper(machine_timer_obj_t *self, mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_mode, ARG_callback, ARG_period };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode,         MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = -1} },
        { MP_QSTR_callback,     MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = NULL } },
        { MP_QSTR_period,       MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = -1 } }
    };

    machine_timer_disable(self);

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (args[ARG_period].u_int != -1) {
        self->period = ((uint64_t)args[ARG_period].u_int) * 1000000;
    }
    if (args[ARG_mode].u_int != -1) {
        self->repeat = (uint8_t)args[ARG_mode].u_int;
    }

    if (args[ARG_callback].u_obj != NULL) {
        self->callback = args[ARG_callback].u_obj;
    }

    machine_timer_enable(self);

    return mp_const_none;
}


static mp_obj_t machine_timer_deinit(mp_obj_t self_in) {
    machine_timer_disable(self_in);
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_deinit_obj, machine_timer_deinit);


static mp_obj_t machine_timer_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_timer_init_helper(args[0], n_args - 1, args + 1, kw_args);
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

#define MICROPY_PY_MACHINE_EXTRA_GLOBALS { MP_ROM_QSTR(MP_QSTR_Timer), (mp_obj_t)&machine_timer_type }, \

