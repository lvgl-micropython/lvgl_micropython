// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include <stdint.h>
#include <stdbool.h>

#ifndef __LCD_DITHER_H__
    #define __LCD_DITHER_H__

    #define CALC_THRESHOLD(x, y)  (uint8_t)(((y & 7) << 3) + (x & 7))

    extern uint8_t *red_thresh;
    extern uint8_t *green_thresh;
    extern uint8_t *blue_thresh;

    bool lcd_dither_init(void);

    static inline void dither_565(uint8_t treshold_id, uint16_t *pixel)
    {
        *pixel = (((((*pixel >> 8) & 0xF8) + red_thresh[treshold_id]) << 8) |
                  ((((*pixel >> 3) & 0xFC) + green_thresh[treshold_id]) << 3) |
                  ((((*pixel & 0x1F) << 3) + blue_thresh[treshold_id]) >> 3));
    }

    static inline void dither_888(uint8_t treshold_id, uint8_t *pixel)
    {
        pixel[0] += red_thresh[treshold_id];
        pixel[1] += green_thresh[treshold_id];
        pixel[2] += blue_thresh[treshold_id];
    }

#endif