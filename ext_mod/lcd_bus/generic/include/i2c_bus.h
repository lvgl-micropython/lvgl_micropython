// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _I2C_BUS_H_
    #define _I2C_BUS_H_

    //local_includes
    #include "lcd_types.h"
    #include "../../../micropy_updates/common/mp_i2c_common.h"


    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"
    #include "py/objarray.h"

    typedef struct _mp_lcd_i2c_bus_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        mp_obj_array_t *view1;
        mp_obj_array_t *view2;

        uint32_t buffer_flags;

        bool trans_done;
        bool rgb565_byte_swap;

        lcd_panel_io_t panel_io_handle;
        uint8_t disable_control_phase: 1;
        uint8_t control_phase_enabled: 1;
        uint8_t lcd_param_bits;
        uint8_t lcd_cmd_bits;
        uint8_t control_phase_bytes;
        uint8_t dev_addr;
        uint32_t scl_speed_hz;
        mp_machine_hw_i2c_bus_obj_t *i2c_bus;
        mp_machine_hw_i2c_device_obj_t i2c_device;
        uint32_t control_phase_cmd;  // control byte when transferring command
        uint32_t control_phase_data; // control byte when transferring data

    } mp_lcd_i2c_bus_obj_t;

    extern const mp_obj_type_t mp_lcd_i2c_bus_type;

    extern void mp_lcd_i2c_bus_deinit_all(void);

#endif
