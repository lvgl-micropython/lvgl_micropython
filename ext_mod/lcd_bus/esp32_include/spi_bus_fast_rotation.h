// Copyright (c) 2024 - 2025 Kevin G. Schlosser
// Copyright (c) 2024 - 2025 Viktor Vorobjov


#ifndef __SPI_BUS_FAST_ROTATION_H__
#define __SPI_BUS_FAST_ROTATION_H__

#include <stdint.h>

// Rotation constants
#define SPI_FAST_ROTATION_0    (0)
#define SPI_FAST_ROTATION_90   (1) 
#define SPI_FAST_ROTATION_180  (2)
#define SPI_FAST_ROTATION_270  (3)

// Main rotation function for SPIBusFast
void spi_fast_copy_pixels(void *dst, void *src, uint32_t x_start, uint32_t y_start,
                         uint32_t x_end, uint32_t y_end, uint32_t dst_width, uint32_t dst_height,
                         uint32_t bytes_per_pixel, uint8_t rotate, uint8_t rgb565_dither);

#endif // __SPI_BUS_FAST_ROTATION_H__