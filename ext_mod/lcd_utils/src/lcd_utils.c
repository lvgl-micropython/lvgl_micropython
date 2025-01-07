// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "../include/remap.h"
#include "../include/binary_float.h"

#include "py/obj.h"
#include "py/runtime.h"


static mp_obj_t spi_mode_to_polarity_phase(mp_obj_t mode)
{
    uint8_t spi_mode = (uint8_t)mp_obj_get_int_truncated(mode);
    uint8_t polarity = (spi_mode >> 1) & 0x1;
    uint8_t phase = spi_mode & 0x1;
    mp_obj_t tuple[2] = {
        mp_obj_new_int_from_uint(polarity),
        mp_obj_new_int_from_uint(phase),
    };
    return mp_obj_new_tuple(2, tuple);
}

static MP_DEFINE_CONST_FUN_OBJ_1(spi_mode_to_polarity_phase_obj, spi_mode_to_polarity_phase);


static mp_obj_t spi_polarity_phase_to_mode(mp_obj_t polarity, mp_obj_t phase)
{
    uint8_t spi_polarity = (uint8_t)mp_obj_get_int_truncated(polarity);
    uint8_t spi_phase = (uint8_t)mp_obj_get_int_truncated(phase);
    uint8_t spi_mode = ((spi_polarity & 0x1) << 1) | (spi_phase & 0x1);

    return mp_obj_new_int_from_uint(spi_mode);
}

static MP_DEFINE_CONST_FUN_OBJ_2(spi_polarity_phase_to_mode_obj, spi_polarity_phase_to_mode);


static const mp_rom_map_elem_t mp_lcd_utils_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),           MP_OBJ_NEW_QSTR(MP_QSTR_lcd_utils)  },
    { MP_ROM_QSTR(MP_QSTR_remap),              MP_ROM_PTR(&mp_lcd_utils_remap_obj) },
    { MP_ROM_QSTR(MP_QSTR_int_float_converter),    MP_ROM_PTR(&mp_lcd_utils_int_float_converter_obj) },
    { MP_ROM_QSTR(MP_QSTR_spi_mode_to_polarity_phase),    MP_ROM_PTR(&spi_mode_to_polarity_phase_obj) },
    { MP_ROM_QSTR(MP_QSTR_spi_polarity_phase_to_mode),    MP_ROM_PTR(&spi_polarity_phase_to_mode_obj) },

};

static MP_DEFINE_CONST_DICT(mp_lcd_utils_module_globals, mp_lcd_utils_module_globals_table);


const mp_obj_module_t mp_module_lcd_utils = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_lcd_utils_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_lcd_utils, mp_module_lcd_utils);