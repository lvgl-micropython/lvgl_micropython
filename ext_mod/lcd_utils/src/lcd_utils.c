#include "../include/remap.h"
#include "../include/binary_float.h"

#include "py/obj.h"
#include "py/runtime.h"


static const mp_rom_map_elem_t mp_lcd_utils_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),           MP_OBJ_NEW_QSTR(MP_QSTR_lcd_utils)  },
    { MP_ROM_QSTR(MP_QSTR_remap),              MP_ROM_PTR(&mp_lcd_utils_remap_obj) },
    { MP_ROM_QSTR(MP_QSTR_int_float_converter),    MP_ROM_PTR(&mp_lcd_utils_int_float_converter_obj) },
};

static MP_DEFINE_CONST_DICT(mp_lcd_utils_module_globals, mp_lcd_utils_module_globals_table);


const mp_obj_module_t mp_module_lcd_utils = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_lcd_utils_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_lcd_utils, mp_module_lcd_utils);