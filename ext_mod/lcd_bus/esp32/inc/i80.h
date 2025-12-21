// Copyright (c) 2024 - 2025 Kevin G. Schlosser

// local_includes
#include "lcd_types.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/objarray.h"

// esp-idf includes
#include "soc/soc_caps.h"

#if SOC_LCD_I80_SUPPORTED
    // esp-idf includes
    #include "esp_lcd_panel_io.h"

    #ifndef __I80_H__
        #define __I80_H__

        typedef struct _mp_lcd_i80_bus_obj_t {
            mp_obj_base_t base;

            mp_obj_t callback;

            mp_obj_array_t *view1;
            mp_obj_array_t *view2;

            lcd_funcs_t funcs;

            lcd_rotate_buffers_t buffers;
            lcd_rotate_data_t rot_data;
            lcd_rotate_task_t task;
            lcd_rotate_task_init_t *task_init;

            // port & bus specific fields below
            esp_lcd_panel_io_handle_t panel_io;
            TaskHandle_t task_handle;


            esp_lcd_panel_io_i80_config_t panel_io_config;
            esp_lcd_i80_bus_config_t bus_config;
            esp_lcd_i80_bus_handle_t bus_handle;
        } mp_lcd_i80_bus_obj_t;

    #endif

#else
    #include "../../bitbang/inc/i80.h"

#endif /* SOC_LCD_I80_SUPPORTED */
