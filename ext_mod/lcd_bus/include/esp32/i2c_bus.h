// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _ESP32_I2C_BUS_H_
    #define _ESP32_I2C_BUS_H_

    //local_includes
    #include "common/lcd_types.h"
    #include "common/lcd_framebuf.h"
    #include "common/sw_rotate.h"

    // esp-idf includes
    #include "esp_lcd_panel_io.h"
    #include "driver/i2c.h"

    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"
    #include "py/objarray.h"

    typedef struct _mp_lcd_i2c_bus_obj_t {
        mp_obj_base_t base;
        lcd_panel_io_t panel_io_handle;

        mp_obj_t callback;

        mp_lcd_framebuf_t *fb1;
        mp_lcd_framebuf_t *fb2;

        uint8_t trans_done: 1;
        uint8_t rgb565_byte_swap: 1;
        uint8_t sw_rotate: 1;

        mp_lcd_sw_rotation_t *sw_rot;

        esp_lcd_panel_io_i2c_config_t *panel_io_config;
        i2c_config_t *bus_config;

        /* specific to bus */
        esp_lcd_i2c_bus_handle_t bus_handle;

        int host;

    } mp_lcd_i2c_bus_obj_t;

    extern const mp_obj_type_t mp_lcd_i2c_bus_type;

    extern void mp_lcd_i2c_bus_deinit_all(void);

#endif /* _ESP32_I2C_BUS_H_ */