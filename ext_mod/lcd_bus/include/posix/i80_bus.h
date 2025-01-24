// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"


#ifndef _I80_BUS_H_
    #define _I80_BUS_H_

    #include "common/lcd_common_types.h"
    #include "common/modlcd_bus.h"
    #include "lcd_types.h"

    struct _mp_lcd_i80_bus_obj_t {
        struct _mp_lcd_bus_obj_t;

        void *panel_io_config;
        void *bus_config;

        /* specific to bus */
        void *bus_handle;

        uint32_t buffer_size;
        uint8_t bpp;

        void (*write_color)(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size);
    };

#endif /* _I80_BUS_H_ */