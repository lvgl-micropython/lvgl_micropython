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
    buf = bytearray(16)
    mv = memoryview(buf)

    self._reset()

    self._writecommand(_SWRESET)
    time.sleep_ms(50)  # NOQA

    self._writecommand(_SLPOUT)
    time.sleep_ms(255)  # NOQA

    buf[:3] = bytearray([0x01, 0x2C, 0x2D])
    self._writecommand(_FRMCTR1, mv[:3])
    time.sleep_us(10)  # NOQA

    self._writecommand(_FRMCTR2, mv[:3])
    time.sleep_us(10)  # NOQA

    self._writecommand(_FRMCTR3, mv[:3])
    time.sleep_us(10)  # NOQA

    buf[0] = 0x07
    self._writecommand(_INVCTR, mv[:1])

    buf[:3] = bytearray([0xA2, 0x02, 0x84])
    self._writecommand(_PWCTR1, mv[:3])
    time.sleep_us(10)  # NOQA

    buf[0] = 0xC5
    self._writecommand(_PWCTR2, mv[:1])

    buf[:2] = bytearray([0x0A, 0x00])
    self._writecommand(_PWCTR3, mv[:2])

    buf[:2] = bytearray([0x8A, 0x2A])
    self._writecommand(_PWCTR4, mv[:2])

    buf[:2] = bytearray([0x8A, 0xEE])
    self._writecommand(_PWCTR5, mv[:2])

    buf[0] = 0x0E
    self._writecommand(_VMCTR1, mv[:1])
    time.sleep_us(10)  # NOQA

    self._writecommand(_MADCTL, mv[:1])
    buf[0] = 0xC8

    buf[:16] = bytearray([0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d,
                          0x29, 0x25, 0x2b, 0x39, 0x00, 0x01, 0x03, 0x10])
    self._writecommand(_GMCTRP1, mv[:16])

    buf[:16] = bytearray([0x03, 0x1d, 0x07, 0x06, 0x2e, 0x2c, 0x29, 0x2d,
                          0x2e, 0x2e, 0x37, 0x3f, 0x00, 0x00, 0x02, 0x10])
    self._writecommand(_GMCTRN1, mv[:16])
    time.sleep_us(10)  # NOQA

    color_size = lv.color_format_get_size(self._color_space)
    if color_size == 2:  # NOQA
        pixel_format = 0x55
    elif color_size == 3:
        pixel_format = 0x77
    else:
        raise RuntimeError(
            'ST7796 IC only supports '
            'lv.COLOR_FORMAT.RGB565 or lv.COLOR_FORMAT.RGB888'
        )

    buf[0] = pixel_format
    self.set_params(_COLMOD, mv[:1])

    time.sleep_us(10)  # NOQA

    self._writecommand(_NORON)
    time.sleep_us(10)  # NOQA

    self._writecommand(_DISPON)
    time.sleep_us(500)  # NOQA
