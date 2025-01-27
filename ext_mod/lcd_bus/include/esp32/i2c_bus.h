// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "py/obj.h"


#ifndef _I2C_BUS_H_
    #define _I2C_BUS_H_

    #include "esp_lcd_panel_io.h"
    #include "driver/i2c.h"

    #include "common/lcd_common_types.h"
    #include "common/sw_rotate_task_common.h"
    #include "common/sw_rotate.h"
    #include "lcd_types.h"


    struct _mp_lcd_i2c_bus_obj_t {
        mp_obj_base_t base;
        lcd_panel_io_t panel_io_handle;

        mp_obj_t callback;

        mp_lcd_framebuf_t *fb1;
        mp_lcd_framebuf_t *fb2;

        uint8_t trans_done: 1;
        uint8_t sw_rotate: 1;
        uint8_t lanes: 5;

        mp_lcd_sw_rotation_t sw_rot;

        esp_lcd_panel_io_i2c_config_t *panel_io_config;
        i2c_config_t *bus_config;

        /* specific to bus */
        esp_lcd_i2c_bus_handle_t bus_handle;

        int host;

    };


#endif /* _I2C_BUS_H_ */