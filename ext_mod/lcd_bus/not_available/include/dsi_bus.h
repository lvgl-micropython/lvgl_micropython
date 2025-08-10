// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _DSI_BUS_H_
    #define _DSI_BUS_H_

    //local_includes
    #include "modlcd_bus.h"

    // micropython includes
    #include "py/obj.h"
    #include "py/runtime.h"


    typedef struct _mp_lcd_dsi_bus_obj_t {
        mp_obj_base_t base;
    } mp_lcd_dsi_bus_obj_t;


    extern const mp_obj_type_t mp_lcd_dsi_bus_type;
#endif /* _DSI_BUS_H_ */