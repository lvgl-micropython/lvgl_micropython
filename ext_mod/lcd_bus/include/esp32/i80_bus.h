// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "py/obj.h"
#include "soc/soc_caps.h"

#if SOC_LCD_I80_SUPPORTED

#ifndef _I80_BUS_H_
    #define _I80_BUS_H_


    #include "esp_lcd_panel_io.h"
    #include "common/lcd_common_types.h"

    struct _mp_lcd_i80_bus_obj_t {
        struct _mp_lcd_bus_obj_t;

        esp_lcd_panel_io_i80_config_t *panel_io_config;
        esp_lcd_i80_bus_config_t *bus_config;

        /* specific to bus */
        esp_lcd_i80_bus_handle_t bus_handle;
    };

#endif /* _I80_BUS_H_ */

#else
    #include "../other_mcus/i80_bus.h"
#endif /* SOC_LCD_I80_SUPPORTED */

