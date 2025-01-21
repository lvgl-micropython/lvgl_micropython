#ifndef __RGB565_DITHER_H__
    #define __RGB565_DITHER_H__

#include <stdint.h>
#include <stdbool.h>

    #define CALC_THRESHOLD(x, y)  (uint8_t)(((y & 7) << 3) + (x & 7))

    extern uint8_t *red_thresh;
    extern uint8_t *green_thresh;
    extern uint8_t *blue_thresh;

    bool rgb565_dither_init(void);

    static inline void rgb565_dither_pixel(uint8_t treshold_id, uint16_t *pixel)
    {
        *pixel = (((((*pixel >> 8) & 0xF8) + red_thresh[treshold_id]) << 8) |
                  ((((*pixel >> 3) & 0xFC) + green_thresh[treshold_id]) << 3) |
                  ((((*pixel & 0x1F) << 3) + blue_thresh[treshold_id]) >> 3));
    }

#endif
