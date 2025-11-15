// Copyright (c) 2024 - 2025 Kevin G. Schlosser
// Copyright (c) 2024 - 2025 Viktor Vorobjov

#include "spi_bus_fast.h"
#include "rom/ets_sys.h"  // ets_printf

#include <string.h>

// MIN macro for coordinate clipping
#define MIN(x, y) ((x) < (y) ? (x) : (y))

// Rotation constants
#define SPI_FAST_ROTATION_0    (0)
#define SPI_FAST_ROTATION_90   (1)
#define SPI_FAST_ROTATION_180  (2)
#define SPI_FAST_ROTATION_270  (3)


// Copy functions for different pixel formats
static void copy_8bpp(uint8_t *from, uint8_t *to)
{
    *to = *from;
}

static void copy_16bpp(uint16_t *from, uint16_t *to)
{
    *to = *from;
}

static void copy_24bpp(uint8_t *from, uint8_t *to)
{
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
}

static void copy_32bpp(uint32_t *from, uint32_t *to)
{
    *to = *from;
}

// Rotation function for 16bpp (RGB565) - most common case
static void rotate_16bpp(uint16_t *src, uint16_t *dst, uint32_t x_start, uint32_t y_start,
                        uint32_t x_end, uint32_t y_end, uint32_t dst_width, uint32_t dst_height,
                        uint8_t rotate)
{
    uint32_t i;
    uint32_t j;
    
    // Source image geometry: calculate line width and starting offset
    uint32_t src_bytes_per_line = x_end - x_start + 1;
    uint32_t offset = y_start * src_bytes_per_line + x_start;
    
    switch (rotate) {
        case SPI_FAST_ROTATION_90:

            // Fix for missing right column: extend x_end by 1 pixel
            // This was discovered during debugging - without this, 
            // a vertical strip of pixels was missing on the right edge
            uint32_t x_end_extended = x_end + 1;
            for (uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end_extended; x++) {
                    // Calculate source index (relative to partial buffer)
                    i = y * src_bytes_per_line + x - offset;
                    // 90° rotation formula: (x,y) -> (y, height-1-x)
                    j = (dst_height - 1 - x) * dst_width + y;
                    // check that we don't go out of bounds
                    if (j < (dst_width * dst_height)) {
                        copy_16bpp(src + i, dst + j);
                    }
                }
            }
            break;
            
        case SPI_FAST_ROTATION_180:
            for (uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    i = y * src_bytes_per_line + x - offset;
                    // 180° rotation formula: (x,y) -> (width-1-x, height-1-y)
                    j = ((dst_height - 1 - y) * dst_width) + dst_width - 1 - x;
                    copy_16bpp(src + i, dst + j);
                }
            }
            break;
            
        case SPI_FAST_ROTATION_270:
            for (uint32_t y = y_start; y < y_end; y++) {
                for (uint32_t x = x_start; x < x_end; x++) {
                    i = y * src_bytes_per_line + x - offset;
                    // 270° rotation formula: (x,y) -> (width-1-y, x)
                    j = x * dst_width + dst_width - 1 - y;
                    copy_16bpp(src + i, dst + j);
                }
            }
            break;
    }
}

// Main copy function with rotation support
void spi_fast_copy_pixels(void *dst, void *src, uint32_t x_start, uint32_t y_start,
                         uint32_t x_end, uint32_t y_end, uint32_t dst_width, uint32_t dst_height,
                         uint32_t bytes_per_pixel, uint8_t rotate, uint8_t rgb565_dither)
{
    
    // EXTREME DEFENSIVE: check all parameters
    if (dst == NULL || src == NULL || bytes_per_pixel == 0 || 
        x_end < x_start || y_end < y_start ||
        dst_width == 0 || dst_height == 0) {
        ets_printf("COPY_PIXELS: INVALID PARAMS!\n");
        return; // Invalid parameters
    }
    
    if (rotate == SPI_FAST_ROTATION_0) {
        // No rotation - direct copy
        uint8_t *dst_ptr = (uint8_t*)dst + ((y_start * dst_width + x_start) * bytes_per_pixel);
        uint32_t width = x_end - x_start + 1;
        uint32_t height = y_end - y_start + 1;
        
        if (x_start == 0 && x_end == (dst_width - 1)) {
            // Full width copy - most efficient
            memcpy(dst_ptr, src, width * height * bytes_per_pixel);
        } else {
            // Line by line copy
            uint8_t *src_ptr = (uint8_t*)src;
            for (uint32_t y = 0; y < height; y++) {
                memcpy(dst_ptr + y * dst_width * bytes_per_pixel, 
                       src_ptr + y * width * bytes_per_pixel, 
                       width * bytes_per_pixel);
            }
        }
    } else {
        // Rotation needed - clip coordinates like
        // Fix for black lines between LVGL update blocks
        // LVGL sends partial updates as rectangular blocks, and without this adjustment,
        // thin black lines appear between blocks after rotation
        y_end += 1; 
        
        // Coordinate clipping to prevent buffer overrun
        // For 90°/270° rotations, width and height are swapped in coordinate space
        if (rotate == SPI_FAST_ROTATION_90 || rotate == SPI_FAST_ROTATION_270) {
            // After 90°/270° rotation: x maps to dst_height, y maps to dst_width
            x_start = MIN(x_start, dst_height);
            x_end = MIN(x_end, dst_height);
            y_start = MIN(y_start, dst_width);
            y_end = MIN(y_end, dst_width);
        } else {
            // For 0°/180° rotation: normal coordinate mapping
            x_start = MIN(x_start, dst_width);
            x_end = MIN(x_end, dst_width);
            y_start = MIN(y_start, dst_height);
            y_end = MIN(y_end, dst_height);
        }
        
        
        if (bytes_per_pixel == 2) {
            rotate_16bpp(src, dst, x_start, y_start, x_end, y_end, dst_width, dst_height, rotate);
        }
        // TODO: Add support for other bit depths if needed (8bpp, 24bpp, 32bpp)
    }
}