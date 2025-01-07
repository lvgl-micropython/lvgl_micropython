// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "../include/remap.h"

#include "py/obj.h"
#include "py/runtime.h"


typedef union {
    float f;
    uint32_t i;
} converter_t;


static mp_obj_t mp_lcd_utils_int_float_converter(size_t n_args, const mp_obj_t *args)
{
    converter_t u;

    if (mp_obj_is_float(args[0])) {
        u.f = mp_obj_get_float_to_f(args[0]);
        return mp_obj_new_int_from_uint(u.i);
    } else {
        u.i = (uint32_t)mp_obj_get_int_truncated(args[0]);
        return mp_obj_new_float_from_f(u.f);
    }
}

MP_DEFINE_CONST_FUN_OBJ_VAR(mp_lcd_utils_int_float_converter_obj, 1, mp_lcd_utils_int_float_converter);
