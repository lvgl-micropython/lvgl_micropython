// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "lcd_sw_rotate.h"
#include "lcd_dither.h"


static void rotate0(uint8_t *src, uint8_t *dst, 
                    uint32_t x_start, uint32_t y_start,
                    uint32_t x_end, uint32_t y_end, 
                    uint32_t dst_width, uint32_t dst_height,
                    uint8_t bytes_per_pixel, uint8_t dither, uint8_t byteswap);

static void rotate_8bpp(uint8_t *src, uint8_t *dst, 
                        uint32_t x_start, uint32_t y_start,
                        uint32_t x_end, uint32_t y_end, 
                        uint32_t dst_width, uint32_t dst_height,
                        uint8_t rotate);

static void rotate_16bpp(uint16_t *src, uint16_t *dst, 
                         uint32_t x_start, uint32_t y_start,
                         uint32_t x_end, uint32_t y_end, 
                         uint32_t dst_width, uint32_t dst_height,
                         uint8_t rotate, uint8_t dither, uint8_t byteswap);

static void rotate_24bpp(uint8_t *src, uint8_t *dst, 
                         uint32_t x_start, uint32_t y_start,
                         uint32_t x_end, uint32_t y_end, 
                         uint32_t dst_width, uint32_t dst_height,
                         uint8_t rotate, uint8_t dither);


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
static inline void copy_swap_16bpp(uint16_t *from, uint16_t *to)
{
    *to++ = (*from << 8) | (*from++ >> 8);
}


__attribute__((always_inline))
static inline void copy_24bpp(uint8_t *from, uint8_t *to)
{
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
}


void rotate(void *dst, void *src, 
            uint32_t x_start, uint32_t y_start,
            uint32_t x_end, uint32_t y_end, 
            uint32_t dst_width, uint32_t dst_height,
            uint32_t bytes_per_pixel, uint8_t rotate, 
            uint8_t dither, uint8_t byteswap)
{
    if ((rotate == LCD_ROTATION_0) && ((!dither && !byteswap) || bytes_per_pixel == 1)) {
        rotate0(src, dst, MIN(x_start, dst_width), MIN(y_start, dst_height),
                MIN(x_end, dst_width), MIN(y_end, dst_height),
                dst_width, dst_height, bytes_per_pixel);
    } else if (rotate == LCD_ROTATION_0 && (dither || byteswap)) {
        if (bytes_per_pixel == 2) {
            rotate_16bpp(src, dst,
                         MIN(x_start, dst_width), MIN(y_start, dst_height),
                         MIN(x_end, dst_width), MIN(y_end, dst_height),
                         dst_width, dst_height, rotate, dither, byteswap);
        } else if (bytes_per_pixel == 3) {
            rotate_24bpp(src, dst,
                         MIN(x_start, dst_width), MIN(y_start, dst_height),
                         MIN(x_end, dst_width), MIN(y_end, dst_height),
                         dst_width, dst_height, rotate, dither);
        }
    } else {
        y_end += 1; // removes black lines between blocks
        if (rotate == LCD_ROTATION_90 || rotate == LCD_ROTATION_270) {
            x_start = MIN(x_start, dst_height);
            x_end = MIN(x_end, dst_height);
            y_start = MIN(y_start, dst_width);
            y_end = MIN(y_end, dst_width);
        } else {
            x_start = MIN(x_start, dst_width);
            x_end = MIN(x_end, dst_width);
            y_start = MIN(y_start, dst_height);
            y_end = MIN(y_end, dst_height);
        }

        if (bytes_per_pixel == 1) {
            rotate_8bpp(src, dst, x_start, y_start, x_end, y_end, dst_width, dst_height, rotate);
        } else if (bytes_per_pixel == 2) {
            rotate_16bpp(src, dst, x_start, y_start, x_end, y_end, dst_width, dst_height, rotate, dither, byteswap);
        } else if (bytes_per_pixel == 3) {
            rotate_24bpp(src, dst, x_start, y_start, x_end, y_end, dst_width, dst_height, rotate, dither);
        }
    }
}


void rotate0(uint8_t *src, uint8_t *dst, 
             uint32_t x_start, uint32_t y_start,
             uint32_t x_end, uint32_t y_end, 
             uint32_t dst_width, uint32_t dst_height, 
             uint8_t bytes_per_pixel)
{
    dst += ((y_start * dst_width + x_start) * bytes_per_pixel);
    if (x_start == 0 && x_end == (dst_width - 1)) {
        memcpy(dst, src, dst_width * (y_end - y_start + 1) * bytes_per_pixel);
    } else {
        uint32_t src_bytes_per_line = (x_end - x_start + 1) * bytes_per_pixel;
        uint32_t dst_bytes_per_line = dst_width * bytes_per_pixel;

        for(uint32_t y = y_start; y < y_end; y++) {
            memcpy(dst, src, src_bytes_per_line);
            dst += dst_bytes_per_line;
            src += src_bytes_per_line;
        }
    }
}


void rotate_8bpp(uint8_t *src, uint8_t *dst, 
                 uint32_t x_start, uint32_t y_start,
                 uint32_t x_end, uint32_t y_end, 
                 uint32_t dst_width, uint32_t dst_height,
                 uint8_t rotate)
{
    uint32_t i;
    uint32_t j;

    uint32_t src_bytes_per_line = x_end - x_start + 1;
    uint32_t offset = y_start * src_bytes_per_line + x_start;

    switch (rotate) {
        case LCD_ROTATION_90:
            for (uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    i = y * src_bytes_per_line + x - offset;
                    j = (dst_height - 1 - x) * dst_width + y;
                    copy_8bpp(src + i, dst + j);
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case LCD_ROTATION_180:
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
                src++;
            }
            break;

        // SWAP_XY   MIRROR_X
        case LCD_ROTATION_270:
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


void rotate_16bpp(uint16_t *src, uint16_t *dst, 
                  uint32_t x_start, uint32_t y_start,
                  uint32_t x_end, uint32_t y_end, 
                  uint32_t dst_width, uint32_t dst_height,
                  uint8_t rotate, uint8_t dither, uint8_t byteswap)
{
    uint32_t i;
    uint32_t j;

    uint32_t src_bytes_per_line = x_end - x_start + 1;
    uint32_t offset = y_start * src_bytes_per_line + x_start;
    
    if (dither && byteswap) {
        switch (rotate) {
            case LCD_ROTATION_0:

            case LCD_ROTATION_90:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (dst_height - 1 - x) * dst_width + y;
                        dither_565(CALC_THRESHOLD(x, y), src + i);
                        copy_swap_16bpp(src + i, dst + j);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case LCD_ROTATION_180:
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);

                for (uint32_t y = y_start; y < y_end; y++) {
                    i = (dst_height - 1 - y) * dst_width + (dst_width - 1 - x_start);
                    for (uint32_t x = x_start; x < x_end; x++) {
                        dither_565(CALC_THRESHOLD(x, y), src);
                        copy_swap_16bpp(src, dst + i);
                        src++;
                        i--;
                    }
                    src++;
                }
                break;

            // SWAP_XY   MIRROR_X
            case LCD_ROTATION_270:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (x * dst_width + dst_width - 1 - y);
                        dither_565(CALC_THRESHOLD(x, y), src + i);
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
    
    } else if (dither) {
        switch (rotate) {
            case LCD_ROTATION_0:

            case LCD_ROTATION_90:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (dst_height - 1 - x) * dst_width + y;
                        dither_565(CALC_THRESHOLD(x, y), src + i);
                        copy_16bpp(src + i, dst + j);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case LCD_ROTATION_180:
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);

                for (uint32_t y = y_start; y < y_end; y++) {
                    i = (dst_height - 1 - y) * dst_width + (dst_width - 1 - x_start);
                    for (uint32_t x = x_start; x < x_end; x++) {
                        dither_565(CALC_THRESHOLD(x, y), src);
                        copy_16bpp(src, dst + i);
                        src++;
                        i--;
                    }
                    src++;
                }
                break;

            // SWAP_XY   MIRROR_X
            case LCD_ROTATION_270:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (x * dst_width + dst_width - 1 - y);
                        dither_565(CALC_THRESHOLD(x, y), src + i);
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
    } else if (swap) {
        switch (rotate) {
            case LCD_ROTATION_0:

            case LCD_ROTATION_90:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (dst_height - 1 - x) * dst_width + y;
                        copy_swap_16bpp(src + i, dst + j);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case LCD_ROTATION_180:
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);

                for (uint32_t y = y_start; y < y_end; y++) {
                    i = (dst_height - 1 - y) * dst_width + (dst_width - 1 - x_start);
                    for (uint32_t x = x_start; x < x_end; x++) {
                        copy_swap_16bpp(src, dst + i);
                        src++;
                        i--;
                    }
                    src++;
                }
                break;

            // SWAP_XY   MIRROR_X
            case LCD_ROTATION_270:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (x * dst_width + dst_width - 1 - y);
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
    } else {
        switch (rotate) {
            case LCD_ROTATION_90:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x - offset;
                        j = (dst_height - 1 - x) * dst_width + y;
                        copy_16bpp(src + i, dst + j);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case LCD_ROTATION_180:
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
                    src++;
            }
                break;

            // SWAP_XY   MIRROR_X
            case LCD_ROTATION_270:
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


void rotate_24bpp(uint8_t *src, uint8_t *dst, 
                  uint32_t x_start, uint32_t y_start,
                  uint32_t x_end, uint32_t y_end, 
                  uint32_t dst_width, uint32_t dst_height,
                  uint8_t rotate, uint8_t dither)
{
    uint32_t i;
    uint32_t j;

    uint32_t src_bytes_per_line = (x_end - x_start + 1) * 3;
    uint32_t offset = y_start * src_bytes_per_line + x_start * 3;

    if (dither) {
        switch (rotate) {
            case LCD_ROTATION_0:

            case LCD_ROTATION_90:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x * 3 - offset;
                        j = ((dst_height - 1 - x) * dst_width + y) * 3;
                        dither_888(CALC_THRESHOLD(x, y), src + i);
                        copy_24bpp(src + i, dst + j);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case LCD_ROTATION_180:
                LCD_UNUSED(j);
                LCD_UNUSED(src_bytes_per_line);
                LCD_UNUSED(offset);

                for (int y = y_start; y < y_end; y++) {
                    i = ((dst_height - 1 - y) * dst_width + (dst_width - 1 - x_start)) * 3;
                    for (size_t x = x_start; x < x_end; x++) {
                        dither_888(CALC_THRESHOLD(x, y), src + i);
                        copy_24bpp(src, dst + i);
                        src += 3;
                        i -= 3;
                    }
                    src++;
                }
                break;

            // SWAP_XY   MIRROR_X
            case LCD_ROTATION_270:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x * 3 - offset;
                        j = (x * dst_width + dst_width - 1 - y) * 3;
                        dither_888(CALC_THRESHOLD(x, y), src + i);
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

    } else {
        switch (rotate) {
            case LCD_ROTATION_90:
                for (uint32_t y = y_start; y < y_end; y++) {
                    for (uint32_t x = x_start; x < x_end; x++) {
                        i = y * src_bytes_per_line + x * 3 - offset;
                        j = ((dst_height - 1 - x) * dst_width + y) * 3;
                        copy_24bpp(src + i, dst + j);
                    }
                }
                break;

            // MIRROR_X MIRROR_Y
            case LCD_ROTATION_180:
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
                    src++;
                }
                break;

            // SWAP_XY   MIRROR_X
            case LCD_ROTATION_270:
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
}


