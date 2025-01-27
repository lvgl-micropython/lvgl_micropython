// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"


#ifndef _LCD_TYPES_H_
    #define _LCD_TYPES_H_

    #include "common/lcd_common_types.h"

    struct _lcd_panel_io_t {
        mp_lcd_err_t (*init)(mp_obj_t obj, uint8_t cmd_bits, uint8_t param_bits);
        mp_lcd_err_t (*del)(mp_obj_t obj);
    };

    extern const mp_obj_fun_builtin_fixed_t mp_lcd_bus_get_lane_count_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_lcd_bus_register_callback_obj;
    extern const mp_obj_fun_builtin_var_t mp_lcd_bus_tx_color_obj;
    extern const mp_obj_fun_builtin_var_t mp_lcd_bus_rx_param_obj;
    extern const mp_obj_fun_builtin_var_t mp_lcd_bus_tx_param_obj;
    extern const mp_obj_fun_builtin_var_t mp_lcd_bus_init_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_lcd_bus_deinit_obj;

#endif /* _LCD_TYPES_H_ */
