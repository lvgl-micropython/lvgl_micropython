// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "../include/remap.h"

#include "py/obj.h"
#include "py/runtime.h"

static mp_obj_t mp_lcd_utils_remap(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    enum { ARG_value, ARG_old_min, ARG_old_max, ARG_new_min, ARG_new_max};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_value,   MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_old_min, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_old_max, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_new_min, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_new_max, MP_ARG_OBJ | MP_ARG_REQUIRED },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    float value;
    float old_min;
    float old_max;
    float new_min;
    float new_max;
    float ret_val;

    bool is_float = (mp_obj_is_float(args[ARG_value].u_obj) ||
        mp_obj_is_float(args[ARG_old_min].u_obj) || mp_obj_is_float(args[ARG_old_max].u_obj) ||
        mp_obj_is_float(args[ARG_new_min].u_obj) || mp_obj_is_float(args[ARG_new_max].u_obj)
    ) ? true: false;

    if (mp_obj_is_float(args[ARG_value].u_obj)) value = mp_obj_get_float_to_f(args[ARG_value].u_obj);
    else value = mp_obj_get_int(args[ARG_value].u_obj) * 1.0f;

    if (mp_obj_is_float(args[ARG_old_min].u_obj)) old_min = mp_obj_get_float_to_f(args[ARG_old_min].u_obj);
    else old_min = mp_obj_get_int(args[ARG_old_min].u_obj) * 1.0f;

    if (mp_obj_is_float(args[ARG_old_max].u_obj)) old_max = mp_obj_get_float_to_f(args[ARG_old_max].u_obj);
    else old_max = mp_obj_get_int(args[ARG_old_max].u_obj) * 1.0f;

    if (mp_obj_is_float(args[ARG_new_min].u_obj)) new_min = mp_obj_get_float_to_f(args[ARG_new_min].u_obj);
    else new_min = mp_obj_get_int(args[ARG_new_min].u_obj) * 1.0f;

    if (mp_obj_is_float(args[ARG_new_max].u_obj)) new_max = mp_obj_get_float_to_f(args[ARG_new_max].u_obj);
    else new_max = mp_obj_get_int(args[ARG_new_max].u_obj) * 1.0f;

    ret_val = (((value - old_min) * (new_max - new_min)) / (old_max - old_min)) + new_min;

    if (is_float) return mp_obj_new_float_from_f(ret_val);
    else return mp_obj_new_int((int)ret_val);
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_utils_remap_obj, 5, mp_lcd_utils_remap);