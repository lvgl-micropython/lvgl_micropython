// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _RGB_BUS_H_
    #define _RGB_BUS_H_

    //local_includes
    #include "modlcd_bus.h"

    // micropython includes
    #include "py/obj.h"
    #include "py/runtime.h"


    typedef struct _mp_lcd_rgb_bus_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        void *buf1;
        void *buf2;
        uint32_t buffer_flags;

        bool trans_done;
        bool rgb565_byte_swap;

        lcd_panel_io_t panel_io_handle;

        void *panel_io_config;
        void *bus_config;
        void *bus_handle;

    } mp_lcd_rgb_bus_obj_t;


    extern const mp_obj_type_t mp_lcd_rgb_bus_type;
#endif /* _RGB_BUS_H_ */