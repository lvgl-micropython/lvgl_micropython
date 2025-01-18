#ifndef __RGB565_DITHER_H__
    #define __RGB565_DITHER_H__

    #include <string.h>

    #define CALC_THRESHOLD(x, y)  (uint8_t)((y & 7) << 3) + (x & 7)

    static uint8_t *red_thresh;
    static uint8_t *green_thresh;
    static uint8_t *blue_thresh;


    static inline void rgb565_dither_pixel(uint8_t treshold_id, uint16_t *pixel)
    {
        *pixel = (((((*pixel >> 8) & 0xF8) + red_thresh[treshold_id]) << 8) |
                  ((((*pixel >> 3) & 0xFC) + green_thresh[treshold_id]) << 3) |
                  ((((*pixel & 0x1F) << 3) + blue_thresh[treshold_id]) >> 3));
    }


    static inline bool rgb565_dither_init(void)
    {
        if (red_thresh == NULL) {
            red_thresh = (uint8_t *)malloc(64);
            if (red_thresh != NULL) {
                memcpy(red_thresh,
                    (uint8_t []){
                    1, 7, 3, 5, 0, 8, 2, 6,
                    7, 1, 5, 3, 8, 0, 6, 2,
                    3, 5, 0, 8, 2, 6, 1, 7,
                    5, 3, 8, 0, 6, 2, 7, 1,
                    0, 8, 2, 6, 1, 7, 3, 5,
                    8, 0, 6, 2, 7, 1, 5, 3,
                    2, 6, 1, 7, 3, 5, 0, 8,
                    6, 2, 7, 1, 5, 3, 8, 0
                }, 64);
            }
        }

        if (green_thresh == NULL) {
            green_thresh = (uint8_t *)malloc(64);
            if (green_thresh != NULL) {
                memcpy(green_thresh,
                    (uint8_t []){
                    1, 3, 2, 2, 3, 1, 2, 2,
                    2, 2, 0, 4, 2, 2, 4, 0,
                    3, 1, 2, 2, 1, 3, 2, 2,
                    2, 2, 4, 0, 2, 2, 0, 4,
                    1, 3, 2, 2, 3, 1, 2, 2,
                    2, 2, 0, 4, 2, 2, 4, 0,
                    3, 1, 2, 2, 1, 3, 2, 2,
                    2, 2, 4, 0, 2, 2, 0, 4
                }, 64);
            }
        }
        if (blue_thresh == NULL) {
            blue_thresh = (uint8_t *)malloc(64);
            if (blue_thresh != NULL) {
                memcpy(blue_thresh,
                    (uint8_t []){
                    5, 3, 8, 0, 6, 2, 7, 1,
                    3, 5, 0, 8, 2, 6, 1, 7,
                    8, 0, 6, 2, 7, 1, 5, 3,
                    0, 8, 2, 6, 1, 7, 3, 5,
                    6, 2, 7, 1, 5, 3, 8, 0,
                    2, 6, 1, 7, 3, 5, 0, 8,
                    7, 1, 5, 3, 8, 0, 6, 2,
                    1, 7, 3, 5, 0, 8, 2, 6
                }, 64);
            }
        }

        if (red_thresh == NULL || blue_thresh == NULL || green_thresh == NULL) return false;
        else return true;
    }

#endif