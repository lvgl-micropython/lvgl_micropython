// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "thread_rlock.h"
#include "thread_common.h"


static mp_obj_t thread_rlock_acquire(size_t n_args, const mp_obj_t *args)
{
    mp_obj_thread_rlock_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t res = mp_const_true;

    if (self->count == 0) {
        bool wait = true;

        if (n_args > 1) {
            wait = mp_obj_get_int(args[1]);
            // TODO support timeout arg
        }
        int ret = lock_acquire(&self->mutex, wait);

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

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(thread_rlock_acquire_obj, 1, 3, thread_rlock_acquire);


static mp_obj_t thread_rlock__enter__(size_t n_args, const mp_obj_t *args)
{
    return thread_rlock_acquire(n_args, args);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(thread_rlock__enter__obj, 1, 1, thread_rlock__enter__);


static mp_obj_t thread_rlock_release(mp_obj_t self_in)
{
    mp_obj_thread_rlock_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->locked) {
        mp_raise_msg(&mp_type_RuntimeError, NULL);
    }
    self->count -= 1;

    if (self->count == 0) {
        self->locked = false;
        lock_release(&self->mutex);
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(thread_rlock_release_obj, thread_rlock_release);


static mp_obj_t thread_rlock__exit__(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused

    return thread_rlock_release(args[0]);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(thread_rlock__exit__obj, 4, 4, thread_rlock__exit__);
