// Copyright (c) 2024 - 2025 Kevin G. Schlosser


/* includes */
// local includes
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "spi_bus.h"


#include "py/obj.h"
#include "py/runtime.h"

static mp_obj_t mp_lcd_spi_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    LCD_UNUSED(type);
    LCD_UNUSED(n_args);
    LCD_UNUSED(n_kw);
    LCD_UNUSED(all_args);

    mp_raise_msg(&mp_type_NotImplementedError, MP_ERROR_TEXT("SPI display bus is not supported"));
    return mp_const_none;
}


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_spi_bus_type,
    MP_QSTR_SPI_Bus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_spi_bus_make_new
);
