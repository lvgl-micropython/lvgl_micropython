# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA


_SWRESET = const(0x01)
_SLPOUT = const(0x11)
_COLMOD = const(0x3A)
_GAMSET = const(0x26)
_ENA3GAMMA = const(0xF2)
_PGC = const(0xE0)
_NGC = const(0xE1)
_FRMCTR1 = const(0xB1)
_DINVCTRL = const(0xB4)
_PWRCTL1 = const(0xC0)
_PWRCTL2 = const(0xC1)
_VCOMCTL1 = const(0xC5)
_VCOMCTL2 = const(0xC7)
_CASET = const(0x2A)
_RASET = const(0x2B)
_MADCTL = const(0x36)
_DISPON = const(0x29)


def init(self):
    param_buf = bytearray(15)
    param_mv = memoryview(param_buf)

    time.sleep_ms(120)  # NOQA

    self.set_params(_SWRESET)

    time.sleep_ms(120)  # NOQA

    self.set_params(_SLPOUT)
    time.sleep_ms(5)  # NOQA

    param_buf[0] = (
        self._madctl(
            self._color_byte_order,
            self._ORIENTATION_TABLE  # NOQA
        )
    )
    self.set_params(_MADCTL, param_mv[:1])

    color_size = lv.color_format_get_size(self._color_space)
    if color_size == 2:
        pixel_format = 0x55
    else:
        raise RuntimeError(
            f'{self.__class__.__name__} IC only supports '
            'lv.COLOR_FORMAT.RGB565'
        )

    param_buf[0] = pixel_format
    self.set_params(_COLMOD, param_mv[:1])

    param_buf[0] = 0x04
    self.set_params(_GAMSET, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(_ENA3GAMMA, param_mv[:1])

    param_buf[:15] = bytearray([
        0x3F, 0x25, 0x1C, 0x1E, 0x20, 0x12, 0x2A, 0x90,
        0x24, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00])
    self.set_params(_PGC, param_mv[:15])

    param_buf[:15] = bytearray([
        0x20, 0x20, 0x20, 0x20, 0x05, 0x00, 0x15, 0xA7,
        0x3D, 0x18, 0x25, 0x2A, 0x2B, 0x2B, 0x3A])
    self.set_params(_NGC, param_mv[:15])

    param_buf[:2] = bytearray([0x08, 0x08])
    self.set_params(_FRMCTR1, param_mv[:2])

    param_buf[0] = 0x07
    self.set_params(_DINVCTRL, param_mv[:1])

    param_buf[:2] = bytearray([0x0A, 0x02])
    self.set_params(_PWRCTL1, param_mv[:2])

    param_buf[0] = 0x02
    self.set_params(_PWRCTL2, param_mv[:1])

    param_buf[:2] = bytearray([0x50, 0x5B])
    self.set_params(_VCOMCTL1, param_mv[:2])

    param_buf[0] = 0x40
    self.set_params(_VCOMCTL2, param_mv[:1])

    param_buf[:4] = bytearray([0x00, 0x00, 0x00, 0x7F])
    self.set_params(_CASET, param_mv[:4])

    param_buf[:4] = bytearray([0x00, 0x00, 0x00, 0x9F])
    self.set_params(_RASET, param_mv[:4])

    time.sleep_ms(250)  # NOQA
    self.set_params(_DISPON)
    time.sleep_ms(25)  # NOQA
