# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time


# HD458002C40 4.58" bar display
# 1 /* hync_polarity */, 10 /* hsync_front_porch */, 10 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
# 1 /* vsync_polarity */, 15 /* vsync_front_porch */, 2 /* vsync_pulse_width */, 17 /* vsync_back_porch */,
# 1 /* pclk_active_neg */, GFX_NOT_DEFINED /* prefer_speed */, false /* useBigEndian */, 0 /* de_idle_high */,
# 0 /* pclk_idle_high */, 80 /* col_offset */, 0 /* row_offset1 */, 8 /* col_offset2 */ );
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

    param_buf[:2] = bytearray([0x09, 0x08])
    self.set_params(0xC1, param_mv[:2])

    param_buf[:2] = bytearray([0x01, 0x02])
    self.set_params(0xC2, param_mv[:2])

    param_buf[0] = 0x02
    self.set_params(0xC3, param_mv[:1])

    param_buf[0] = 0x10
    self.set_params(0xCC, param_mv[:1])

    param_buf[:16] = bytearray([
        0x40, 0x14, 0x59, 0x10, 0x12, 0x08, 0x03, 0x09, 
        0x05, 0x1E, 0x05, 0x14, 0x10, 0x68, 0x33, 0x15])
    self.set_params(0xB0, param_mv[:16])

    param_buf[:16] = bytearray([
        0x40, 0x08, 0x53, 0x09, 0x11, 0x09, 0x02, 0x07, 
        0x09, 0x1A, 0x04, 0x12, 0x12, 0x64, 0x29, 0x29])
    self.set_params(0xB1, param_mv[:16])

    param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, 0x11])
    self.set_params(0xFF, param_mv[:5])

    param_buf[0] = 0x6D
    self.set_params(0xB0, param_mv[:1])

    param_buf[0] = 0x1D
    self.set_params(0xB1, param_mv[:1])

    param_buf[0] = 0x87
    self.set_params(0xB2, param_mv[:1])

    param_buf[0] = 0x80
    self.set_params(0xB3, param_mv[:1])

    param_buf[0] = 0x49
    self.set_params(0xB5, param_mv[:1])

    param_buf[0] = 0x85
    self.set_params(0xB7, param_mv[:1])

    param_buf[0] = 0x20
    self.set_params(0xB8, param_mv[:1])

    param_buf[0] = 0x78
    self.set_params(0xC1, param_mv[:1])

    param_buf[0] = 0x78
    self.set_params(0xC2, param_mv[:1])

    param_buf[0] = 0x88
    self.set_params(0xD0, param_mv[:1])

    param_buf[:3] = bytearray([0x00, 0x00, 0x02])
    self.set_params(0xE0, param_mv[:3])

    param_buf[:11] = bytearray([
        0x02, 0x8C, 0x00, 0x00, 0x03, 0x8C, 
        0x00, 0x00, 0x00, 0x33, 0x33])
    self.set_params(0xE1, param_mv[:11])

    param_buf[:13] = bytearray([
        0x33, 0x33, 0x33, 0x33, 0xC9, 0x3C, 0x00, 
        0x00, 0xCA, 0x3C, 0x00, 0x00, 0x00])
    self.set_params(0xE2, param_mv[:13])

    param_buf[:4] = bytearray([0x00, 0x00, 0x33, 0x33])
    self.set_params(0xE3, param_mv[:4])

    param_buf[:2] = bytearray([0x44, 0x44])
    self.set_params(0xE4, param_mv[:2])

    param_buf[:16] = bytearray([
        0x05, 0xCD, 0x82, 0x82, 0x01, 0xC9, 0x82, 0x82, 
        0x07, 0xCF, 0x82, 0x82, 0x03, 0xCB, 0x82, 0x82])
    self.set_params(0xE5, param_mv[:16])

    param_buf[:4] = bytearray([0x00, 0x00, 0x33, 0x33])
    self.set_params(0xE6, param_mv[:4])

    param_buf[:2] = bytearray([0x44, 0x44])
    self.set_params(0xE7, param_mv[:2])

    param_buf[:16] = bytearray([
        0x06, 0xCE, 0x82, 0x82, 0x02, 0xCA, 0x82, 0x82, 
        0x08, 0xD0, 0x82, 0x82, 0x04, 0xCC, 0x82, 0x82])
    self.set_params(0xE8, param_mv[:16])

    param_buf[:7] = bytearray([0x08, 0x01, 0xE4, 0xE4, 0x88, 0x00, 0x40])
    self.set_params(0xEB, param_mv[:7])

    param_buf[:3] = bytearray([0x00, 0x00, 0x00])
    self.set_params(0xEC, param_mv[:3])

    param_buf[:16] = bytearray([
        0xFF, 0xF0, 0x07, 0x65, 0x4F, 0xFC, 0xC2, 0x2F, 
        0xF2, 0x2C, 0xCF, 0xF4, 0x56, 0x70, 0x0F, 0xFF])
    self.set_params(0xED, param_mv[:16])

    param_buf[:6] = bytearray([0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F])
    self.set_params(0xEF, param_mv[:6])

    param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, 0x00])
    self.set_params(0xFF, param_mv[:5])

    self.set_params(0x11)

    time.sleep_ms(120)  # NOQA

    param_buf[0] = 0x00
    self.set_params(0x35, param_mv[:1])

    param_buf[0] = 0x66
    self.set_params(0x3A, param_mv[:1])

    self.set_params(0x29)
