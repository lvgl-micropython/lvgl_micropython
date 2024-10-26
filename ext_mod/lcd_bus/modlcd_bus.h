/* Copyright (C) 2024  Kevin G Schlosser
 * Code that is written by the above named is done under the GPL license
 * and that license is able to be viewed in the LICENSE file in the root
 * of this project.
 */

#ifndef _MODLCD_BUS_H_
    #define _MODLCD_BUS_H_

    #include "lcd_types.h"

    // micropython includes
    #include "py/obj.h"
    #include "py/runtime.h"
    #include "py/objarray.h"

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

#endif /* _MODLCD_BUS_H_ */


