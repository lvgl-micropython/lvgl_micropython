# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA


_SLPOUT = const(0x11)
_DISPON = const(0x29)
_CASET = const(0x2A)
_PASET = const(0x2B)
_MADCTL = const(0x36)
_COLMOD = const(0x3A)
_WRDISBV = const(0x51)   # display brightness
_SETPANEL = const(0xC0)
_SETOSC = const(0xC5)
_SETGAMMA = const(0xC8)
_SETPOWER = const(0xD0)
_SETVCOM = const(0xD1)
_SETNORPOW = const(0xD2)


def init(self):
    param_buf = bytearray(12)
    param_mv = memoryview(param_buf)

    self.set_params(_SLPOUT)

    time.sleep_ms(20)  # NOQA

    param_buf[:3] = bytearray([0x07, 0x42, 0x18])
    self.set_params(_SETPOWER, param_mv[:3])

    param_buf[:3] = bytearray([0x00, 0x07, 0x10])
    self.set_params(_SETVCOM, param_mv[:3])

    param_buf[:2] = bytearray([0x01, 0x02])
    self.set_params(_SETNORPOW, param_mv[:2])

    param_buf[:5] = bytearray([0x10, 0x3B, 0x00, 0x02, 0x11])
    self.set_params(_SETPANEL, param_mv[:5])

    param_buf[0] = 0x08
    self.set_params(_SETOSC, param_mv[:1])

    param_buf[:12] = bytearray([
        0x00, 0x32, 0x36, 0x45, 0x06, 0x16, 0x37, 0x75, 0x77, 0x54, 0x0C, 0x00])
    self.set_params(_SETGAMMA, param_mv[:12])

    param_buf[0] = (
        self._madctl(
            self._color_byte_order,
            self._ORIENTATION_TABLE  # NOQA
        )
    )
    self.set_params(_MADCTL, param_mv[:1])

    color_size = lv.color_format_get_size(self._color_space)
    if color_size == 2:  # NOQA
        pixel_format = 0x55
    else:
        raise RuntimeError(
            f'{self.__class__.__name__} IC only supports '
            'lv.COLOR_FORMAT.RGB565'
        )

    param_buf[0] = pixel_format
    self.set_params(_COLMOD, param_mv[:1])

    param_buf[:4] = bytearray([0x00, 0x00, 0x01, 0x3F])
    self.set_params(_CASET, param_mv[:4])

    param_buf[3] = 0xDF
    self.set_params(_PASET, param_mv[:4])

    time.sleep_ms(120)  # NOQA
    self.set_params(_DISPON)
    time.sleep_ms(25)  # NOQA
