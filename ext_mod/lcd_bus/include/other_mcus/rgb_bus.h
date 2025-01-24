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


    struct _mp_lcd_rgb_bus_obj_t {
        struct _mp_lcd_bus_obj_t;

        void *panel_io_config;
        void *bus_config;

        /* specific to bus */
        void *bus_handle;

    };


#endif /* _RGB_BUS_H_ */