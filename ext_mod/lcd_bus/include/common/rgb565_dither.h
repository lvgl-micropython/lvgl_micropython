#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>


#ifndef __RGB565_DITHER_H__
    #define __RGB565_DITHER_H__

    #define CALC_THRESHOLD(x, y)  (uint8_t)(((y & 7) << 3) + (x & 7))

    extern uint8_t *red_thresh;
    extern uint8_t *grn_thresh;
    extern uint8_t *blu_thresh;

    bool rgb565_dither_init(void);
    void rgb565_dither_free(void);

    static inline void rgb565_dither_byte_swap_pixel(uint8_t treshold_id, uint16_t *src, uint16_t *dst)
    {
        *dst = (((((*src >> 3) & 0xFC) + grn_thresh[treshold_id]) << 13) |
               (((((*src & 0x1F) << 3) + blu_thresh[treshold_id]) & 0xF8) << 5) |
                ((((*src >> 8) & 0xF8) + red_thresh[treshold_id]) & 0xFF) |
               (((((*src >> 3) & 0xFC) + grn_thresh[treshold_id]) >> 5) & 0x07));
    }

    static inline void rgb565_dither_pixel(uint8_t treshold_id, uint16_t *src, uint16_t *dst)
    {
        *dst = (((((*src >> 8) & 0xF8) + red_thresh[treshold_id]) << 8) |
                ((((*src >> 3) & 0xFC) + grn_thresh[treshold_id]) << 3) |
                ((((*src & 0x1F) << 3) + blu_thresh[treshold_id]) >> 3));
    }

#endif