#include "rotate.h"

// stdlib includes
#include <string.h>
#include <stdint.h>

static void rotate0(uint8_t *src, uint8_t *dst, rotation_data_t *data);
static void rotate_8bpp(uint8_t *src, uint8_t *dst, rotation_data_t *data);
static void rotate_16bpp(uint16_t *src, uint16_t *dst, rotation_data_t *data);
static void rotate_24bpp(uint8_t *src, uint8_t *dst, rotation_data_t *data);
static void rotate_32bpp(uint32_t *src, uint32_t *dst, rotation_data_t *data);


__attribute__((always_inline))
static inline void copy_8bpp(uint8_t *from, uint8_t *to)
{
    *to++ = *from++;
}


__attribute__((always_inline))
static inline void copy_16bpp(uint16_t *from, uint16_t *to)
{
    *to++ = *from++;
}


__attribute__((always_inline))
static inline void copy_24bpp(uint8_t *from, uint8_t *to)
{
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
}


__attribute__((always_inline))
static inline void copy_32bpp(uint32_t *from, uint32_t *to)
{
    *to++ = *from++;
}


uint32_t rotate(void *src, void *dst, rotation_data_t *data)
{
    rotation_data_t rot_data;

    if (data->rotation != ROTATION_0) data->y_end += 1;

    if (data->rotation == ROTATION_90 || data->rotation == ROTATION_270) {
        rot_data.x_start = MIN(data->x_start, data->height);
        rot_data.x_end = MIN(data->x_end, data->height);
        rot_data.y_start = MIN(data->y_start, data->width);
        rot_data.y_end = MIN(data->y_end, data->width);
    } else {
        rot_data.x_start = MIN(data->x_start, data->width);
        rot_data.x_end = MIN(data->x_end, data->width);
        rot_data.y_start = MIN(data->y_start, data->height);
        rot_data.y_end = MIN(data->y_end, data->height);
    }

    rot_data.height = data->height;
    rot_data.width = data->width;
    rot_data.rotation = data->rotation;
    rot_data.bytes_per_pixel = data->bytes_per_pixel;

    if (data->rotation == ROTATION_0) {
        rotate0(src, dst, &rot_data);
    } else {
        if (data->bytes_per_pixel == 1) rotate_8bpp(src, dst, &rot_data);
        else if (data->bytes_per_pixel == 2) rotate_16bpp(src, dst, &rot_data);
        else if (data->bytes_per_pixel == 3) rotate_24bpp(src, dst, &rot_data);
        else if (data->bytes_per_pixel == 4) rotate_32bpp(src, dst, &rot_data);
    }

    return (rot_data.x_end - rot_data.x_start + 1) * (rot_data.y_end - rot_data.y_start + 1) * rot_data.bytes_per_pixel;
}


void rotate0(uint8_t *src, uint8_t *dst, rotation_data_t *data)
{
    uint32_t x_start = data->x_start;
    uint32_t y_start = data->y_start;
    uint32_t x_end = data->x_end;
    uint32_t y_end = data->y_end;

    uint32_t dst_width = data->width;

    uint8_t bytes_per_pixel = data->bytes_per_pixel;

    dst += ((y_start * dst_width + x_start) * bytes_per_pixel);

    if (x_start == 0 && x_end == (dst_width - 1)) {
        memcpy(dst, src, dst_width * (y_end - y_start + 1) * bytes_per_pixel);
    } else {
        uint32_t src_bytes_per_line = (x_end - x_start + 1) * bytes_per_pixel;
        uint32_t dst_bytes_per_line = dst_width * bytes_per_pixel;

        for (uint32_t y=y_start;y<y_end;y++) {
            memcpy(dst, src, src_bytes_per_line);
            dst += dst_bytes_per_line;
            src += src_bytes_per_line;
        }
    }
}

void rotate_8bpp(uint8_t *src, uint8_t *dst, rotation_data_t *data)
{
    uint32_t i;

    uint32_t x_start = data->x_start;
    uint32_t y_start = data->y_start;
    uint32_t x_end = data->x_end;
    uint32_t y_end = data->y_end;

    uint32_t dst_width = data->width;
    uint32_t dst_height = data->height;

    uint32_t src_bytes_per_line = x_end - x_start + 1;
    uint32_t offset = y_start * src_bytes_per_line + x_start;

    switch (data->rotation) {
        case ROTATION_90:
            dst_height--;

            for (uint32_t y=y_start;y<y_end;y++) {
                i = y * src_bytes_per_line - offset;

                for (uint32_t x=x_start;x<x_end;x++) {
                    copy_8bpp(src + (i + x),
                            dst + (((dst_height - x) * dst_width) + y));
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case ROTATION_180:
            dst_height--;
            offset = dst_width - 1 - x_start;

            for (uint32_t y=y_start;y<y_end;y++) {
                i = ((dst_height - y) * dst_width) + offset;

                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_8bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case ROTATION_270:
            uint32_t dst_width_minus_one = dst_width - 1;

            for (uint32_t y=y_start;y<y_end;y++) {
                i = y * src_bytes_per_line - offset;
                dst_height = dst_width_minus_one - y;

                for (uint32_t x=x_start;x<x_end;x++) {
                    copy_8bpp(src + (i + x), dst + (x * dst_width + dst_height));
                }
            }
            break;

        default:
            break;
    }

}


void rotate_16bpp(uint16_t *src, uint16_t *dst, rotation_data_t *data)
{
    uint32_t i;

    uint32_t x_start = data->x_start;
    uint32_t y_start = data->y_start;
    uint32_t x_end = data->x_end;
    uint32_t y_end = data->y_end;

    uint32_t dst_width = data->width;
    uint32_t dst_height = data->height;

    uint32_t src_bytes_per_line = x_end - x_start + 1;
    uint32_t offset = y_start * src_bytes_per_line + x_start;

    switch (data->rotation) {
        case ROTATION_90:
            dst_height--;

            for (uint32_t y=y_start;y<y_end;y++) {
                i = y * src_bytes_per_line - offset;

                for (uint32_t x=x_start;x<x_end;x++) {
                    copy_16bpp(src + (i + x),
                            dst + (((dst_height - x) * dst_width) + y));
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case ROTATION_180:
            dst_height--;
            offset = dst_width - 1 - x_start;

            for (uint32_t y=y_start;y<y_end;y++) {
                i = ((dst_height - y) * dst_width) + offset;

                for (uint32_t x=x_start;x<x_end;x++) {
                    copy_16bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case ROTATION_270:
            uint32_t dst_width_minus_one = dst_width - 1;

            for (uint32_t y=y_start;y<y_end;y++) {
                i = y * src_bytes_per_line - offset;
                dst_height = dst_width_minus_one - y;

                for (uint32_t x=x_start;x<x_end;x++) {
                    copy_16bpp(src + (i + x), dst + (x * dst_width + dst_height));
                }
            }
            break;

        default:
            break;
    }
}


void rotate_24bpp(uint8_t *src, uint8_t *dst, rotation_data_t *data)
{
    uint32_t i;
    uint32_t j;

    uint32_t x_start = data->x_start;
    uint32_t y_start = data->y_start;
    uint32_t x_end = data->x_end;
    uint32_t y_end = data->y_end;

    uint32_t dst_width = data->width;
    uint32_t dst_height = data->height;

    uint32_t src_bytes_per_line = (x_end - x_start + 1) * 3;
    uint32_t offset = y_start * src_bytes_per_line + x_start * 3;

    switch (data->rotation) {

        case ROTATION_90:
            dst_height--;

            for (uint32_t y=y_start;y<y_end;y++) {
                i = y * src_bytes_per_line - offset;

                for (uint32_t x=x_start;x<x_end;x++) {
                    copy_24bpp(src + (i + (x * 3)),
                            dst + ((((dst_height - x) * dst_width) + y) * 3));
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case ROTATION_180:
            dst_height--;
            offset = dst_width - 1 - x_start;

            for (uint32_t y=y_start;y<y_end;y++) {
                i = (((dst_height - y) * dst_width) + offset) * 3;

                for (size_t x=x_start;x<x_end;x++) {
                    copy_24bpp(src, dst + i);
                    src += 3;
                    i -= 3;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case ROTATION_270:
            for (uint32_t y=y_start;y<y_end;y++) {
                i = y * src_bytes_per_line - offset;
                j = dst_width - 1 - y;

                for (uint32_t x=x_start;x<x_end;x++) {
                    copy_24bpp(src + (i + (x * 3)),
                            dst + (((x * dst_width) + j) * 3));
                }
            }
            break;

        default:
            break;
    }
}


void rotate_32bpp(uint32_t *src, uint32_t *dst, rotation_data_t *data)
{
    uint32_t i;

    uint32_t x_start = data->x_start;
    uint32_t y_start = data->y_start;
    uint32_t x_end = data->x_end;
    uint32_t y_end = data->y_end;

    uint32_t dst_width = data->width;
    uint32_t dst_height = data->height;

    uint32_t src_bytes_per_line = x_end - x_start + 1;
    uint32_t offset = y_start * src_bytes_per_line + x_start;

    switch (data->rotation) {
        case ROTATION_90:
            dst_height--;

            for (uint32_t y=y_start;y<y_end;y++) {
                i = y * src_bytes_per_line - offset;

                for (uint32_t x=x_start;x<x_end;x++) {
                    copy_32bpp(src + (i + x),
                            dst + (((dst_height - x) * dst_width) + y));
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case ROTATION_180:
            dst_height--;
            offset = dst_width - 1 - x_start;

            for (uint32_t y=y_start;y<y_end;y++) {
                i = (dst_height - y) * dst_width + offset;

                for (uint32_t x=x_start;x<x_end;x++) {
                    copy_32bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case ROTATION_270:
            uint32_t dst_width_minus_one = dst_width - 1;

            for (uint32_t y=y_start;y<y_end;y++) {
                i = y * src_bytes_per_line - offset;
                dst_height = dst_width_minus_one - y;

                for (uint32_t x=x_start;x<x_end;x++) {
                    copy_32bpp(src + (i + x), dst + (x * dst_width + dst_height));
                }
            }
            break;

        default:
            break;
    }
}