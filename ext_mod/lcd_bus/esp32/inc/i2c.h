// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef __I2C_H__
    #define __I2C_H__

    //local_includes
    #include "lcd_types.h"

    // esp-idf includes
    #include "esp_lcd_panel_io.h"
    #include "driver/i2c.h"

    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"
    #include "py/objarray.h"

    typedef struct _mp_lcd_i2c_bus_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        mp_obj_array_t *view1;
        mp_obj_array_t *view2;

        lcd_funcs_t funcs;

        lcd_rotate_buffers_t buffers;
        lcd_rotate_data_t rot_data;
        lcd_rotate_task_t task;
        lcd_rotate_task_init_t *task_init;

        // port & bus specific fields below
        esp_lcd_panel_io_handle_t panel_io;
        TaskHandle_t task_handle;


        esp_lcd_panel_io_i2c_config_t panel_io_config;
        i2c_port_t port;
        esp_lcd_i2c_bus_handle_t bus_handle;

    } mp_lcd_i2c_bus_obj_t;

#endif
