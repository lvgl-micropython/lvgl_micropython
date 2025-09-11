// Copyright (c) 2024 - 2025 Kevin G. Schlosser

//local_includes
#include "lcd_types.h"
#include "modlcd_bus.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/objarray.h"

// esp-idf includes
#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED
    #ifndef __DSI_BUS_H___
        #define __DSI_BUS_H___

        // esp-idf includes
        #include "esp_lcd_panel_io.h"
        #include "esp_lcd_panel_interface.h"
        #include "esp_lcd_panel_io.h"
        #include "esp_lcd_mipi_dsi.h"


        typedef struct _mp_lcd_dsi_bus_obj_t {
            mp_obj_base_t base;

            mp_obj_t callback;

            void *buf1;
            void *buf2;
            uint32_t buffer_flags;

            bool trans_done;
            bool rgb565_byte_swap;

            lcd_panel_io_t panel_io_handle;

            esp_lcd_dbi_io_config_t panel_io_config;
            esp_lcd_dsi_bus_config_t bus_config;
            esp_lcd_dsi_bus_handle_t bus_handle;
            esp_lcd_panel_handle_t panel_handle;
            esp_lcd_dpi_panel_config_t panel_config;

            uint32_t buffer_size;
            mp_obj_array_t *view1;
            mp_obj_array_t *view2;

            void *transmitting_buf;

        } mp_lcd_dsi_bus_obj_t;

        extern const mp_obj_type_t mp_lcd_dsi_bus_type;

    #endif /* __DSI_BUS_H___ */
#else
    #include "../../unavailable/include/dsi_bus.h"
#endif /* SOC_MIPI_DSI_SUPPORTED */

