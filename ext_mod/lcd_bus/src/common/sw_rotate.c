// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "common/sw_rotate.h"
#include "common/rgb565_dither.h"

#include <string.h>

#define RGB_BUS_ROTATION_0    (0)
#define RGB_BUS_ROTATION_90   (1)
#define RGB_BUS_ROTATION_180  (2)
#define RGB_BUS_ROTATION_270  (3)


static void rotate0(uint8_t *src, uint8_t *dst, mp_lcd_sw_rotation_data_t *copy_data);
static void rotate_8bpp(uint8_t *src, uint8_t *dst, mp_lcd_sw_rotation_data_t *copy_data);
static void rotate_16bpp(uint16_t *src, uint16_t *dst, mp_lcd_sw_rotation_data_t *copy_data);
static void rotate_24bpp(uint8_t *src, uint8_t *dst, mp_lcd_sw_rotation_data_t *copy_data);
static void rotate_32bpp(uint32_t *src, uint32_t *dst, mp_lcd_sw_rotation_data_t *copy_data);


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


void mp_lcd_sw_rotate(void *dst, void *src, mp_lcd_sw_rotation_data_t *copy_data)
{
    if (rotate == RGB_BUS_ROTATION_0) {
        copy_data->x_start = MIN(copy_data->x_start, copy_data->dst_width);
        copy_data->y_start = MIN(copy_data->y_start, copy_data->dst_height);
        copy_data->x_end = MIN(copy_data->x_end, copy_data->dst_width);
        copy_data->y_end = MIN(copy_data->y_end, copy_data->dst_height);
        rotate0(src, dst, copy_data);
    } else {
        y_end += 1;
        if (rotate == RGB_BUS_ROTATION_90 || rotate == RGB_BUS_ROTATION_270) {
            copy_data->x_start = MIN(copy_data->x_start, copy_data->dst_height);
            copy_data->x_end = MIN(copy_data->x_end, copy_data->dst_height);
            copy_data->y_start = MIN(copy_data->y_start, copy_data->dst_width);
            copy_data->y_end = MIN(copy_data->y_end, copy_data->dst_width);
        } else {
            copy_data->x_start = MIN(copy_data->x_start, copy_data->dst_width);
            copy_data->x_end = MIN(copy_data->x_end, copy_data->dst_width);
            copy_data->y_start = MIN(copy_data->y_start, copy_data->dst_height);
            copy_data->y_end = MIN(copy_data->y_end, copy_data->dst_height);
        }

        switch(bytes_per_pixel) {
            case 1:
                rotate_8bpp(src, dst, copy_data);
                break;
            case 2:
                rotate_16bpp(src, dst, copy_data);
                break;
            case 3:
                rotate_24bpp(src, dst, copy_data);
                break;
            case 4:
                rotate_32bpp(src, dst, copy_data);
                break
        }
    }
}


void rotate0(uint8_t *src, uint8_t *dst, mp_lcd_sw_rotation_data_t *copy_data)
{
    uint32_t x_start = copy_data->x_start;
    uint32_t y_start = copy_data->y_start;
    uint32_t x_end = copy_data->x_end;
    uint32_t y_end = copy_data->y_end;

    uint32_t dst_width = copy_data->dst_width;
    uint32_t dst_height = copy_data->dst_height;

    dst += ((y_start * dst_width + x_start) * copy_data->bytes_per_pixel);
    if(x_start == 0 && x_end == (dst_width - 1) && !copy_data->rgb565_dither) {
        memcpy(dst, src, dst_width * (y_end - y_start + 1) * copy_data->bytes_per_pixel);
    } else {
        uint32_t src_bytes_per_line = (x_end - x_start + 1) * copy_data->bytes_per_pixel;
        uint32_t dst_bytes_per_line = dst_width * copy_data->bytes_per_pixel;

        if (rgb565_dither) {
            for(uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    rgb565_dither_pixel(CALC_THRESHOLD(x, y), (uint16_t *)(src) + x);
                    copy_16bpp((uint16_t *)(src) + x, (uint16_t *)(dst) + x);
                }
                dst += dst_bytes_per_line;
                src += src_bytes_per_line;
            }
        } else {
            for(uint32_t y = y_start; y < y_end; y++) {
                memcpy(dst, src, src_bytes_per_line);
                dst += dst_bytes_per_line;
                src += src_bytes_per_line;
            }
        }
    }
}

void rotate_8bpp(uint8_t *src, uint8_t *dst, mp_lcd_sw_rotation_data_t *copy_data)
{
    uint32_t x_start = copy_data->x_start;
    uint32_t y_start = copy_data->y_start;
    uint32_t x_end = copy_data->x_end;
    uint32_t y_end = copy_data->y_end;

    uint32_t dst_width = copy_data->dst_width;
    uint32_t dst_height = copy_data->dst_height;

    uint32_t i;
    uint32_t j;

    uint32_t src_bytes_per_line = x_end - x_start + 1;
    uint32_t offset = y_start * src_bytes_per_line + x_start;

    switch (copy_data->rotate) {
        case RGB_BUS_ROTATION_90:
            for (uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    i = y * src_bytes_per_line + x - offset;
                    j = (dst_height - 1 - x) * dst_width + y;
                    copy_8bpp(src + i, dst + j);
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case RGB_BUS_ROTATION_180:
            LCD_UNUSED(j);
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);

            for (uint32_t y = y_start; y < y_end; y++) {
                i = (dst_height - 1 - y) * dst_width + (dst_width - 1 - x_start);
                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_8bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case RGB_BUS_ROTATION_270:
            for (uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    i = y * src_bytes_per_line + x - offset;
                    j = x * dst_width + dst_width - 1 - y;
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


void rotate_16bpp(uint16_t *src, mp_lcd_sw_rotation_data_t *copy_data)
{
    uint32_t x_start = copy_data->x_start;
    uint32_t y_start = copy_data->y_start;
    uint32_t x_end = copy_data->x_end;
    uint32_t y_end = copy_data->y_end;

    uint32_t dst_width = copy_data->dst_width;
    uint32_t dst_height = copy_data->dst_height;

    uint32_t i;
    uint32_t j;

    uint32_t src_bytes_per_line = x_end - x_start + 1;
    uint32_t offset = y_start * src_bytes_per_line + x_start;

    if (copy_data->rgb565_dither) {
        switch (copy_data->rotate) {
            case RGB_BUS_ROTATION_90:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (dst_height - 1 - x) * dst_width + y;
                        rgb565_dither_pixel(CALC_THRESHOLD(x, y), src + i);
                        copy_16bpp(src + i, dst + j);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case RGB_BUS_ROTATION_180:
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);

                for (uint32_t y = y_start; y < y_end; y++) {
                    i = (dst_height - 1 - y) * dst_width + (dst_width - 1 - x_start);
                    for (uint32_t x = x_start; x < x_end; x++) {
                        rgb565_dither_pixel(CALC_THRESHOLD(x, y), src);
                        copy_16bpp(src, dst + i);
                        src++;
                        i--;
                    }
                }
                break;

            // SWAP_XY   MIRROR_X
            case RGB_BUS_ROTATION_270:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (x * dst_width + dst_width - 1 - y);
                        rgb565_dither_pixel(CALC_THRESHOLD(x, y), src + i);
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
    } else {
        switch (copy_data->rotate) {
            case RGB_BUS_ROTATION_90:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (dst_height - 1 - x) * dst_width + y;
                        copy_16bpp(src + i, dst + j);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case RGB_BUS_ROTATION_180:
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);

                for (uint32_t y = y_start; y < y_end; y++) {
                    i = (dst_height - 1 - y) * dst_width + (dst_width - 1 - x_start);
                    for (uint32_t x = x_start; x < x_end; x++) {
                        copy_16bpp(src, dst + i);
                        src++;
                        i--;
                    }
                }
                break;

            // SWAP_XY   MIRROR_X
            case RGB_BUS_ROTATION_270:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (x * dst_width + dst_width - 1 - y);
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


void rotate_24bpp(uint8_t *src, uint8_t *dst, mp_lcd_sw_rotation_data_t *copy_data)
{
    uint32_t x_start = copy_data->x_start;
    uint32_t y_start = copy_data->y_start;
    uint32_t x_end = copy_data->x_end;
    uint32_t y_end = copy_data->y_end;

    uint32_t dst_width = copy_data->dst_width;
    uint32_t dst_height = copy_data->dst_height;

    uint32_t i;
    uint32_t j;

    uint32_t src_bytes_per_line = (x_end - x_start + 1) * 3;
    uint32_t offset = y_start * src_bytes_per_line + x_start * 3;

    switch (copy_data->rotate) {
        case RGB_BUS_ROTATION_90:
            for (uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    i = y * src_bytes_per_line + x * 3 - offset;
                    j = ((dst_height - 1 - x) * dst_width + y) * 3;
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
                i = ((dst_height - 1 - y) * dst_width + (dst_width - 1 - x_start)) * 3;
                for (size_t x = x_start; x < x_end; x++) {
                    copy_24bpp(src, dst + i);
                    src += 3;
                    i -= 3;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case RGB_BUS_ROTATION_270:
            for (uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    i = y * src_bytes_per_line + x * 3 - offset;
                    j = (x * dst_width + dst_width - 1 - y) * 3;
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


void rotate_32bpp(uint32_t *src, uint32_t *dst, mp_lcd_sw_rotation_data_t *copy_data)
{
    uint32_t x_start = copy_data->x_start;
    uint32_t y_start = copy_data->y_start;
    uint32_t x_end = copy_data->x_end;
    uint32_t y_end = copy_data->y_end;

    uint32_t dst_width = copy_data->dst_width;
    uint32_t dst_height = copy_data->dst_height;

    uint32_t i;
    uint32_t j;

    uint32_t src_bytes_per_line = x_end - x_start + 1;
    uint32_t offset = y_start * src_bytes_per_line + x_start;

    switch (copy_data->rotate) {
        case RGB_BUS_ROTATION_90:
            for (uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    i = y * src_bytes_per_line + x - offset;
                    j = (dst_height - 1 - x) * dst_width + y;
                    copy_32bpp(src + i, dst + j);
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case RGB_BUS_ROTATION_180:
            LCD_UNUSED(j);
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);

            for (uint32_t y = y_start; y < y_end; y++) {
                i = (dst_height - 1 - y) * dst_width + (dst_width - 1 - x_start);
                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_32bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case RGB_BUS_ROTATION_270:
            for (uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    i = y * src_bytes_per_line + x - offset;
                    j = x * dst_width + dst_width - 1 - y;
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

#endif
