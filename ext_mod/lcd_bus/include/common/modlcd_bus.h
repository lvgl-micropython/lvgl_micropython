// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"

#ifndef _MODLCD_BUS_H_
    #define _MODLCD_BUS_H_

    extern const mp_obj_fun_builtin_var_t mp_lcd_bus_init_obj;
    extern const mp_obj_fun_builtin_var_t mp_lcd_bus_get_lane_count_obj;
    extern const mp_obj_fun_builtin_var_t mp_lcd_bus_tx_param_obj;
    extern const mp_obj_fun_builtin_var_t mp_lcd_bus_tx_color_obj;
    extern const mp_obj_fun_builtin_fixed_t mp_lcd_bus_deinit_obj;
    extern const mp_obj_fun_builtin_var_t mp_lcd_bus_rx_param_obj;
    extern const mp_obj_fun_builtin_var_t mp_lcd_bus_register_callback_obj;
    extern const mp_obj_fun_builtin_var_t mp_lcd_bus_free_framebuffer_obj;
    extern const mp_obj_fun_builtin_var_t mp_lcd_bus_allocate_framebuffer_obj;

    extern const mp_obj_dict_t mp_lcd_bus_locals_dict;

    void mp_lcd_bus_shutdown(void);

#endif /* _MODLCD_BUS_H_ */


