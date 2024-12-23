#include <stdint.h>

#ifndef __ROTATE_H__
    #define __ROTATE_H__

    #ifndef MAX
        #define MAX(a, b) (((a) > (b)) ? (a) : (b))
    #endif

    #ifndef MIN
        #define MIN(a, b) (((a) < (b)) ? (a) : (b))
    #endif


    #define ROTATION_0    (0)
    #define ROTATION_90   (1)
    #define ROTATION_180  (2)
    #define ROTATION_270  (3)

    typedef struct _rotation_data_t {
        int x_start;
        int y_start;
        int x_end;
        int y_end;

        uint16_t width;
        uint16_t height;

        uint8_t rotation: 2;
        uint8_t bytes_per_pixel: 2;
        uint8_t last_update: 1;

    } rotation_data_t;


    uint32_t rotate(void *src, void *dst, rotation_data_t *data);

#endif