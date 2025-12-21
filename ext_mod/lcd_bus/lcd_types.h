// Copyright (c) 2024 - 2025 Kevin G. Schlosser

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objarray.h"

#include "lcd_os.h"


#ifndef __LCD_TYPES_H__
    #define __LCD_TYPES_H__

    #define LCD_UNUSED(x) ((void)x)

    extern const mp_obj_type_t mp_lcd_dsi_bus_type;
    extern const mp_obj_type_t mp_lcd_i2c_bus_type;
    extern const mp_obj_type_t mp_lcd_i80_bus_type;
    extern const mp_obj_type_t mp_lcd_led_bus_type;
    extern const mp_obj_type_t mp_lcd_rgb_bus_type;
    extern const mp_obj_type_t mp_lcd_sdl_bus_type;
    extern const mp_obj_type_t mp_lcd_spi_bus_type;

    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        #define LCD_DEBUG_PRINT(...) mp_printf(&mp_plat_print, __VA_ARGS__);
    #else
        #define LCD_DEBUG_PRINT(...)
    #endif

    typedef struct _lcd_funcs_t {
        mp_lcd_err_t (*get_lane_count)(mp_obj_t obj, uint8_t *lane_count);
        mp_lcd_err_t (*init)(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
        mp_lcd_err_t (*rx_param)(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
        mp_lcd_err_t (*tx_param)(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
        mp_lcd_err_t (*tx_color)(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update);
        mp_obj_t (*allocate_framebuffer)(mp_obj_t obj, uint32_t size, uint32_t caps);
        mp_obj_t (*free_framebuffer)(mp_obj_t obj, mp_obj_t buf);
        mp_lcd_err_t (*del)(mp_obj_t obj);
    } lcd_funcs_t;

    // typedef struct lcd_panel_io_t *lcd_panel_io_handle_t; /*!< Type of LCD panel IO handle */
    mp_lcd_err_t lcd_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
    mp_lcd_err_t lcd_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
    mp_lcd_err_t lcd_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t lcd_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t lcd_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update);
    mp_obj_t lcd_allocate_framebuffer(mp_obj_t obj, uint32_t size, uint32_t caps);
    mp_obj_t lcd_free_framebuffer(mp_obj_t obj, mp_obj_t buf);
    mp_lcd_err_t lcd_del(mp_obj_t obj);



    typedef struct _mp_lcd_bus_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        mp_obj_array_t *view1;
        mp_obj_array_t *view2;

        lcd_funcs_t funcs;

        lcd_rotate_buffers_t buffers;
        lcd_rotate_data_t rot_data;
        lcd_rotate_task_t task;
        lcd_rotate_task_init_t *task_init;
        lcd_tx_cmds_t tx_cmds;

    } mp_lcd_bus_obj_t;

#endif /* _LCD_TYPES_H_ */
