# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import time
import lvgl as lv


_COLMOD = const(0x3A)


def init(self):
    param_buf = self._param_buf
    param_mv = self._param_mv

    color_size = lv.color_format_get_size(self._color_space)

    if color_size == 2:  # NOQA
        colmod = 0x55
    elif color_size == 3:
        colmod = 0x77
    else:
        raise RuntimeError(
            'SPD2010 IC only supports '
            'lv.COLOR_FORMAT.RGB888 and lv.COLOR_FORMAT.RGB565'
        )

    param_buf[:3] = bytearray([0x20, 0x10, 0x00])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = colmod
    self.set_params(_COLMOD, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x10])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x11
    self.set_params(0x0C, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x10, param_mv[:1])

    param_buf[0] = 0x11
    self.set_params(0x11, param_mv[:1])

    param_buf[0] = 0x42
    self.set_params(0x15, param_mv[:1])

    param_buf[0] = 0x11
    self.set_params(0x16, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x1A, param_mv[:1])

    param_buf[0] = 0x11
    self.set_params(0x1B, param_mv[:1])

    param_buf[0] = 0x80
    self.set_params(0x61, param_mv[:1])

    param_buf[0] = 0x80
    self.set_params(0x62, param_mv[:1])

    param_buf[0] = 0x44
    self.set_params(0x54, param_mv[:1])

    param_buf[0] = 0x88
    self.set_params(0x58, param_mv[:1])

    param_buf[0] = 0xcc
    self.set_params(0x5C, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x10])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x80
    self.set_params(0x20, param_mv[:1])

    param_buf[0] = 0x81
    self.set_params(0x21, param_mv[:1])

    param_buf[0] = 0x31
    self.set_params(0x22, param_mv[:1])

    param_buf[0] = 0x20
    self.set_params(0x23, param_mv[:1])

    param_buf[0] = 0x11
    self.set_params(0x24, param_mv[:1])

    param_buf[0] = 0x11
    self.set_params(0x25, param_mv[:1])

    param_buf[0] = 0x12
    self.set_params(0x26, param_mv[:1])

    param_buf[0] = 0x12
    self.set_params(0x27, param_mv[:1])

    param_buf[0] = 0x80
    self.set_params(0x30, param_mv[:1])

    param_buf[0] = 0x81
    self.set_params(0x31, param_mv[:1])

    param_buf[0] = 0x31
    self.set_params(0x32, param_mv[:1])

    param_buf[0] = 0x20
    self.set_params(0x33, param_mv[:1])

    param_buf[0] = 0x11
    self.set_params(0x34, param_mv[:1])

    param_buf[0] = 0x11
    self.set_params(0x35, param_mv[:1])

    param_buf[0] = 0x12
    self.set_params(0x36, param_mv[:1])

    param_buf[0] = 0x12
    self.set_params(0x37, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x10])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x11
    self.set_params(0x41, param_mv[:1])

    param_buf[0] = 0x22
    self.set_params(0x42, param_mv[:1])

    param_buf[0] = 0x33
    self.set_params(0x43, param_mv[:1])

    param_buf[0] = 0x11
    self.set_params(0x49, param_mv[:1])

    param_buf[0] = 0x22
    self.set_params(0x4A, param_mv[:1])

    param_buf[0] = 0x33
    self.set_params(0x4B, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x15])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x00
    self.set_params(0x00, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x01, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x02, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x03, param_mv[:1])

    param_buf[0] = 0x10
    self.set_params(0x04, param_mv[:1])

    param_buf[0] = 0x0C
    self.set_params(0x05, param_mv[:1])

    param_buf[0] = 0x23
    self.set_params(0x06, param_mv[:1])

    param_buf[0] = 0x22
    self.set_params(0x07, param_mv[:1])

    param_buf[0] = 0x21
    self.set_params(0x08, param_mv[:1])

    param_buf[0] = 0x20
    self.set_params(0x09, param_mv[:1])

    param_buf[0] = 0x33
    self.set_params(0x0A, param_mv[:1])

    param_buf[0] = 0x32
    self.set_params(0x0B, param_mv[:1])

    param_buf[0] = 0x34
    self.set_params(0x0C, param_mv[:1])

    param_buf[0] = 0x35
    self.set_params(0x0D, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x0E, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x0F, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x20, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x21, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x22, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x23, param_mv[:1])

    param_buf[0] = 0x0C
    self.set_params(0x24, param_mv[:1])

    param_buf[0] = 0x10
    self.set_params(0x25, param_mv[:1])

    param_buf[0] = 0x20
    self.set_params(0x26, param_mv[:1])

    param_buf[0] = 0x21
    self.set_params(0x27, param_mv[:1])

    param_buf[0] = 0x22
    self.set_params(0x28, param_mv[:1])

    param_buf[0] = 0x23
    self.set_params(0x29, param_mv[:1])

    param_buf[0] = 0x33
    self.set_params(0x2A, param_mv[:1])

    param_buf[0] = 0x32
    self.set_params(0x2B, param_mv[:1])

    param_buf[0] = 0x34
    self.set_params(0x2C, param_mv[:1])

    param_buf[0] = 0x35
    self.set_params(0x2D, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x2E, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x2F, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x16])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x00
    self.set_params(0x00, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x01, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x02, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x03, param_mv[:1])

    param_buf[0] = 0x08
    self.set_params(0x04, param_mv[:1])

    param_buf[0] = 0x04
    self.set_params(0x05, param_mv[:1])

    param_buf[0] = 0x19
    self.set_params(0x06, param_mv[:1])

    param_buf[0] = 0x18
    self.set_params(0x07, param_mv[:1])

    param_buf[0] = 0x17
    self.set_params(0x08, param_mv[:1])

    param_buf[0] = 0x16
    self.set_params(0x09, param_mv[:1])

    param_buf[0] = 0x33
    self.set_params(0x0A, param_mv[:1])

    param_buf[0] = 0x32
    self.set_params(0x0B, param_mv[:1])

    param_buf[0] = 0x34
    self.set_params(0x0C, param_mv[:1])

    param_buf[0] = 0x35
    self.set_params(0x0D, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x0E, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x0F, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x20, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x21, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x22, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x23, param_mv[:1])

    param_buf[0] = 0x04
    self.set_params(0x24, param_mv[:1])

    param_buf[0] = 0x08
    self.set_params(0x25, param_mv[:1])

    param_buf[0] = 0x16
    self.set_params(0x26, param_mv[:1])

    param_buf[0] = 0x17
    self.set_params(0x27, param_mv[:1])

    param_buf[0] = 0x18
    self.set_params(0x28, param_mv[:1])

    param_buf[0] = 0x19
    self.set_params(0x29, param_mv[:1])

    param_buf[0] = 0x33
    self.set_params(0x2A, param_mv[:1])

    param_buf[0] = 0x32
    self.set_params(0x2B, param_mv[:1])

    param_buf[0] = 0x34
    self.set_params(0x2C, param_mv[:1])

    param_buf[0] = 0x35
    self.set_params(0x2D, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x2E, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x2F, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x12])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x99
    self.set_params(0x00, param_mv[:1])

    param_buf[0] = 0x28
    self.set_params(0x2A, param_mv[:1])

    param_buf[0] = 0x0f
    self.set_params(0x2B, param_mv[:1])

    param_buf[0] = 0x16
    self.set_params(0x2C, param_mv[:1])

    param_buf[0] = 0x28
    self.set_params(0x2D, param_mv[:1])

    param_buf[0] = 0x0f
    self.set_params(0x2E, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0xA0])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0xdc
    self.set_params(0x08, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x45])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x9C
    self.set_params(0x01, param_mv[:1])

    param_buf[0] = 0x9C
    self.set_params(0x03, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x42])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x2c
    self.set_params(0x05, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x11])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x01
    self.set_params(0x50, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x00])
    self.set_params(0xFF, param_mv[:3])

    param_buf[:4] = bytearray([0x00, 0x00, 0x01, 0x9B])
    self.set_params(0x2A, param_mv[:4])

    param_buf[:4] = bytearray([0x00, 0x00, 0x01, 0x9B])
    self.set_params(0x2B, param_mv[:4])

    param_buf[:3] = bytearray([0x20, 0x10, 0x40])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x00
    self.set_params(0x86, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x00])
    self.set_params(0xFF, param_mv[:3])

    param_buf[:3] = bytearray([0x20, 0x10, 0x12])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x66
    self.set_params(0x0D, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x17])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x3c
    self.set_params(0x39, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x31])
    self.set_params(0xff, param_mv[:3])

    param_buf[0] = 0x03
    self.set_params(0x38, param_mv[:1])

    param_buf[0] = 0xf0
    self.set_params(0x39, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x36, param_mv[:1])

    param_buf[0] = 0xe8
    self.set_params(0x37, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x34, param_mv[:1])

    param_buf[0] = 0xCF
    self.set_params(0x35, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x32, param_mv[:1])

    param_buf[0] = 0xBA
    self.set_params(0x33, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x30, param_mv[:1])

    param_buf[0] = 0xA2
    self.set_params(0x31, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x2e, param_mv[:1])

    param_buf[0] = 0x95
    self.set_params(0x2f, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x2c, param_mv[:1])

    param_buf[0] = 0x7e
    self.set_params(0x2d, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x2a, param_mv[:1])

    param_buf[0] = 0x62
    self.set_params(0x2b, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x28, param_mv[:1])

    param_buf[0] = 0x44
    self.set_params(0x29, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x26, param_mv[:1])

    param_buf[0] = 0xfc
    self.set_params(0x27, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x24, param_mv[:1])

    param_buf[0] = 0xd0
    self.set_params(0x25, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x22, param_mv[:1])

    param_buf[0] = 0x98
    self.set_params(0x23, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x20, param_mv[:1])

    param_buf[0] = 0x6f
    self.set_params(0x21, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x1e, param_mv[:1])

    param_buf[0] = 0x32
    self.set_params(0x1f, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x1c, param_mv[:1])

    param_buf[0] = 0xf6
    self.set_params(0x1d, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x1a, param_mv[:1])

    param_buf[0] = 0xb8
    self.set_params(0x1b, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x18, param_mv[:1])

    param_buf[0] = 0x6E
    self.set_params(0x19, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x16, param_mv[:1])

    param_buf[0] = 0x41
    self.set_params(0x17, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x14, param_mv[:1])

    param_buf[0] = 0xfd
    self.set_params(0x15, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x12, param_mv[:1])

    param_buf[0] = 0xCf
    self.set_params(0x13, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x10, param_mv[:1])

    param_buf[0] = 0x98
    self.set_params(0x11, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x0e, param_mv[:1])

    param_buf[0] = 0x89
    self.set_params(0x0f, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x0c, param_mv[:1])

    param_buf[0] = 0x79
    self.set_params(0x0d, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x0a, param_mv[:1])

    param_buf[0] = 0x67
    self.set_params(0x0b, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x08, param_mv[:1])

    param_buf[0] = 0x55
    self.set_params(0x09, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x06, param_mv[:1])

    param_buf[0] = 0x3F
    self.set_params(0x07, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x04, param_mv[:1])

    param_buf[0] = 0x28
    self.set_params(0x05, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x02, param_mv[:1])

    param_buf[0] = 0x0E
    self.set_params(0x03, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x00])
    self.set_params(0xff, param_mv[:3])

    param_buf[:3] = bytearray([0x20, 0x10, 0x32])
    self.set_params(0xff, param_mv[:3])

    param_buf[0] = 0x03
    self.set_params(0x38, param_mv[:1])

    param_buf[0] = 0xf0
    self.set_params(0x39, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x36, param_mv[:1])

    param_buf[0] = 0xe8
    self.set_params(0x37, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x34, param_mv[:1])

    param_buf[0] = 0xCF
    self.set_params(0x35, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x32, param_mv[:1])

    param_buf[0] = 0xBA
    self.set_params(0x33, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x30, param_mv[:1])

    param_buf[0] = 0xA2
    self.set_params(0x31, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x2e, param_mv[:1])

    param_buf[0] = 0x95
    self.set_params(0x2f, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x2c, param_mv[:1])

    param_buf[0] = 0x7e
    self.set_params(0x2d, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x2a, param_mv[:1])

    param_buf[0] = 0x62
    self.set_params(0x2b, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x28, param_mv[:1])

    param_buf[0] = 0x44
    self.set_params(0x29, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x26, param_mv[:1])

    param_buf[0] = 0xfc
    self.set_params(0x27, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x24, param_mv[:1])

    param_buf[0] = 0xd0
    self.set_params(0x25, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x22, param_mv[:1])

    param_buf[0] = 0x98
    self.set_params(0x23, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x20, param_mv[:1])

    param_buf[0] = 0x6f
    self.set_params(0x21, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x1e, param_mv[:1])

    param_buf[0] = 0x32
    self.set_params(0x1f, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x1c, param_mv[:1])

    param_buf[0] = 0xf6
    self.set_params(0x1d, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x1a, param_mv[:1])

    param_buf[0] = 0xb8
    self.set_params(0x1b, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x18, param_mv[:1])

    param_buf[0] = 0x6E
    self.set_params(0x19, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x16, param_mv[:1])

    param_buf[0] = 0x41
    self.set_params(0x17, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x14, param_mv[:1])

    param_buf[0] = 0xfd
    self.set_params(0x15, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x12, param_mv[:1])

    param_buf[0] = 0xCf
    self.set_params(0x13, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x10, param_mv[:1])

    param_buf[0] = 0x98
    self.set_params(0x11, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x0e, param_mv[:1])

    param_buf[0] = 0x89
    self.set_params(0x0f, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x0c, param_mv[:1])

    param_buf[0] = 0x79
    self.set_params(0x0d, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x0a, param_mv[:1])

    param_buf[0] = 0x67
    self.set_params(0x0b, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x08, param_mv[:1])

    param_buf[0] = 0x55
    self.set_params(0x09, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x06, param_mv[:1])

    param_buf[0] = 0x3F
    self.set_params(0x07, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x04, param_mv[:1])

    param_buf[0] = 0x28
    self.set_params(0x05, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x02, param_mv[:1])

    param_buf[0] = 0x0E
    self.set_params(0x03, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x00])
    self.set_params(0xff, param_mv[:3])

    param_buf[:3] = bytearray([0x20, 0x10, 0x11])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x01
    self.set_params(0x60, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x65, param_mv[:1])

    param_buf[0] = 0x38
    self.set_params(0x66, param_mv[:1])

    param_buf[0] = 0x04
    self.set_params(0x67, param_mv[:1])

    param_buf[0] = 0x34
    self.set_params(0x68, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x69, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x61, param_mv[:1])

    param_buf[0] = 0x38
    self.set_params(0x62, param_mv[:1])

    param_buf[0] = 0x04
    self.set_params(0x63, param_mv[:1])

    param_buf[0] = 0x34
    self.set_params(0x64, param_mv[:1])

    param_buf[0] = 0x11
    self.set_params(0x0A, param_mv[:1])

    param_buf[0] = 0x20
    self.set_params(0x0B, param_mv[:1])

    param_buf[0] = 0x20
    self.set_params(0x0c, param_mv[:1])

    param_buf[0] = 0x06
    self.set_params(0x55, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x42])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x3D
    self.set_params(0x05, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x06, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x00])
    self.set_params(0xFF, param_mv[:3])

    param_buf[:3] = bytearray([0x20, 0x10, 0x12])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0xDC
    self.set_params(0x1F, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x17])
    self.set_params(0xff, param_mv[:3])

    param_buf[0] = 0xAA
    self.set_params(0x11, param_mv[:1])

    param_buf[0] = 0x12
    self.set_params(0x16, param_mv[:1])

    param_buf[0] = 0xC3
    self.set_params(0x0B, param_mv[:1])

    param_buf[0] = 0x0E
    self.set_params(0x10, param_mv[:1])

    param_buf[0] = 0xAA
    self.set_params(0x14, param_mv[:1])

    param_buf[0] = 0xA0
    self.set_params(0x18, param_mv[:1])

    param_buf[0] = 0x80
    self.set_params(0x1A, param_mv[:1])

    param_buf[0] = 0x80
    self.set_params(0x1F, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x11])
    self.set_params(0xff, param_mv[:3])

    param_buf[0] = 0xEE
    self.set_params(0x30, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x12])
    self.set_params(0xff, param_mv[:3])

    param_buf[0] = 0x0F
    self.set_params(0x15, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x2D])
    self.set_params(0xff, param_mv[:3])

    param_buf[0] = 0x3E
    self.set_params(0x01, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x40])
    self.set_params(0xff, param_mv[:3])

    param_buf[0] = 0xC4
    self.set_params(0x83, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x12])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0xCC
    self.set_params(0x00, param_mv[:1])

    param_buf[0] = 0xA0
    self.set_params(0x36, param_mv[:1])

    param_buf[0] = 0x2D
    self.set_params(0x2A, param_mv[:1])

    param_buf[0] = 0x1e
    self.set_params(0x2B, param_mv[:1])

    param_buf[0] = 0x26
    self.set_params(0x2C, param_mv[:1])

    param_buf[0] = 0x2D
    self.set_params(0x2D, param_mv[:1])

    param_buf[0] = 0x1e
    self.set_params(0x2E, param_mv[:1])

    param_buf[0] = 0xE6
    self.set_params(0x1F, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0xA0])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0xE6
    self.set_params(0x08, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x12])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x0F
    self.set_params(0x10, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x18])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x01
    self.set_params(0x01, param_mv[:1])

    param_buf[0] = 0x1E
    self.set_params(0x00, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x43])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x04
    self.set_params(0x03, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x18])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x01
    self.set_params(0x3A, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x50])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x08
    self.set_params(0x05, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x00])
    self.set_params(0xFF, param_mv[:3])

    param_buf[:3] = bytearray([0x20, 0x10, 0x50])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0xA6
    self.set_params(0x00, param_mv[:1])

    param_buf[0] = 0xA6
    self.set_params(0x01, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x00])
    self.set_params(0xFF, param_mv[:3])

    param_buf[:3] = bytearray([0x20, 0x10, 0x50])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x55
    self.set_params(0x08, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x00])
    self.set_params(0xFF, param_mv[:3])

    param_buf[:3] = bytearray([0x20, 0x10, 0x10])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x43
    self.set_params(0x0B, param_mv[:1])

    param_buf[0] = 0x12
    self.set_params(0x0C, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x10, param_mv[:1])

    param_buf[0] = 0x12
    self.set_params(0x11, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x15, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x16, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x1A, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x1B, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x61, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x62, param_mv[:1])

    param_buf[0] = 0x11
    self.set_params(0x51, param_mv[:1])

    param_buf[0] = 0x55
    self.set_params(0x55, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x58, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x5C, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x10])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x81
    self.set_params(0x20, param_mv[:1])

    param_buf[0] = 0x82
    self.set_params(0x21, param_mv[:1])

    param_buf[0] = 0x72
    self.set_params(0x22, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x30, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x31, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x32, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x10])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x44
    self.set_params(0x44, param_mv[:1])

    param_buf[0] = 0x55
    self.set_params(0x45, param_mv[:1])

    param_buf[0] = 0x66
    self.set_params(0x46, param_mv[:1])

    param_buf[0] = 0x77
    self.set_params(0x47, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x49, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x4A, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x4B, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x17])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x00
    self.set_params(0x37, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x15])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x08
    self.set_params(0x04, param_mv[:1])

    param_buf[0] = 0x04
    self.set_params(0x05, param_mv[:1])

    param_buf[0] = 0x1C
    self.set_params(0x06, param_mv[:1])

    param_buf[0] = 0x1A
    self.set_params(0x07, param_mv[:1])

    param_buf[0] = 0x18
    self.set_params(0x08, param_mv[:1])

    param_buf[0] = 0x16
    self.set_params(0x09, param_mv[:1])

    param_buf[0] = 0x05
    self.set_params(0x24, param_mv[:1])

    param_buf[0] = 0x09
    self.set_params(0x25, param_mv[:1])

    param_buf[0] = 0x17
    self.set_params(0x26, param_mv[:1])

    param_buf[0] = 0x19
    self.set_params(0x27, param_mv[:1])

    param_buf[0] = 0x1B
    self.set_params(0x28, param_mv[:1])

    param_buf[0] = 0x1D
    self.set_params(0x29, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x16])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x09
    self.set_params(0x04, param_mv[:1])

    param_buf[0] = 0x05
    self.set_params(0x05, param_mv[:1])

    param_buf[0] = 0x1D
    self.set_params(0x06, param_mv[:1])

    param_buf[0] = 0x1B
    self.set_params(0x07, param_mv[:1])

    param_buf[0] = 0x19
    self.set_params(0x08, param_mv[:1])

    param_buf[0] = 0x17
    self.set_params(0x09, param_mv[:1])

    param_buf[0] = 0x04
    self.set_params(0x24, param_mv[:1])

    param_buf[0] = 0x08
    self.set_params(0x25, param_mv[:1])

    param_buf[0] = 0x16
    self.set_params(0x26, param_mv[:1])

    param_buf[0] = 0x18
    self.set_params(0x27, param_mv[:1])

    param_buf[0] = 0x1A
    self.set_params(0x28, param_mv[:1])

    param_buf[0] = 0x1C
    self.set_params(0x29, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x18])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x02
    self.set_params(0x1F, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x11])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x99
    self.set_params(0x15, param_mv[:1])

    param_buf[0] = 0x99
    self.set_params(0x16, param_mv[:1])

    param_buf[0] = 0x88
    self.set_params(0x1C, param_mv[:1])

    param_buf[0] = 0x88
    self.set_params(0x1D, param_mv[:1])

    param_buf[0] = 0x88
    self.set_params(0x1E, param_mv[:1])

    param_buf[0] = 0xf0
    self.set_params(0x13, param_mv[:1])

    param_buf[0] = 0x34
    self.set_params(0x14, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x12])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x89
    self.set_params(0x12, param_mv[:1])

    param_buf[0] = 0x06
    self.set_params(0x06, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x18, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x11])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x00
    self.set_params(0x0A, param_mv[:1])

    param_buf[0] = 0xF0
    self.set_params(0x0B, param_mv[:1])

    param_buf[0] = 0xF0
    self.set_params(0x0c, param_mv[:1])

    param_buf[0] = 0x10
    self.set_params(0x6A, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x00])
    self.set_params(0xFF, param_mv[:3])

    param_buf[:3] = bytearray([0x20, 0x10, 0x11])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x70
    self.set_params(0x08, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x09, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x00])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x00
    self.set_params(0x35, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x12])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x70
    self.set_params(0x21, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x2D])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x00
    self.set_params(0x02, param_mv[:1])

    param_buf[:3] = bytearray([0x20, 0x10, 0x00])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x00
    self.set_params(0x11, param_mv[:1])

    time.sleep_ms(120)  # NOQA
