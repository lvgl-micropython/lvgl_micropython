// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "thread_semaphore.h"



void semaphore_attr_func(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    mp_obj_threading_semaphore_t *self = MP_OBJ_TO_PTR(self_in);

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        if (attr == MP_QSTR__value) {
            dest[0] = mp_obj_new_int_from_uint(self->value);
        }
    }
}


static mp_obj_t semaphore_acquire(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_blocking, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_blocking,     MP_ARG_BOOL, { .u_bool = true } },
        { MP_QSTR_timeout,      MP_ARG_OBJ, { .u_obj = mp_const_none } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_threading_semaphore_t *self = (mp_obj_threading_semaphore_t *)args[ARG_self].u_obj;

    bool blocking = (bool)args[ARG_blocking].u_bool;

    bool res;
    uint16_t count = uxSemaphoreGetCount(self->mutex.handle);

    lowers count

    if (!blocking) {
        if (count >= self->start_value) {
            res = false;
        } else {
            res = (bool)(pdTRUE == xSemaphoreTake(self->mutex.handle, 0));
        }
    } else {
        float timeout_f;

        if (args[ARG_timeout].u_obj != mp_const_none) {
            if (mp_obj_is_float(args[ARG_timeout].u_obj)) timeout_f = mp_obj_get_float_to_f(args[ARG_timeout].u_obj);
            else timeout_f = mp_obj_get_int(args[ARG_timeout].u_obj) * 1.0f;
        } else {
            timeout_f = -1.0f;
        }

        int32_t timeout = (int32_t)(timeout_f * 1000);
        if (count == 0) {
            self->waiting += 1;
        }

        res = (bool)(pdTRUE == xSemaphoreTake(self->mutex.handle, timeout < 0 ? portMAX_DELAY : pdMS_TO_TICKS((uint16_t)timeout)));
        if (res == true) {
            if (self->waiting > 0) {
                self->waiting -= 1;
            }
        }
    }

    if (res == true) {
        self->value += 1;
    }

    return mp_obj_new_bool(res);
}

MP_DEFINE_CONST_FUN_OBJ_KW(semaphore_acquire_obj, 1, semaphore_acquire);


static mp_obj_t semaphore__enter__(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    return threading_semaphore_acquire(n_args, pos_args, kw_args);
}

MP_DEFINE_CONST_FUN_OBJ_KW(semaphore__enter__obj, 1, semaphore__enter__);


static mp_obj_t semaphore_release(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_n };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,  MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_n,     MP_ARG_BOOL, { .u_int = 1 } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_threading_semaphore_t *self = (mp_obj_threading_semaphore_t *)args[ARG_self].u_obj;

    uint16_t n = (uint16_t)args[ARG_n].u_int;

    for (uint16_t i=0;i<n;i++) {
        if (self->value == 0 && self->waiting == 0) {
            mp_raise_msg(
                &mp_type_ValueError,
                MP_ERROR_TEXT("Unable to release a bounded semaphore that is not acquired"),
            );
            return mp_const_none;
        }
        if (self->value > 0) { 
            self->value -= 1;
        }
        xSemaphoreGive(self->mutex.handle);
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(semaphore_release_obj, 1, semaphore_release);


static mp_obj_t semaphore__exit__(size_t n_args, const mp_obj_t *args)
{
    (void)n_args; // unused
    
    mp_map_t *kw_args = NULL;
    const mp_obj_t pos_args[1];
    pos_args[0] = args[0];
    
    return threading_semaphore_release(1, pos_args, kw_args);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(semaphore__exit__obj, 4, 4, semaphore__exit__);


static mp_obj_t semaphore__del__(mp_obj_t self_in)
{
    mp_obj_threading_semaphore_t *self = MP_OBJ_TO_PTR(self_in);

    for (uint16_t i=self->value;i<self->start_value;i++) {
        xSemaphoreGive(self->mutex.handle);
    }

    vSemaphoreDelete(self->mutex.handle);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(semaphore__del__obj, semaphore__del__);

