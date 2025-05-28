#include <stdint.h>
#include <inttypes.h>

#ifndef __ROTATION_H__
    #define __ROTATION_H__

    typedef struct {
        int32_t x_start;
        int32_t y_start;
        int32_t x_end;
        int32_t y_end;
        uint16_t dst_width;
        uint16_t dst_height;
        size_t color_size;
        uint8_t bytes_per_pixel: 2;
        uint8_t rotation: 2;
        uint8_t dither: 1;
        uint8_t swap: 1,
        uint8_t last_update: 1,
        uint8_t sw_rotation: 1
    } rotation_data_t;

    void copy_pixels(void *dst, void *src, rotation_data_t* r_data);

#endif