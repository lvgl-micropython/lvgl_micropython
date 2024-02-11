#ifndef _LCD_TYPES_H_
    #define _LCD_TYPES_H_

    #ifndef __containerof
        #define __containerof(ptr, type, member) (type *)((char *)ptr - offsetof(type, member))
    #endif

    #define LCD_UNUSED(x) ((void)x)

    // micropython includes
    #include "py/obj.h"
    #include "py/runtime.h"
    #include "py/objarray.h"

    typedef struct _lcd_panel_io_t lcd_panel_io_t;

    #ifdef ESP_IDF_VERSION
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

        struct _lcd_panel_io_t {
            mp_lcd_err_t (*get_lane_count)(lcd_panel_io_t *io, uint8_t *lane_count);
            mp_lcd_err_t (*init)(lcd_panel_io_t *io, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size);
            mp_lcd_err_t (*rx_param)(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size);
            mp_lcd_err_t (*tx_param)(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size);
            mp_lcd_err_t (*tx_color)(lcd_panel_io_t *io, int lcd_cmd, void *color, size_t color_size);
            mp_obj_t (*allocate_framebuffer)(lcd_panel_io_t *io, uint32_t size, uint32_t caps);
            mp_lcd_err_t (*del)(lcd_panel_io_t *io);
            esp_lcd_panel_io_handle_t panel_io;
        };

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

        struct _lcd_panel_io_t {
            mp_lcd_err_t (*get_lane_count)(lcd_panel_io_t *io, uint8_t *lane_count);
            mp_lcd_err_t (*init)(lcd_panel_io_t *io, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size);
            mp_lcd_err_t (*rx_param)(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size);
            mp_lcd_err_t (*tx_param)(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size);
            mp_lcd_err_t (*tx_color)(lcd_panel_io_t *io, int lcd_cmd, void *color, size_t color_size);
            mp_obj_t (*allocate_framebuffer)(lcd_panel_io_t *io, uint32_t size, uint32_t caps);
            mp_lcd_err_t (*del)(lcd_panel_io_t *io);
        };

        bool bus_trans_done_cb(lcd_panel_io_t *panel_io, void *edata, void *user_ctx);

    #endif

    // typedef struct lcd_panel_io_t *lcd_panel_io_handle_t; /*!< Type of LCD panel IO handle */

    mp_lcd_err_t lcd_panel_io_get_lane_count(lcd_panel_io_t *io, uint8_t *lane_count);
    mp_lcd_err_t lcd_panel_io_init(lcd_panel_io_t *io, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size);
    mp_lcd_err_t lcd_panel_io_rx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t lcd_panel_io_tx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t lcd_panel_io_tx_color(lcd_panel_io_t *io, int lcd_cmd, void *color, size_t color_size);
    mp_obj_t lcd_panel_io_allocate_framebuffer(lcd_panel_io_t *io, uint32_t size, uint32_t caps);
    mp_lcd_err_t lcd_panel_io_del(lcd_panel_io_t *io);

    typedef struct _mp_lcd_bus_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        void *buf1;
        void *buf2;

        bool trans_done;
        bool rgb565_byte_swap;

        lcd_panel_io_t panel_io_handle;

    } mp_lcd_bus_obj_t;


    void rgb565_byte_swap(void *buf, uint32_t buf_size_px);
#endif /* _LCD_TYPES_H_ */
