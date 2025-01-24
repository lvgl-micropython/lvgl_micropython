// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"


#ifndef _LCD_TYPES_H_
    #define _LCD_TYPES_H_

    #include "py/obj.h"


    struct _lcd_panel_io_t {
        mp_lcd_err_t (*init)(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t cmd_bits, uint8_t param_bits);
        mp_lcd_err_t (*rx_param)(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
        mp_lcd_err_t (*del)(mp_obj_t obj);
    };


    bool bus_trans_done_cb(struct _lcd_panel_io_t *panel_io, void *edata, void *user_ctx);

#endif /* _LCD_TYPES_H_ */
