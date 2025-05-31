#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifndef __DITHER_H__
    #define __DITHER_H__

    #define CALC_THRESHOLD(x, y)  (uint8_t)(((y & 7) << 3) + (x & 7))

    extern uint8_t *red_thresh;
    extern uint8_t *green_thresh;
    extern uint8_t *blue_thresh;

    bool dither_init(void);
    void dither_deinit(void);

    __attribute__((always_inline))
    static inline void rgb565_dither_swap_pixel(uint8_t treshold_id, uint16_t *to, uint16_t *from)
    {
        *to = (
            ((((*from >> 8) & 0xF8) + red_thresh[treshold_id]) << 8) |
            ((((*from >> 3) & 0xFC) + green_thresh[treshold_id]) << 3) |
            ((((*from & 0x1F) << 3) + blue_thresh[treshold_id]) >> 3)
        );
        *to = (*to >> 8) | (*to << 8);
    }

    __attribute__((always_inline))
    static inline void rgb565_dither_pixel(uint8_t treshold_id, uint16_t *to, uint16_t *from)
    {
        *to = (
            ((((*from >> 8) & 0xF8) + red_thresh[treshold_id]) << 8) |
            ((((*from >> 3) & 0xFC) + green_thresh[treshold_id]) << 3) |
            ((((*from & 0x1F) << 3) + blue_thresh[treshold_id]) >> 3)
        );
    }

    __attribute__((always_inline))
    static inline void rgb888_dither_pixel(uint8_t treshold_id, uint8_t *to, uint8_t *from)
    {
        *to++ = (*from++) + red_thresh[treshold_id];
        *to++ = (*from++) + green_thresh[treshold_id];
        *to++ = (*from++) + blue_thresh[treshold_id];
    }

#endif
