// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "py/obj.h"

#include "soc/soc_caps.h"


#if SOC_LCD_RGB_SUPPORTED
#ifndef _RGB_BUS_H_
    #define _RGB_BUS_H_

    #include "esp_lcd_panel_io.h"
    #include "esp_lcd_panel_ops.h"
    #include "esp_lcd_panel_interface.h"
    #include "esp_lcd_panel_rgb.h"

    #include "common/lcd_common_types.h"


    struct _mp_lcd_rgb_bus_obj_t {
        struct _mp_lcd_bus_obj_t;

        esp_lcd_rgb_panel_config_t *panel_io_config;
        void *padding;

        /* specific to bus */
        esp_lcd_panel_handle_t panel_handle;

    };

    extern void mp_lcd_rgb_bus_deinit_all(void);

#endif /* _RGB_BUS_H_ */
#else
    #include "../other_mcus/rgb_bus.h"
#endif /*SOC_LCD_RGB_SUPPORTED*/
