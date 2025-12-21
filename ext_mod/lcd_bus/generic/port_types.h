
#ifndef __PORT_TYPES_H__
    #define __PORT_TYPES_H__

    typedef enum {
        LCD_OK = 0,
        LCD_FAIL = -1,
        LCD_ERR_NO_MEM = 0x101,
        LCD_ERR_INVALID_ARG = 0x102,
        LCD_ERR_INVALID_STATE = 0x103,
        LCD_ERR_INVALID_SIZE = 0x104,
        LCD_ERR_NOT_SUPPORTED = 0x106
    } mp_lcd_err_t;