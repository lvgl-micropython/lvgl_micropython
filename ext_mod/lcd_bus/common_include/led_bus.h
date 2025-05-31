// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef __LED_BUS_H__
    #define __LED_BUS_H__

    //local_includes
    #include "modlcd_bus.h"
    #include "mphalport.h"
    #include "lcd_types.h"

    // micropython includes
    #include "py/obj.h"
    #include "py/objarray.h"
    #include "py/runtime.h"

    typedef struct _mp_lcd_led_bus_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        mp_obj_array_t *view1;
        mp_obj_array_t *view2;

        uint32_t buffer_flags;

        uint8_t trans_done: 1;
        uint8_t num_lanes: 5;
        lcd_task_t task;

        lcd_init_t init;
        lcd_bufs_t bufs;

        lcd_tx_data_t tx_data;
        lcd_tx_cmds_t tx_cmds;

        rotation_data_t r_data;

        internal_cb_funcs_t internal_cb_funcs;
    } mp_lcd_led_bus_obj_t;

    extern const mp_obj_type_t mp_lcd_led_bus_type;

#endif