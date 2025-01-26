// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "py/obj.h"


#ifndef _I2C_BUS_H_
    #define _I2C_BUS_H_

    #include "esp_lcd_panel_io.h"
    #include "driver/i2c.h"

    #include "common/lcd_common_types.h"


    struct _mp_lcd_i2c_bus_obj_t {
        struct _mp_lcd_bus_obj_t;

        esp_lcd_panel_io_i2c_config_t *panel_io_config;
        i2c_config_t *bus_config;

        /* specific to bus */
        esp_lcd_i2c_bus_handle_t bus_handle;

        int host;

    };


#endif /* _I2C_BUS_H_ */