#include "../include/remap.h"

#include "py/obj.h"
#include "py/runtime.h"

static mp_obj_t mp_lcd_utils_float_to_binary(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_value,   MP_ARG_OBJ | MP_ARG_REQUIRED }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    union { float f; uint32_t i; } __u;
    __u.f = mp_obj_get_float_to_f(args[ARG_value].u_obj;

    return mp_obj_new_int((int)__u.i);
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_utils_float_to_binary_obj, 1, mp_lcd_utils_float_to_binary);

static mp_obj_t mp_lcd_utils_binary_to_float(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_value,   MP_ARG_OBJ | MP_ARG_REQUIRED }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    union { float f; uint32_t i; } __u;
    __u.i = (uint32_t)mp_obj_get_int(args[ARG_value].u_obj)

    return mp_obj_new_float_from_f(__u.f)
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_utils_binary_to_float_obj, 1, mp_lcd_utils_binary_to_float);

