// Copyright (c) 2024 - 2025 Kevin G. Schlosser

//local_includes
#include "modlcd_bus.h"

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
    
    
#ifndef __LED_BUS_H__
    #define __LED_BUS_H__

    typedef struct _mp_lcd_led_bus_obj_t {
        mp_obj_base_t base;
    } mp_lcd_led_bus_obj_t;

    extern const mp_obj_type_t mp_lcd_led_bus_type;
    
#endif /* __LED_BUS_H__ */
