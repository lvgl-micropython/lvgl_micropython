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
    #include "common/sw_rotate_task_common.h"
    #include "common/sw_rotate.h"
    #include "lcd_types.h"


    struct _mp_lcd_rgb_bus_obj_t {
        mp_obj_base_t base;
        lcd_panel_io_t panel_io_handle;

        mp_obj_t callback;

        mp_lcd_framebuf_t *fb1;
        mp_lcd_framebuf_t *fb2;

        uint8_t trans_done: 1;
        uint8_t sw_rotate: 1;
        uint8_t lanes: 5;

        mp_lcd_sw_rotation_t sw_rot;

        esp_lcd_rgb_panel_config_t *panel_io_config;
        void *padding;

        /* specific to bus */
        esp_lcd_panel_handle_t panel_handle;

    };

#endif /* _RGB_BUS_H_ */
#else
    #include "../other_mcus/rgb_bus.h"
#endif /*SOC_LCD_RGB_SUPPORTED*/
