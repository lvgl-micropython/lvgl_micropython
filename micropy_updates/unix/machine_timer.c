

#include "py/obj.h"
#include "py/runtime.h"
#include "mphalport.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "SDL_timer.h"
#include "modmachine.h"


typedef struct _machine_timer_obj_t {
    mp_obj_base_t base;

    mp_uint_t group;
    mp_uint_t index;

    uint8_t repeat;
    uint32_t period;

    mp_obj_t callback;

    SDL_TimerID timer_id;

    struct _machine_timer_obj_t *next;
} machine_timer_obj_t;


const mp_obj_type_t machine_timer_type;


static void machine_timer_disable(machine_timer_obj_t *self);
static void machine_timer_init_helper(machine_timer_obj_t *self, int16_t mode, mp_obj_t callback, int64_t period);

void machine_timer_deinit_all(void) {
    // Disable, deallocate and remove all timers from list
    machine_timer_obj_t **t = &MP_STATE_PORT(machine_timer_obj_head);
    while (*t != NULL) {
        machine_timer_disable(*t);
        machine_timer_obj_t *next = (*t)->next;
        m_del_obj(machine_timer_obj_t, *t);
        *t = next;
    }
}


static uint32_t _timer_handler(uint32_t interval, void *arg)
{
    machine_timer_obj_t *self = (machine_timer_obj_t *)arg;
    if (self->callback == mp_const_none) {
        self->timer_id = 0;
        return 0;
    }

    if (self->timer_id == 0) {
        return 0;
    }

	mp_sched_schedule(self->callback, MP_OBJ_FROM_PTR(self));

	if (!self->repeat) {
	    self->timer_id = 0;
	    return 0;
	}

	return interval;
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
        self = m_new_obj(machine_timer_obj_t);
        self->base.type = &machine_timer_type;

        self->group = group;
        self->index = index;
        self->timer_id = 0;

        // Add the timer to the linked-list of timers
        self->next = MP_STATE_PORT(machine_timer_obj_head);
        MP_STATE_PORT(machine_timer_obj_head) = self;
    }

    machine_timer_init_helper(self, (int16_t)mode, args[ARG_callback].u_obj, (int64_t)period);

    return self;
}


static void machine_timer_disable(machine_timer_obj_t *self)
{
    if (self->timer_id != 0) {
        SDL_RemoveTimer(self->timer_id);
        self->timer_id = 0;
    }
}


static void machine_timer_enable(machine_timer_obj_t *self)
{
	self->timer_id = SDL_AddTimer(self->period, &_timer_handler, self);
}


static void machine_timer_init_helper(machine_timer_obj_t *self, int16_t mode, mp_obj_t callback, int64_t period)
{
    machine_timer_disable(self);

    if (period != -1) self->period = (uint32_t)period;
    if (mode != -1) self->repeat = (uint8_t)mode;
    if (callback != NULL) self->callback = callback;

    machine_timer_enable(self);
}


static mp_obj_t machine_timer_deinit(mp_obj_t self_in)
{
    machine_timer_disable(self);
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
