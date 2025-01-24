// Copyright (c) 2024 - 2025 Kevin G. Schlosser


// micropython includes
#include "py/obj.h"


#ifndef _COMMON_LCD_TYPES_H_
    #define _COMMON_LCD_TYPES_H_

    #include "lcd_framebuf.h"
    #include "lcd_types.h"

    #define LCD_UNUSED(x) ((void)x)

#if CONFIG_LCD_ENABLE_DEBUG_LOG
    #define LCD_DEBUG_PRINT(...) mp_printf(&mp_plat_print, __VA_ARGS__);
#else
    #define LCD_DEBUG_PRINT(...)
#endif


    extern const mp_obj_type_t mp_lcd_i2c_bus_type;
    extern const mp_obj_type_t mp_lcd_i80_bus_type;
    extern const mp_obj_type_t mp_lcd_rgb_bus_type;
    extern const mp_obj_type_t mp_lcd_spi_bus_type;

    typedef struct _mp_lcd_i2c_bus_obj_t mp_lcd_i2c_bus_obj_t;
    typedef struct _mp_lcd_i80_bus_obj_t mp_lcd_i80_bus_obj_t;
    typedef struct _mp_lcd_rgb_bus_obj_t mp_lcd_rgb_bus_obj_t;
    typedef struct _mp_lcd_spi_bus_obj_t mp_lcd_spi_bus_obj_t;

    typedef struct _lcd_panel_io_t lcd_panel_io_t;


    typedef enum {
        LCD_OK = 0,
        LCD_FAIL = -1,
        LCD_ERR_NO_MEM = 0x101,
        LCD_ERR_INVALID_ARG = 0x102,
        LCD_ERR_INVALID_STATE = 0x103,
        LCD_ERR_INVALID_SIZE = 0x104,
        LCD_ERR_NOT_SUPPORTED = 0x106
    } mp_lcd_err_t;


    typedef struct _mp_lcd_bus_obj_t {
        mp_obj_base_t base;
        lcd_panel_io_t panel_io_handle;

        mp_obj_t callback;

        mp_lcd_framebuf_t *fb1;
        mp_lcd_framebuf_t *fb2;

        uint8_t trans_done: 1;
        uint8_t rgb565_byte_swap: 1;
        uint8_t sw_rotate: 1;
        uint8_t lanes: 5;

        mp_lcd_sw_rotation_t sw_rot;
    } mp_lcd_bus_obj_t;


    void mp_lcd_flush_ready_cb(mp_obj_t cb);

#endif /* _COMMON_LCD_TYPES_H_ */
