// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "common/sw_rotate.h"
#include "common/rgb565_dither.h"

#include <string.h>

static void rotate0(uint8_t *src, uint8_t *dst, mp_lcd_sw_rotation_data_t *copy_data);
static void rotate_8bpp(uint8_t *src, uint8_t *dst, mp_lcd_sw_rotation_data_t *copy_data);
static void rotate_16bpp(uint16_t *src, uint16_t *dst, mp_lcd_sw_rotation_data_t *copy_data);
static void rotate_16bpp_swap_dither(uint16_t *src, uint16_t *dst, mp_lcd_sw_rotation_data_t *copy_data);
static void rotate_16bpp_dither(uint16_t *src, uint16_t *dst, mp_lcd_sw_rotation_data_t *copy_data);
static void rotate_16bpp_swap(uint16_t *src, uint16_t *dst, mp_lcd_sw_rotation_data_t *copy_data);
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
    uint8_t rotation = copy_data->rotation;
    uint8_t bytes_per_pixel = copy_data->bytes_per_pixel;

    if (rotation == LCD_ROTATION_0) {
        copy_data->x_start = LCD_MIN(copy_data->x_start, copy_data->dst_width);
        copy_data->y_start = LCD_MIN(copy_data->y_start, copy_data->dst_height);
        copy_data->x_end = LCD_MIN(copy_data->x_end, copy_data->dst_width);
        copy_data->y_end = LCD_MIN(copy_data->y_end, copy_data->dst_height);
    } else {
       copy_data->y_end += 1;
        if (rotation == LCD_ROTATION_90 || rotation == LCD_ROTATION_270) {
            copy_data->x_start = LCD_MIN(copy_data->x_start, copy_data->dst_height);
            copy_data->x_end = LCD_MIN(copy_data->x_end, copy_data->dst_height);
            copy_data->y_start = LCD_MIN(copy_data->y_start, copy_data->dst_width);
            copy_data->y_end = LCD_MIN(copy_data->y_end, copy_data->dst_width);
        } else {
            copy_data->x_start = LCD_MIN(copy_data->x_start, copy_data->dst_width);
            copy_data->x_end = LCD_MIN(copy_data->x_end, copy_data->dst_width);
            copy_data->y_start = LCD_MIN(copy_data->y_start, copy_data->dst_height);
            copy_data->y_end = LCD_MIN(copy_data->y_end, copy_data->dst_height);
        }
    }

    if (bytes_per_pixel == 2) {
        uint8_t swap = copy_data->rgb565_swap;
        uint8_t dither = copy_data->rgb565_dither;

        if (swap && dither) {
            rotate_16bpp_swap_dither(src, dst, copy_data);
        } else if (dither) {
            rotate_16bpp_dither(src, dst, copy_data);
        } else if (swap) {
            rotate_16bpp_swap(src, dst, copy_data);
        } else {
            rotate_16bpp(src, dst, copy_data);
        }
    } else if (rotation == LCD_ROTATION_0) {
        rotate0(src, dst, copy_data);
    } else {
        switch(bytes_per_pixel) {
            case 1:
                rotate_8bpp(src, dst, copy_data);
                break;
            case 3:
                rotate_24bpp(src, dst, copy_data);
                break;
            case 4:
                rotate_32bpp(src, dst, copy_data);
                break;
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

    dst += ((y_start * dst_width + x_start) * copy_data->bytes_per_pixel);
    if(x_start == 0 && x_end == (dst_width - 1) && !copy_data->rgb565_dither) {
        memcpy(dst, src, dst_width * (y_end - y_start + 1) * copy_data->bytes_per_pixel);
    } else {
        uint32_t src_bytes_per_line = (x_end - x_start + 1) * copy_data->bytes_per_pixel;
        uint32_t dst_bytes_per_line = dst_width * copy_data->bytes_per_pixel;

        for(uint32_t y = y_start; y < y_end; y++) {
            memcpy(dst, src, src_bytes_per_line);
            dst += dst_bytes_per_line;
            src += src_bytes_per_line;
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

    switch (copy_data->rotation) {
        case LCD_ROTATION_90:
            uint32_t src_line_bytes;
            dst_height -= 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                src_line_bytes = (y * src_bytes_per_line) - offset;

                for (uint32_t x = x_start; x < x_end; x++) {
                    i = src_line_bytes + x;
                    j = ((dst_height - x) * dst_width) + y;
                    copy_8bpp(src + i, dst + j);
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case LCD_ROTATION_180:
            j = dst_width - 1 - x_start;
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);
            dst_height -= 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                i = ((dst_height - y) * dst_width) + j;
                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_8bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case LCD_ROTATION_270:
            uint32_t m;
            uint32_t o;

            for (uint32_t y = y_start; y < y_end; y++) {
                m = y * src_bytes_per_line;
                o = dst_width - 1 - y;

                for (uint32_t x = x_start; x < x_end; x++) {
                    i = m + x;
                    j = (x * dst_width) + o;
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


void rotate_16bpp_swap_dither(uint16_t *src, uint16_t *dst, mp_lcd_sw_rotation_data_t *copy_data)
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

    switch (copy_data->rotation) {
        case LCD_ROTATION_0:
            for(uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    rgb565_dither_byte_swap_pixel(CALC_THRESHOLD(x, y), src + x, dst + x);
                    // copy_16bpp(src + x, dst + x);
                }
                dst += dst_width;
                src += src_bytes_per_line;
            }
            break;    
    
        case LCD_ROTATION_90:
            uint32_t src_line_bytes;
            dst_height -= 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                src_line_bytes = (y * src_bytes_per_line) - offset;
                for (uint32_t x = x_start; x < x_end; x++) {
                    i = src_line_bytes + x;
                    j = ((dst_height - x) * dst_width) + y;
                    rgb565_dither_byte_swap_pixel(CALC_THRESHOLD(x, y), src + i, dst + j);
                    // copy_16bpp(src + i, dst + j);
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case LCD_ROTATION_180:
            j = dst_width - 1 - x_start;
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);
            dst_height -= 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                i = ((dst_height - y) * dst_width) + j;
                for (uint32_t x = x_start; x < x_end; x++) {
                    rgb565_dither_byte_swap_pixel(CALC_THRESHOLD(x, y), src, dst + i);

                    // copy_16bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case LCD_ROTATION_270:
            uint32_t m;
            uint32_t o;

            for (uint32_t y = y_start; y < y_end; y++) {
                m = (y * src_bytes_per_line) - offset;
                o = dst_width - 1 - y;

                for (uint32_t x = x_start; x < x_end; x++) {
                    i = m + x;
                    j = (x * dst_width) + o;
                    rgb565_dither_byte_swap_pixel(CALC_THRESHOLD(x, y), src + i, dst + j);

                    // copy_16bpp(src + i, dst + j);
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


void rotate_16bpp_dither(uint16_t *src, uint16_t *dst, mp_lcd_sw_rotation_data_t *copy_data)
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

    switch (copy_data->rotation) {
        case LCD_ROTATION_0:
            for(uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    rgb565_dither_pixel(CALC_THRESHOLD(x, y), src + x, dst + x);

                    // copy_16bpp(src + x, dst + x);
                }
                dst += dst_width;
                src += src_bytes_per_line;
            }
            break;    
    
        case LCD_ROTATION_90:
            uint32_t src_line_bytes;
            dst_height -= 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                src_line_bytes = (y * src_bytes_per_line) - offset;

                for (uint32_t x = x_start; x < x_end; x++) {
                    i = src_line_bytes + x;
                    j = ((dst_height - x) * dst_width) + y;
                    rgb565_dither_pixel(CALC_THRESHOLD(x, y), src + i, dst + j);

                    // copy_16bpp(src + i, dst + j);
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case LCD_ROTATION_180:
            j = dst_width - 1 - x_start;
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);
            dst_height -= 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                i = ((dst_height - y) * dst_width) + j;
                for (uint32_t x = x_start; x < x_end; x++) {
                    rgb565_dither_pixel(CALC_THRESHOLD(x, y), src, dst + i);

                    // copy_16bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case LCD_ROTATION_270:
            uint32_t m;
            uint32_t o;

            for (uint32_t y = y_start; y < y_end; y++) {
                m = (y * src_bytes_per_line) - offset;
                o = dst_width - 1 - y;

                for (uint32_t x = x_start; x < x_end; x++) {
                    i = m + x;
                    j = (x * dst_width) + o;
                    rgb565_dither_pixel(CALC_THRESHOLD(x, y), src + i, dst + j);

                    // copy_16bpp(src + i, dst + j);
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


void rotate_16bpp_swap(uint16_t *src, uint16_t *dst, mp_lcd_sw_rotation_data_t *copy_data)
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

    switch (copy_data->rotation) {
        case LCD_ROTATION_0:
            for(uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    *(dst + x) = (*(src + x) << 8) | (*(src + x) >> 8);
                    
                    // copy_16bpp(src + x, dst + x);
                }
                dst += dst_width;
                src += src_bytes_per_line;
            }
            break;    
    
        case LCD_ROTATION_90:
            uint32_t src_line_bytes;
            dst_height -= 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    i = src_line_bytes + x;
                    j = ((dst_height - x) * dst_width) + y;
                    *(dst + j) = (*(src + i) << 8) | (*(src + i) >> 8);
                    
                    // copy_16bpp(src + i, dst + j);
                }
            }
            break;
        
        // MIRROR_X MIRROR_Y
        case LCD_ROTATION_180:
            j = dst_width - 1 - x_start;
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);
            dst_height -= 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                i = ((dst_height - y) * dst_width) + j;
                for (uint32_t x = x_start; x < x_end; x++) {
                    *(dst + i) = (*src << 8) | (*src >> 8);
                    
                    // copy_16bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case LCD_ROTATION_270:
            uint32_t m;
            uint32_t o;

            for (uint32_t y = y_start; y < y_end; y++) {
                m = (y * src_bytes_per_line) - offset;
                o = dst_width - 1 - y;

                for (uint32_t x = x_start; x < x_end; x++) {
                    i = m + x;
                    j = (x * dst_width) + o;
                    *(dst + j) = (*(src + i) << 8) | (*(src + i) >> 8);
                    
                    // copy_16bpp(src + i, dst + j);
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


void rotate_16bpp(uint16_t *src, uint16_t *dst, mp_lcd_sw_rotation_data_t *copy_data)
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

    switch (copy_data->rotation) {
        case LCD_ROTATION_0:
            if(x_start == 0 && x_end == (dst_width - 1)) {
                memcpy(dst, src, dst_width * (y_end - y_start + 1) * 2);
            } else {
                uint32_t src_bytes_per_line = (x_end - x_start + 1) * 2;
                uint32_t dst_bytes_per_line = dst_width * 2;

                for(uint32_t y = y_start; y < y_end; y++) {
                    memcpy(dst, src, src_bytes_per_line);
                    dst += dst_bytes_per_line;
                    src += src_bytes_per_line;
                }
            }
            break;

        case LCD_ROTATION_90:
            uint32_t src_line_bytes;
            dst_height -= 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                src_line_bytes = (y * src_bytes_per_line) - offset;

                for (uint32_t x = x_start; x < x_end; x++) {
                    i = src_line_bytes + x;
                    j = ((dst_height - x) * dst_width) + y;
                    copy_16bpp(src + i, dst + j);
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case LCD_ROTATION_180:
            j = dst_width - 1 - x_start;
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);
            dst_height -= 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                i = ((dst_height - y) * dst_width) + j;
                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_16bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case LCD_ROTATION_270:
            uint32_t m;
            uint32_t o;

            for (uint32_t y = y_start; y < y_end; y++) {
                m = (y * src_bytes_per_line) - offset;
                o = dst_width - 1 - y;

                for (uint32_t x = x_start; x < x_end; x++) {
                    i = m + x;
                    j = (x * dst_width) + o;
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

    switch (copy_data->rotation) {
        case LCD_ROTATION_90:
            uint32_t src_line_bytes;
            dst_height -= 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                src_line_bytes = (y * src_bytes_per_line) - offset;

                for (uint32_t x = x_start; x < x_end; x++) {
                    i = src_line_bytes + (x * 3);
                    j = (((dst_height - x) * dst_width) + y) * 3;
                    copy_24bpp(src + i, dst + j);
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case LCD_ROTATION_180:
            j = dst_width - 1 - x_start;
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);
            dst_height -= 1;

            for (int y = y_start; y < y_end; y++) {
                i = (((dst_height - y) * dst_width) + j) * 3;
                for (size_t x = x_start; x < x_end; x++) {
                    copy_24bpp(src, dst + i);
                    src += 3;
                    i -= 3;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case LCD_ROTATION_270:
            uint32_t m;
            uint32_t o;

            for (uint32_t y = y_start; y < y_end; y++) {
                m = (y * src_bytes_per_line) - offset;
                o = dst_width - 1 - y;

                for (uint32_t x = x_start; x < x_end; x++) {
                    i = m + (x * 3);
                    j = ((x * dst_width) + o) * 3;
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

    switch (copy_data->rotation) {
        case LCD_ROTATION_90:
            uint32_t src_line_bytes;
            dst_height -= 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                src_line_bytes = (y * src_bytes_per_line) - offset;

                for (uint32_t x = x_start; x < x_end; x++) {
                    i = src_line_bytes + x;
                    j = ((dst_height - x) * dst_width) + y;
                    copy_32bpp(src + i, dst + j);
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case LCD_ROTATION_180:
            j = dst_width - 1 - x_start;
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);
            dst_height -= 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                i = ((dst_height - y) * dst_width) + j;

                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_32bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case LCD_ROTATION_270:
            uint32_t m;
            uint32_t o;

            for (uint32_t y = y_start; y < y_end; y++) {
                m = (y * src_bytes_per_line) - offset;
                o = dst_width - 1 - y;

                for (uint32_t x = x_start; x < x_end; x++) {
                    i = m + x;
                    j = (x * dst_width) + o;
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
