// Copyright (c) 2024 - 2025 Kevin G. Schlosser
#include "py/obj.h"

#ifndef _LCD_TYPES_H_
    #define _LCD_TYPES_H_

    #include "common/lcd_common_types.h"
    #include "esp_lcd_panel_io.h"

    struct _lcd_panel_io_t {
        mp_lcd_err_t (*init)(mp_obj_t obj, uint8_t cmd_bits, uint8_t param_bits);
        mp_lcd_err_t (*del)(mp_obj_t obj);
        esp_lcd_panel_io_handle_t panel_io;
    };

#endif /* _LCD_TYPES_H_ */
