// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _I80_BUS_H_
    #define _I80_BUS_H_

    //local_includes
    #include "common/modlcd_bus.h"
    #include "common/lcd_framebuf.h"
    #include "common/sw_rotate.h"

    #include "mphalport.h"

    // micropython includes
    #include "py/obj.h"
    #include "py/objarray.h"
    #include "py/runtime.h"


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