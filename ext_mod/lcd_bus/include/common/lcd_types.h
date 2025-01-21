// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _LCD_TYPES_H_
    #define _LCD_TYPES_H_

    #define LCD_UNUSED(x) ((void)x)

    // micropython includes
    #include "py/obj.h"
    #include "py/runtime.h"
    #include "py/objarray.h"

    typedef struct _lcd_panel_io_t lcd_panel_io_t;

    #ifdef ESP_IDF_VERSION
        #include "sdkconfig.h"

        // esp-idf includes
        #include "esp_lcd_panel_io.h"

        #define LCD_OK                 0
        #define LCD_FAIL               -1
        #define LCD_ERR_NO_MEM         0x101
        #define LCD_ERR_INVALID_ARG    0x102
        #define LCD_ERR_INVALID_STATE  0x103
        #define LCD_ERR_INVALID_SIZE   0x104
        #define LCD_ERR_NOT_SUPPORTED  0x106

        typedef int mp_lcd_err_t;

        void cb_isr(mp_obj_t cb);
        bool bus_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);

    #else

        typedef enum {
            LCD_OK = 0,
            LCD_FAIL = -1,
            LCD_ERR_NO_MEM = 0x101,
            LCD_ERR_INVALID_ARG = 0x102,
            LCD_ERR_INVALID_STATE = 0x103,
            LCD_ERR_INVALID_SIZE = 0x104,
            LCD_ERR_NOT_SUPPORTED = 0x106
        } mp_lcd_err_t;

        bool bus_trans_done_cb(lcd_panel_io_t *panel_io, void *edata, void *user_ctx);
    #endif

    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        #define LCD_DEBUG_PRINT(...) mp_printf(&mp_plat_print, __VA_ARGS__);
    #else
        #define LCD_DEBUG_PRINT(...)
    #endif

    struct _lcd_panel_io_t {
        mp_lcd_err_t (*get_lane_count)(mp_obj_t obj, uint8_t *lane_count);
        mp_lcd_err_t (*init)(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
        mp_lcd_err_t (*rx_param)(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
        mp_lcd_err_t (*tx_param)(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
        mp_lcd_err_t (*tx_color)(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update);
        mp_lcd_err_t (*del)(mp_obj_t obj);

    #ifdef ESP_IDF_VERSION
        esp_lcd_panel_io_handle_t panel_io;
    #endif
    };

    // typedef struct lcd_panel_io_t *lcd_panel_io_handle_t; /*!< Type of LCD panel IO handle */

    mp_lcd_err_t lcd_panel_io_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
    mp_lcd_err_t lcd_panel_io_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
    mp_lcd_err_t lcd_panel_io_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t lcd_panel_io_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t lcd_panel_io_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update);

    mp_lcd_err_t lcd_panel_io_del(mp_obj_t obj);

    typedef struct _mp_lcd_bus_obj_t {
        mp_obj_base_t base;
        lcd_panel_io_t panel_io_handle;

        mp_obj_t callback;

        mp_lcd_framebuf_t *fb1;
        mp_lcd_framebuf_t *fb2;

        uint8_t trans_done: 1;
        uint8_t rgb565_byte_swap: 1;
        uint8_t sw_rotate: 1;

        mp_lcd_sw_rotation_t *sw_rot;
    } mp_lcd_bus_obj_t;

    void rgb565_byte_swap(void *buf, uint32_t buf_size_px);
    void mp_lcd_flush_ready_cb(mp_obj_t cb);

#endif /* _LCD_TYPES_H_ */
