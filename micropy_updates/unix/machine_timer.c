// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"
#include "py/runtime.h"
#include "mphalport.h"
#include "machine_timer.h"

#if !MICROPY_PY_THREAD
#error Timer requires MICROPY_PY_THREAD
#endif


#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include <string.h>

#include <pthread.h>
#include <unistd.h>

#include <termios.h>
#include <fcntl.h>

static const useconds_t TIMER_POLL_INTERVAL_US = 1000;  // 1 millisecond

static pthread_t timer_poll_thread_id;

bool timer_polling = false;
pthread_mutex_t timer_lock;


const mp_obj_type_t machine_timer_type;

static machine_timer_obj_t *timer_objs[TIMER_COUNT];


static void machine_timer_disable(machine_timer_obj_t *self);
static void machine_timer_init_helper(machine_timer_obj_t *self, int16_t mode, mp_obj_t callback, int32_t period);


static void *timer_poll_thread(void *arg)
{
    (void)arg;

    machine_timer_obj_t *timer;

    while (timer_polling) {
        usleep(TIMER_POLL_INTERVAL_US);

        pthread_mutex_lock(&timer_lock);

        for (uint8_t i=0;i<TIMER_COUNT;i++) {
            timer = timer_objs[i];

            if (timer != NULL && timer->active) {
                timer->ms_ticks += 1;

                if (timer->ms_ticks >= timer->period) {
                    timer->ms_ticks = 0;

                    if (timer->callback != NULL && timer->callback != mp_const_none) {
                        if (!timer->repeat) timer->active = false;

                        mp_sched_schedule(timer->callback, MP_OBJ_FROM_PTR(timer));
                    }
                }
            }
        }
        pthread_mutex_unlock(&timer_lock);
    }

    return NULL;
}


void machine_timer_deinit_all(void)
{
    // Disable, deallocate and remove all timers from list
    machine_timer_obj_t *timer;
    pthread_mutex_lock(&timer_lock);

    for (uint8_t i=0;i<TIMER_COUNT;i++) {
        timer = timer_objs[i];
        if (timer != NULL) {
            machine_timer_disable(timer);
        }
    }

    pthread_mutex_unlock(&timer_lock);

    timer_polling = false;
    pthread_join(timer_poll_thread_id, NULL);
    pthread_mutex_destroy(&timer_lock);

    timer = NULL;

    for (uint8_t i=0;i<TIMER_COUNT;i++) {
        timer = timer_objs[i];
        if (timer != NULL) {
            m_del_obj(machine_timer_obj_t, timer);
            timer_objs[i] = NULL;
        }
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

    if (id >= TIMER_COUNT) {
        mp_raise_msg_varg(
            &mp_type_ValueError,
            MP_ERROR_TEXT("Timer ID must be in range 0 - %d"),
            TIMER_COUNT
        );
        return mp_const_none;
    }

    if (!timer_polling) {
        pthread_mutex_init(&timer_lock, NULL);
        timer_polling = true;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&timer_poll_thread_id, &attr, &timer_poll_thread, NULL);
    }

    int16_t mode = (int16_t)args[ARG_mode].u_int;
    int32_t period = (int32_t)args[ARG_period].u_int;

    pthread_mutex_lock(&timer_lock);
    machine_timer_obj_t *self = timer_objs[id];

    // Check whether the timer is already initialized, if so use it
    // The timer does not exist, create it.
    if (self == NULL) {
        self = m_new_obj(machine_timer_obj_t);
        self->base.type = &machine_timer_type;

        self->id = id;
        self->callback = NULL;
        timer_objs[id] = self;
    } else {
        machine_timer_disable(self);
    }
    pthread_mutex_unlock(&timer_lock);

    machine_timer_init_helper(self, mode, args[ARG_callback].u_obj, period);

    return self;
}


static void machine_timer_disable(machine_timer_obj_t *self)
{
    self->active = false;
}


static void machine_timer_enable(machine_timer_obj_t *self)
{
    self->ms_ticks = 0;
	self->active = true;
}


static void machine_timer_init_helper(machine_timer_obj_t *self, int16_t mode, mp_obj_t callback, int32_t period)
{
    pthread_mutex_lock(&timer_lock);
    machine_timer_disable(self);

    if (period != -1) self->period = (uint16_t)period;
    if (mode != -1) self->repeat = (uint8_t)mode;
    if (callback != NULL) self->callback = callback;

    machine_timer_enable(self);
    pthread_mutex_unlock(&timer_lock);
}


static mp_obj_t machine_timer_del(mp_obj_t self_in)
{
    machine_timer_obj_t *self = (machine_timer_obj_t *)self_in;

    pthread_mutex_lock(&timer_lock);
    machine_timer_disable(self);

    if (timer_objs[self->id] != NULL) {
        timer_objs[self->id] = NULL;
        m_del_obj(machine_timer_obj_t, self);
    }

    pthread_mutex_unlock(&timer_lock);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(machine_timer_del_obj, machine_timer_del);


static mp_obj_t machine_timer_deinit(mp_obj_t self_in)
{
    machine_timer_obj_t *self = (machine_timer_obj_t *)self_in;

    pthread_mutex_lock(&timer_lock);
    machine_timer_disable(self);
    pthread_mutex_unlock(&timer_lock);

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
        (int32_t)args[ARG_period].u_int
    );

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_KW(machine_timer_init_obj, 1, machine_timer_init);


static const mp_rom_map_elem_t machine_timer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_timer_del_obj) },
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
