// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _RGB_BUS_H_
    #define _RGB_BUS_H_

    //local_includes
    #include "common/modlcd_bus.h"
    #include "common/lcd_framebuf.h"
    #include "common/sw_rotate.h"

    // micropython includes
    #include "py/obj.h"
    #include "py/runtime.h"


    typedef struct _mp_lcd_rgb_bus_obj_t {
        mp_obj_base_t base;
        lcd_panel_io_t panel_io_handle;

        mp_obj_t callback;

        mp_lcd_framebuf_t *fb1;
        mp_lcd_framebuf_t *fb2;

        uint8_t trans_done: 1;
        uint8_t rgb565_byte_swap: 1;
        uint8_t sw_rotate: 1;

        mp_lcd_sw_rotation_t *sw_rot;

        void *panel_io_config;
        void *bus_config;

        /* specific to bus */
        void *bus_handle;

    } mp_lcd_rgb_bus_obj_t;


    extern const mp_obj_type_t mp_lcd_rgb_bus_type;
#endif /* _RGB_BUS_H_ */