// Copyright (c) 2024 - 2025 Kevin G. Schlosser
#include "py/obj.h"

#ifndef _LCD_TYPES_H_
    #define _LCD_TYPES_H_

    #include "esp_lcd_panel_io.h"

    void cb_isr(mp_obj_t cb);
    bool bus_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);

    struct _lcd_panel_io_t {
        mp_lcd_err_t (*init)(mp_obj_t obj, uint8_t cmd_bits, uint8_t param_bits);
        mp_lcd_err_t (*rx_param)(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
        mp_lcd_err_t (*deinit)(mp_obj_t obj);
        esp_lcd_panel_io_handle_t panel_io;
    };

#endif /* _LCD_TYPES_H_ */
