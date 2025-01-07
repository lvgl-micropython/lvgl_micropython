# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time


# HD371001C40 3.71" bar display
# 1 /* hync_polarity */, 20 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 20 /* hsync_back_porch */,
# 1 /* vsync_polarity */, 20 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch, */,
# 0 /* pclk_active_neg */, GFX_NOT_DEFINED /* prefer_speed */, false /* useBigEndian */, 0 /* de_idle_high */,
# 0 /* pclk_idle_high */, 120 /* col_offset1 */ );
def init(self):
    param_buf = bytearray(16)
    param_mv = memoryview(param_buf)
    
    param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, 0x13])
    self.set_params(0xFF, param_mv[:5])

    param_buf[0] = 0x08
    self.set_params(0xEF, param_mv[:1])

    param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, 0x10])
    self.set_params(0xFF, param_mv[:5])

    param_buf[:2] = bytearray([0x77, 0x00])
    self.set_params(0xC0, param_mv[:2])

    param_buf[:2] = bytearray([0x11, 0x0C])
    self.set_params(0xC1, param_mv[:2])

    param_buf[:2] = bytearray([0x07, 0x02])
    self.set_params(0xC2, param_mv[:2])

    param_buf[0] = 0x30
    self.set_params(0xCC, param_mv[:1])

    param_buf[:16] = bytearray([
        0x06, 0xCF, 0x14, 0x0C, 0x0F, 0x03, 0x00, 0x0A, 
        0x07, 0x1B, 0x03, 0x12, 0x10, 0x25, 0x36, 0x1E])
    self.set_params(0xB0, param_mv[:16])

    param_buf[:16] = bytearray([
        0x0C, 0xD4, 0x18, 0x0C, 0x0E, 0x06, 0x03, 0x06, 
        0x08, 0x23, 0x06, 0x12, 0x10, 0x30, 0x2F, 0x1F])
    self.set_params(0xB1, param_mv[:16])

    param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, 0x11])
    self.set_params(0xFF, param_mv[:5])

    param_buf[0] = 0x73
    self.set_params(0xB0, param_mv[:1])

    param_buf[0] = 0x7C
    self.set_params(0xB1, param_mv[:1])

    param_buf[0] = 0x83
    self.set_params(0xB2, param_mv[:1])

    param_buf[0] = 0x80
    self.set_params(0xB3, param_mv[:1])

    param_buf[0] = 0x49
    self.set_params(0xB5, param_mv[:1])

    param_buf[0] = 0x87
    self.set_params(0xB7, param_mv[:1])

    param_buf[0] = 0x33
    self.set_params(0xB8, param_mv[:1])

    param_buf[:2] = bytearray([0x10, 0x1F])
    self.set_params(0xB9, param_mv[:2])

    param_buf[0] = 0x03
    self.set_params(0xBB, param_mv[:1])

    param_buf[0] = 0x08
    self.set_params(0xC1, param_mv[:1])

    param_buf[0] = 0x08
    self.set_params(0xC2, param_mv[:1])

    param_buf[0] = 0x88
    self.set_params(0xD0, param_mv[:1])

    param_buf[:6] = bytearray([0x00, 0x00, 0x02, 0x00, 0x00, 0x0C])
    self.set_params(0xE0, param_mv[:6])

    param_buf[:11] = bytearray([
        0x05, 0x96, 0x07, 0x96, 0x06, 0x96, 
        0x08, 0x96, 0x00, 0x44, 0x44])
    self.set_params(0xE1, param_mv[:11])

    param_buf[:12] = bytearray([
        0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 
        0x02, 0x00, 0x00, 0x00, 0x02, 0x00])
    self.set_params(0xE2, param_mv[:12])

    param_buf[:4] = bytearray([0x00, 0x00, 0x33, 0x33])
    self.set_params(0xE3, param_mv[:4])

    param_buf[:2] = bytearray([0x44, 0x44])
    self.set_params(0xE4, param_mv[:2])

    param_buf[:16] = bytearray([
        0x0D, 0xD4, 0x28, 0x8C, 0x0F, 0xD6, 0x28, 0x8C, 
        0x09, 0xD0, 0x28, 0x8C, 0x0B, 0xD2, 0x28, 0x8C])
    self.set_params(0xE5, param_mv[:16])

    param_buf[:4] = bytearray([0x00, 0x00, 0x33, 0x33])
    self.set_params(0xE6, param_mv[:4])

    param_buf[:2] = bytearray([0x44, 0x44])
    self.set_params(0xE7, param_mv[:2])

    param_buf[:16] = bytearray([
        0x0E, 0xD5, 0x28, 0x8C, 0x10, 0xD7, 0x28, 0x8C, 
        0x0A, 0xD1, 0x28, 0x8C, 0x0C, 0xD3, 0x28, 0x8C])
    self.set_params(0xE8, param_mv[:16])

    param_buf[:6] = bytearray([0x00, 0x01, 0xE4, 0xE4, 0x44, 0x00])
    self.set_params(0xEB, param_mv[:6])

    param_buf[:16] = bytearray([
        0xF3, 0xC1, 0xBA, 0x0F, 0x66, 0x77, 0x44, 0x55, 
        0x55, 0x44, 0x77, 0x66, 0xF0, 0xAB, 0x1C, 0x3F])
    self.set_params(0xED, param_mv[:16])

    param_buf[:6] = bytearray([0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F])
    self.set_params(0xEF, param_mv[:6])

    param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, 0x13])
    self.set_params(0xFF, param_mv[:5])

    param_buf[:2] = bytearray([0x00, 0x0E])
    self.set_params(0xE8, param_mv[:2])

    self.set_params(0x11)

    time.sleep_ms(120)  # NOQA

    param_buf[:2] = bytearray([0x00, 0x0C])
    self.set_params(0xE8, param_mv[:2])

    time.sleep_ms(10)  # NOQA

    param_buf[:2] = bytearray([0x40, 0x00])
    self.set_params(0xE8, param_mv[:2])

    param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, 0x00])
    self.set_params(0xFF, param_mv[:5])

    param_buf[0] = 0x00
    self.set_params(0x36, param_mv[:1])

    param_buf[0] = 0x66
    self.set_params(0x3A, param_mv[:1])

    self.set_params(0x29)

    time.sleep_ms(20)  # NOQA

    param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, 0x10])
    self.set_params(0xFF, param_mv[:5])

    param_buf[:2] = bytearray([0x00, 0x00])
    self.set_params(0xE5, param_mv[:2])
