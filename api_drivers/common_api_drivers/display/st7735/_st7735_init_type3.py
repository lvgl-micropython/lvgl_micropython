# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA


_NOP = const(0x00)
_SWRESET = const(0x01)
_RDDID = const(0x04)
_RDDST = const(0x09)

_SLPIN = const(0x10)
_SLPOUT = const(0x11)
_PTLON = const(0x12)
_NORON = const(0x13)

_DISPOFF = const(0x28)
_DISPON = const(0x29)

_PTLAR = const(0x30)
_COLMOD = const(0x3A)
_MADCTL = const(0x36)

_FRMCTR1 = const(0xB1)
_FRMCTR2 = const(0xB2)
_FRMCTR3 = const(0xB3)
_INVCTR = const(0xB4)
_DISSET5 = const(0xB6)

_PWCTR1 = const(0xC0)
_PWCTR2 = const(0xC1)
_PWCTR3 = const(0xC2)
_PWCTR4 = const(0xC3)
_PWCTR5 = const(0xC4)
_VMCTR1 = const(0xC5)

_PWCTR6 = const(0xFC)

_GMCTRP1 = const(0xE0)
_GMCTRN1 = const(0xE1)


def init(self):
    param_buf = bytearray(16)
    param_mv = memoryview(param_buf)
    self.set_params(_SWRESET)
    time.sleep_ms(150)  # NOQA

    self.set_params(_SLPOUT)
    time.sleep_ms(255)  # NOQA

    param_buf[0] = 0x01
    param_buf[1] = 0x2C
    param_buf[2] = 0x2D
    self.set_params(_FRMCTR1, param_mv[:3])

    param_buf[0] = 0x01
    param_buf[1] = 0x2C
    param_buf[2] = 0x2D
    self.set_params(_FRMCTR2, param_mv[:3])

    param_buf[0] = 0x01
    param_buf[1] = 0x2C
    param_buf[2] = 0x2D
    param_buf[3] = 0x01
    param_buf[4] = 0x2C
    param_buf[5] = 0x2D

    self.set_params(_FRMCTR3, param_mv[:6])

    param_buf[0] = 0x07
    self.set_params(_INVCTR, param_mv[:1])

    param_buf[0] = 0xA2
    param_buf[1] = 0x02
    param_buf[2] = 0x84
    self.set_params(_PWCTR1, param_mv[:3])

    param_buf[0] = 0xC5
    self.set_params(_PWCTR2, param_mv[:1])

    param_buf[0] = 0x0A
    param_buf[1] = 0x00
    self.set_params(_PWCTR3, param_mv[:2])

    param_buf[0] = 0x8A
    param_buf[1] = 0x2A
    self.set_params(_PWCTR4, param_mv[:2])

    param_buf[0] = 0x8A
    param_buf[1] = 0xEE
    self.set_params(_PWCTR5, param_mv[:2])

    param_buf[0] = 0x0E
    self.set_params(_VMCTR1, param_mv[:1])

    self.set_params(self._INVOFF)

    param_buf[0] = (
        self._madctl(
            self._color_byte_order,
            self._ORIENTATION_TABLE  # NOQA
        )
    )
    self.set_params(_MADCTL, param_mv[:1])

    color_size = lv.color_format_get_size(self._color_space)
    if color_size == 2:  # NOQA
        pixel_format = 0x05
    elif color_size == 3:
        pixel_format = 0x07
    else:
        raise RuntimeError(
            'ST7796 IC only supports '
            'lv.COLOR_FORMAT.RGB565 or lv.COLOR_FORMAT.RGB888'
        )

    param_buf[0] = pixel_format
    self.set_params(_COLMOD, param_mv[:1])

    param_buf[:16] = bytearray(
        [
            0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d,
            0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10
        ]
    )
    self.set_params(_GMCTRP1, param_mv[:16])

    param_buf[:16] = bytearray(
        [
            0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D,
            0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10
        ]
    )
    self.set_params(_GMCTRN1, param_mv[:16])

    self.set_params(_NORON)
    time.sleep_ms(10)  # NOQA

    self.set_params(_DISPON)
    time.sleep_ms(100)  # NOQA
