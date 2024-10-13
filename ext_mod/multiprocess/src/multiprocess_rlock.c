
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "../inc/multiprocess_rlock.h"
#include "../inc/common.h"



static const mp_obj_type_t mp_type_process_rlock;


static mp_obj_process_rlock_t *mp_obj_new_process_rlock(void)
{
    mp_obj_process_rlock_t *self = mp_obj_malloc(mp_obj_process_rlock_t, &mp_type_process_rlock);
    mutex_init(&self->mutex);
    self->locked = false;
    return self;
}


static mp_obj_t process_rlock_acquire(size_t n_args, const mp_obj_t *args)
{
    mp_obj_process_rlock_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t res = mp_const_true;

    if (self->count == 0) {
        bool wait = true;

        if (n_args > 1) {
            wait = mp_obj_get_int(args[1]);
            // TODO support timeout arg
        }
        int ret = mutex_lock(&self->mutex, wait);

        if (ret == 0) {
            res = mp_const_false;
        } else if (ret == 1) {
            self->locked = true;
            res = mp_const_true;
        } else {
            mp_raise_OSError(-ret);
        }
    }

    if (res == mp_const_true) self->count += 1;
    return res;
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(process_lock_acquire_obj, 1, 3, process_lock_acquire);


static mp_obj_t process_rlock___enter__(size_t n_args, const mp_obj_t *args)
{
    return process_rlock_acquire(n_args, args);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(process_rlock___enter___obj, 1, 1, process_rlock___enter__);


static mp_obj_t process_rlock_release(mp_obj_t self_in)
{
    mp_obj_process_rlock_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->locked) {
        mp_raise_msg(&mp_type_RuntimeError, NULL);
    }
    self->count -= 1;

    if (self->count == 0) {
        self->locked = false;
        mutex_unlock(&self->mutex);
    }

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(process_rlock_release_obj, process_rlock_release);


static mp_obj_t process_rlock___exit__(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused

    return process_rlock_release(args[0]);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(process_rlock___exit___obj, 4, 4, process_rlock___exit__);


static const mp_rom_map_elem_t process_rlock_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_acquire), MP_ROM_PTR(&process_rlock_acquire_obj) },
    { MP_ROM_QSTR(MP_QSTR_release), MP_ROM_PTR(&process_rlock_release_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&process_rlock___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&process_rlock___exit___obj) },
};

static MP_DEFINE_CONST_DICT(process_rlock_locals_dict, process_rlock_locals_dict_table);


static MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_process_rlock,
    MP_QSTR_RLock,
    MP_TYPE_FLAG_NONE,
    locals_dict, &process_rlock_locals_dict
);