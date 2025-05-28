// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _ESP32_I80_BUS_H_
    #define _ESP32_I80_BUS_H_

    //local_includes
    #include "lcd_types.h"
    #include "lcd_bus_task.h"

    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"
    #include "py/objarray.h"

    // esp-idf includes
    #include "soc/soc_caps.h"

    #if SOC_LCD_I80_SUPPORTED
        // esp-idf includes
        #include "esp_lcd_panel_io.h"


        typedef struct _mp_lcd_i80_bus_obj_t {
            mp_obj_base_t base;

            mp_obj_t callback;

            mp_obj_array_t *view1;
            mp_obj_array_t *view2;

            uint32_t buffer_flags;

            uint8_t trans_done: 1;

            lcd_task_t task;
            lcd_init_t init;
            lcd_bufs_t bufs;

            lcd_tx_data_t tx_data;
            lcd_tx_cmds_t tx_cmds;

            rotation_data_t r_data;

            lcd_panel_io_t panel_io_handle;

            // ********************** bus specific **********************
            esp_lcd_panel_io_i80_config_t panel_io_config;
            esp_lcd_i80_bus_config_t bus_config;
            esp_lcd_i80_bus_handle_t bus_handle;

        } mp_lcd_i80_bus_obj_t;

        extern const mp_obj_type_t mp_lcd_i80_bus_type;

        extern void mp_lcd_i80_bus_deinit_all(void);

    #else
        #include "../common_include/i80_bus.h"

    #endif /* SOC_LCD_I80_SUPPORTED */
#endif /* _ESP32_I80_BUS_H_ */
