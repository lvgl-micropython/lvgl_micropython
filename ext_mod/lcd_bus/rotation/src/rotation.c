

#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "rotation.h"
#include "dither.h"


#define RGB_BUS_ROTATION_0    (0)
#define RGB_BUS_ROTATION_90   (1)
#define RGB_BUS_ROTATION_180  (2)
#define RGB_BUS_ROTATION_270  (3)


static void rotate0(uint8_t *src, uint8_t *dst, rotation_data_t *r_data);
static void rotate_8bpp(uint8_t *src, uint8_t *dst, rotation_data_t *r_data);
static void rotate_16bpp(uint16_t *src, uint16_t *dst, rotation_data_t *r_data);
static void rotate_24bpp(uint8_t *src, uint8_t *dst, rotation_data_t *r_data);
static void rotate_32bpp(uint32_t *src, uint32_t *dst, rotation_data_t *r_data);


__attribute__((always_inline))
static inline void copy_8bpp(uint8_t *from, uint8_t *to)
{
    *to++ = *from++;
}


__attribute__((always_inline))
static inline void copy_swap_16bpp(uint16_t *from, uint16_t *to)
{
    *to++ = (*from >> 8) | (*from++ << 8);
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

void copy_pixels(void *dst, void *src, rotation_data_t *r_data)
{
    if (rotate == RGB_BUS_ROTATION_0) {
        r_data->x_start = MIN(r_data->x_start, r_data->dst_width);
        r_data->y_start = MIN(r_data->y_start, r_data->dst_height);
        r_data->x_end = MIN(r_data->x_end, r_data->dst_width);
        r_data->y_end = MIN(r_data->y_end, r_data->dst_height);

        rotate0(src, dst, r_data);
    } else {

        r_data->y_end += 1;

        if (rotate == RGB_BUS_ROTATION_90 || rotate == RGB_BUS_ROTATION_270) {
            r_data->x_start = MIN(r_data->x_start, r_data->dst_height);
            r_data->y_start = MIN(r_data->y_start, r_data->dst_width);
            r_data->x_end = MIN(r_data->x_end, r_data->dst_height);
            r_data->y_end = MIN(r_data->y_end, r_data->dst_width);
        } else {
            r_data->x_start = MIN(r_data->x_start, r_data->dst_width);
            r_data->y_start = MIN(r_data->y_start, r_data->dst_height);
            r_data->x_end = MIN(r_data->x_end, r_data->dst_width);
            r_data->y_end = MIN(r_data->y_end, r_data->dst_height);
        }

        switch (r_data->bytes_per_pixel) {
            case 1:
                rotate_8bpp(src, dst, r_data);
                break;
            case 2:
                rotate_16bpp(src, dst, r_data);
                break
            case 3:
                rotate_24bpp(src, dst, r_data);
                break
            case 4:
                rotate_32bpp(src, dst, r_data);
                break
        }
    }
}


void rotate0(uint8_t *src, uint8_t *dst, rotation_data_t *r_data)
{
    dst += ((r_data->y_start * r_data->dst_width + r_data->x_start) * r_data->bytes_per_pixel);
    if(r_data->x_start == 0 && r_data->x_end == (r_data->dst_width - 1) && !r_data->dither) {
        memcpy(dst, src, r_data->dst_width * (r_data->y_end - r_data->y_start + 1) * r_data->bytes_per_pixel);
    } else {
        uint32_t src_bytes_per_line = (r_data->x_end - r_data->x_start) * r_data->bytes_per_pixel;
        uint32_t dst_bytes_per_line = r_data->dst_width * r_data->bytes_per_pixel;

        if (r_data->dither && r_data->swap) {
            for(uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                for (uint32_t x=0; x < r_data->x_end; x++) {
                    rgb565_dither_swap_pixel(CALC_THRESHOLD(x, y), (uint16_t *)(dst) + x, (uint16_t *)(src) + x);
                }
                dst += dst_bytes_per_line;
                src += src_bytes_per_line;
            }
            return;
        } else if (r_data->dither) {
            switch(r_data->bytes_per_pixel) {
                case 2:
                    for(uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                        for (uint32_t x=0; x < r_data->x_end; x++) {
                            rgb565_dither_pixel(CALC_THRESHOLD(x, y), (uint16_t *)(dst) + x, (uint16_t *)(src) + x);
                        }
                        dst += dst_bytes_per_line;
                        src += src_bytes_per_line;
                    }
                    return;
                case 3:
                    for(uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                        for (uint32_t x=0; x < r_data->x_end; x++) {
                            rgb888_dither_pixel(CALC_THRESHOLD(x, y), (uint8_t *)dst + x, (uint8_t *)(src) + x);
                        }
                        dst += dst_bytes_per_line;
                        src += src_bytes_per_line;
                    }
                    return;
                default:
                    break;
            }
        }

        for(uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
            memcpy(dst, src, src_bytes_per_line);
            dst += dst_bytes_per_line;
            src += src_bytes_per_line;
        }
    }
}

void rotate_8bpp(uint8_t *src, uint8_t *dst, rotation_data_t *r_data)
{
    uint32_t i;
    uint32_t j;

    uint32_t src_bytes_per_line = r_data->x_end - r_data->x_start;
    uint32_t offset = r_data->y_start * src_bytes_per_line + r_data->x_start;

    switch (rotate) {
        case RGB_BUS_ROTATION_90:
            for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                    i = y * src_bytes_per_line + x - offset;
                    j = (r_data->dst_height - 1 - x) * r_data->dst_width + y;
                    copy_8bpp(src + i, dst + j);
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case RGB_BUS_ROTATION_180:
            LCD_UNUSED(j);
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);

            for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                i = (r_data->dst_height - 1 - y) * r_data->dst_width + (r_data->dst_width - 1 - r_data->x_start);
                for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                    copy_8bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case RGB_BUS_ROTATION_270:
            for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                    i = y * src_bytes_per_line + x - offset;
                    j = x * r_data->dst_width + r_data->dst_width - 1 - y;
                    copy_8bpp(src + i, dst + j);
                }
            }
            break;

        default:
            LCD_UNUSED(i);
            LCD_UNUSED(j);
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);
            break;
    }

}


void rotate_16bpp(uint16_t *src, uint16_t *dst, rotation_data_t *r_data)
{
    uint32_t i;
    uint32_t j;

    uint32_t src_bytes_per_line = r_data->x_end - r_data->x_start;
    uint32_t offset = r_data->y_start * src_bytes_per_line + r_data->x_start;


    if (r_data->dither && r_data->swap) {
        switch (r_data->rotation) {
            case RGB_BUS_ROTATION_90:
                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (r_data->dst_height - 1 - x) * r_data->dst_width + y;
                        rgb565_dither_swap_pixel(CALC_THRESHOLD(x, y), dst + j, src + i);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case RGB_BUS_ROTATION_180:
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);

                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    i = (r_data->dst_height - 1 - y) * r_data->dst_width + (r_data->dst_width - 1 - r_data->x_start);
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        rgb565_dither_swap_pixel(CALC_THRESHOLD(x, y), dst + i, src);
                        src++;
                        i--;
                    }
                }
                break;

            // SWAP_XY   MIRROR_X
            case RGB_BUS_ROTATION_270:
                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (x * r_data->dst_width + r_data->dst_width - 1 - y);
                        rgb565_dither_swap_pixel(CALC_THRESHOLD(x, y), dst + j, src + i);
                    }
                }
                break;

            default:
                LCD_UNUSED(i);
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);
                break;
        }
    } else if (r_data->swap) {
        switch (r_data->rotation) {
            case RGB_BUS_ROTATION_90:
                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (r_data->dst_height - 1 - x) * r_data->dst_width + y;
                        copy_swap_16bpp(src + i, dst + j);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case RGB_BUS_ROTATION_180:
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);

                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    i = (r_data->dst_height - 1 - y) * r_data->dst_width + (r_data->dst_width - 1 - r_data->x_start);
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        copy_swap_16bpp(src, dst + i);
                        src++;
                        i--;
                    }
                }
                break;

            // SWAP_XY   MIRROR_X
            case RGB_BUS_ROTATION_270:
                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (x * r_data->dst_width + r_data->dst_width - 1 - y);
                        copy_swap_16bpp(src + i, dst + j);
                    }
                }
                break;

            default:
                LCD_UNUSED(i);
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);
                break;
        }
    } else if (r_data->dither) {
        switch (r_data->rotation) {
            case RGB_BUS_ROTATION_90:
                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (r_data->dst_height - 1 - x) * r_data->dst_width + y;
                        rgb565_dither_pixel(CALC_THRESHOLD(x, y), dst + j, src + i);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case RGB_BUS_ROTATION_180:
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);

                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    i = (r_data->dst_height - 1 - y) * r_data->dst_width + (r_data->dst_width - 1 - r_data->x_start);
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        rgb565_dither_pixel(CALC_THRESHOLD(x, y), dst + i, src);
                        src++;
                        i--;
                    }
                }
                break;

            // SWAP_XY   MIRROR_X
            case RGB_BUS_ROTATION_270:
                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (x * r_data->dst_width + r_data->dst_width - 1 - y);
                        rgb565_dither_pixel(CALC_THRESHOLD(x, y), dst + j, src + i);
                    }
                }
                break;

            default:
                LCD_UNUSED(i);
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);
                break;
        }
    } else {
        switch (r_data->rotation) {
            case RGB_BUS_ROTATION_90:
                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (r_data->dst_height - 1 - x) * r_data->dst_width + y;
                        copy_16bpp(src + i, dst + j);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case RGB_BUS_ROTATION_180:
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);

                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    i = (r_data->dst_height - 1 - y) * r_data->dst_width + (r_data->dst_width - 1 - r_data->x_start);
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        copy_16bpp(src, dst + i);
                        src++;
                        i--;
                    }
                }
                break;

            // SWAP_XY   MIRROR_X
            case RGB_BUS_ROTATION_270:
                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (x * r_data->dst_width + r_data->dst_width - 1 - y);
                        copy_16bpp(src + i, dst + j);
                    }
                }
                break;

            default:
                LCD_UNUSED(i);
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);
                break;
        }
    }
}


void rotate_24bpp(uint8_t *src, uint8_t *dst, rotation_data_t *r_data)
{
    uint32_t i;
    uint32_t j;

    uint32_t src_bytes_per_line = (r_data->x_end - r_data->x_start) * 3;
    uint32_t offset = r_data->y_start * src_bytes_per_line + r_data->x_start * 3;


    if (r_data->dither) {
        switch (r_data->rotation) {
            case RGB_BUS_ROTATION_90:
                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        i = y * src_bytes_per_line + x * 3 - offset;
                        j = ((r_data->dst_height - 1 - x) * r_data->dst_width + y) * 3;
                        rgb888_dither_pixel(CALC_THRESHOLD(x, y), dst + j, src + i);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case RGB_BUS_ROTATION_180:
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);

                for (int y = y_start; y < y_end; y++) {
                    i = ((r_data->dst_height - 1 - y) * r_data->dst_width + (r_data->dst_width - 1 - r_data->x_start)) * 3;
                    for (size_t x = r_data->x_start; x < r_data->x_end; x++) {
                        rgb888_dither_pixel(CALC_THRESHOLD(x, y), dst + i, src);
                        src += 3;
                        i -= 3;
                    }
                }
                break;

            // SWAP_XY   MIRROR_X
            case RGB_BUS_ROTATION_270:
                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        i = y * src_bytes_per_line + x * 3 - offset;
                        j = (x * r_data->dst_width + r_data->dst_width - 1 - y) * 3;
                        rgb888_dither_pixel(CALC_THRESHOLD(x, y), dst + j, src + i);
                    }
                }
                break;

            default:
                LCD_UNUSED(i);
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);
                break;
        }
    } else {
        switch (r_data->rotation) {
            case RGB_BUS_ROTATION_90:
                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        i = y * src_bytes_per_line + x * 3 - offset;
                        j = ((r_data->dst_height - 1 - x) * r_data->dst_width + y) * 3;
                        copy_24bpp(src + i, dst + j);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case RGB_BUS_ROTATION_180:
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);

                for (int y = y_start; y < y_end; y++) {
                    i = ((r_data->dst_height - 1 - y) * r_data->dst_width + (r_data->dst_width - 1 - r_data->x_start)) * 3;
                    for (size_t x = r_data->x_start; x < r_data->x_end; x++) {
                        copy_24bpp(src, dst + i);
                        src += 3;
                        i -= 3;
                    }
                }
                break;

            // SWAP_XY   MIRROR_X
            case RGB_BUS_ROTATION_270:
                for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                    for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                        i = y * src_bytes_per_line + x * 3 - offset;
                        j = (x * r_data->dst_width + r_data->dst_width - 1 - y) * 3;
                        copy_24bpp(src + i, dst + j);
                    }
                }
                break;

            default:
                LCD_UNUSED(i);
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);
                break;
        }
    }
}


void rotate_32bpp(uint32_t *src, uint32_t *dst, rotation_data_t *r_data)
{
    uint32_t i;
    uint32_t j;

    uint32_t src_bytes_per_line = r_data->x_end - r_data->x_start;
    uint32_t offset = r_data->y_start * src_bytes_per_line + r_data->x_start;

    switch (r_data->rotation) {
        case RGB_BUS_ROTATION_90:
            for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                    i = y * src_bytes_per_line + x - offset;
                    j = (r_data->dst_height - 1 - x) * r_data->dst_width + y;
                    copy_32bpp(src + i, dst + j);
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case RGB_BUS_ROTATION_180:
            LCD_UNUSED(j);
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);

            for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                i = (r_data->dst_height - 1 - y) * r_data->dst_width + (r_data->dst_width - 1 - r_data->x_start);
                for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                    copy_32bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case RGB_BUS_ROTATION_270:
            for (uint32_t y = r_data->y_start; y < r_data->y_end; y++) {
                for (uint32_t x = r_data->x_start; x < r_data->x_end; x++) {
                    i = y * src_bytes_per_line + x - offset;
                    j = x * r_data->dst_width + r_data->dst_width - 1 - y;
                    copy_32bpp(src + i, dst + j);
                }
            }
            break;

        default:
            LCD_UNUSED(i);
            LCD_UNUSED(j);
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);
            break;
    }
}
