// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef __SPI3WIRE_H__
    #define __SPI3WIRE_H__

    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"

    #include "esp_lcd_panel_io_additions.h"
    #include "esp_lcd_panel_io.h"

#if CONFIG_LCD_ENABLE_DEBUG_LOG
    #define SPI3WIRE_DEBUG_PRINT(...) mp_printf(&mp_plat_print, __VA_ARGS__);
#else
    #define SPI3WIRE_DEBUG_PRINT(...)
#endif

    typedef struct {
        mp_obj_base_t base;

        esp_lcd_panel_io_handle_t panel_io_handle;
        esp_lcd_panel_io_3wire_spi_config_t *io_config;
    } mp_spi3wire_obj_t;

    extern const mp_obj_type_t mp_spi3wire_type;

    extern void mp_spi3wire_deinit_all(void);

#endif