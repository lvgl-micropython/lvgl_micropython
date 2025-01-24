// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _I2C_BUS_H_
    #define _I2C_BUS_H_

    //local_includes
    #include "common/modlcd_bus.h"
    #include "common/lcd_framebuf.h"
    #include "common/sw_rotate.h"

    // micropython includes
    #include "py/obj.h"
    #include "py/runtime.h"


    struct _mp_lcd_i2c_bus_obj_t {
        struct _mp_lcd_bus_obj_t;
    };

#endif /* _I2C_BUS_H_ */