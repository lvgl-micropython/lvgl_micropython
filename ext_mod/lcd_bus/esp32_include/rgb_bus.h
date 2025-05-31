// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED
    #ifndef _ESP32_RGB_BUS_H_
        #define _ESP32_RGB_BUS_H_

        //local_includes
        #include "lcd_types.h"
        #include "lcd_bus_task.h"

        #include "esp_lcd_panel_io.h"
        #include "esp_lcd_panel_ops.h"
        #include "esp_lcd_panel_interface.h"
        #include "esp_lcd_panel_rgb.h"

        // micropython includes
        #include "mphalport.h"
        #include "py/obj.h"
        #include "py/objarray.h"

        typedef struct _mp_lcd_rgb_bus_obj_t {
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

            // ********************** bus specific **********************
            esp_lcd_rgb_panel_config_t panel_io_config;
            esp_lcd_rgb_timing_t bus_config;

            esp_lcd_panel_handle_t panel_handle;
            uint32_t buffer_size;

        } mp_lcd_rgb_bus_obj_t;

        extern const mp_obj_type_t mp_lcd_rgb_bus_type;

    #endif /* _ESP32_RGB_BUS_H_ */
#else
    #include "../common_include/rgb_bus.h"

#endif /*SOC_LCD_RGB_SUPPORTED*/