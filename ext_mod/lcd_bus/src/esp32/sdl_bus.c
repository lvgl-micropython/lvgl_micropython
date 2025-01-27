// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"

#include "common/modlcd_bus.h"
#include "common/lcd_common_types.h"
#include "lcd_types.h"
#include "sdl_bus.h"


static mp_obj_t sdl_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    LCD_UNUSED(type);
    LCD_UNUSED(n_args);
    LCD_UNUSED(n_kw);
    LCD_UNUSED(all_args);

    return mp_const_none;
}


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_sdl_bus_type,
    MP_QSTR_SDLBus,
    MP_TYPE_FLAG_NONE,
    make_new, sdl_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
);
