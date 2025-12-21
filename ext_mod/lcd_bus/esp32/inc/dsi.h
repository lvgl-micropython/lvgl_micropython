// Copyright (c) 2024 - 2025 Kevin G. Schlosser

//local_includes
#include "lcd_types.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/objarray.h"

// esp-idf includes
#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED
    // esp-idf includes
    #include "esp_lcd_panel_io.h"
    #include "esp_lcd_panel_interface.h"
    #include "esp_lcd_panel_io.h"
    #include "esp_lcd_mipi_dsi.h"

    #ifndef __DSI_H__
        #define __DSI_H__

        typedef struct _mp_lcd_dsi_bus_obj_t {
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

            esp_lcd_dbi_io_config_t panel_io_config;
            esp_lcd_dsi_bus_config_t bus_config;
            esp_lcd_dsi_bus_handle_t bus_handle;
            esp_lcd_panel_handle_t panel_handle;
            esp_lcd_dpi_panel_config_t panel_config;

        } mp_lcd_dsi_bus_obj_t;

    #endif
#else
    #include "../../unsupported/inc/dsi.h"

#endif

