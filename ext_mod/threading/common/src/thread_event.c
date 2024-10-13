// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "thread_event.h"


static mp_obj_t event_is_set(mp_obj_t self_in)
{
    mp_obj_threading_event_t *self = MP_OBJ_TO_PTR(self_in);
    bool res = (bool)(xEventGroupGetBits(self->event.handle) == 1);
    return mp_obj_new_bool(res);
}

MP_DEFINE_CONST_FUN_OBJ_1(event_is_set_obj, event_is_set);



static mp_obj_t event_set(mp_obj_t self_in)
{
    mp_obj_threading_event_t *self = MP_OBJ_TO_PTR(self_in);
    xEventGroupSetBits(self->event.handle, 0);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(event_set_obj, event_set);



static mp_obj_t event_clear(mp_obj_t self_in)
{
    mp_obj_threading_event_t *self = MP_OBJ_TO_PTR(self_in);
    xEventGroupClearBits(self->event.handle, 0);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(event_clear_obj, event_clear);


static mp_obj_t event_wait(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,    MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_timeout, MP_ARG_OBJ, { .u_obj = mp_const_none } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_threading_event_t *self = (mp_obj_threading_event_t *)args[ARG_self].u_obj;

    float timeout_f;

    if (args[ARG_timout].u_obj != mp_const_none) {
        if (mp_obj_is_float(args[ARG_timeout].u_obj)) timeout_f = mp_obj_get_float_to_f(args[ARG_timeout].u_obj);
        else timeout_f = mp_obj_get_int(args[ARG_timeout].u_obj) * 1.0f;
    } else {
        timeout_f = -1.0f;
    }

    int32_t timeout = (int32_t)(timeout_f * 1000);

    xEventGroupWaitBits(self->event.handle, 0, pdFALSE, pdFALSE, timeout < 0 ? portMAX_DELAY : pdMS_TO_TICKS((uint16_t)timeout)));
}


MP_DEFINE_CONST_FUN_OBJ_KW(event_wait_obj, 1, event_wait);


static mp_obj_t event__del__(mp_obj_t self_in)
{
    mp_obj_threading_event_t *self = MP_OBJ_TO_PTR(self_in);

    xEventGroupSetBits(self->event.handle, 0);
    vEventGroupDelete(self->event.handle);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(event__del__obj, event__del__);

