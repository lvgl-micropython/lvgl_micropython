# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA


_SWRESET = const(0x01)
_SLPOUT = const(0x11)
_IFMODE = const(0xB0)
_WRNVRAM = const(0xD0)
_NVPROTKEY = const(0xD1)
_RDNVSTAT = const(0xD2)
_PWRCTL1 = const(0xC0)
_VCOMCTL1 = const(0xC5)
_GAMMASET = const(0xC8)
_MADCTL = const(0x36)
_COLMOD = const(0x3A)
_CASET = const(0x2A)
_RASET = const(0x2B)
_DISPON = const(0x29)


def init(self):
    param_buf = bytearray(15)
    param_mv = memoryview(param_buf)

    time.sleep_ms(120)  # NOQA

    self.set_params(_SWRESET)

    time.sleep_ms(120)  # NOQA

    self.set_params(_SLPOUT)

    param_buf[0] = 0x00
    self.set_params(_IFMODE, param_mv[:1])

    param_buf[0] = 0x07
    param_buf[1] = 0x42
    param_buf[2] = 0x18
    self.set_params(_WRNVRAM, param_mv[:3])

    param_buf[0] = 0x00
    param_buf[1] = 0x07
    param_buf[2] = 0x10
    self.set_params(_NVPROTKEY, param_mv[:3])

    param_buf[0] = 0x01
    param_buf[1] = 0x02
    self.set_params(_RDNVSTAT, param_mv[:2])

    param_buf[0] = 0x12
    param_buf[1] = 0x3B
    param_buf[2] = 0x00
    param_buf[3] = 0x02
    param_buf[4] = 0x11
    self.set_params(_PWRCTL1, param_mv[:5])

    param_buf[0] = 0x03
    self.set_params(_VCOMCTL1, param_mv[:1])

    param_buf[0] = 0x00
    param_buf[1] = 0x32
    param_buf[2] = 0x36
    param_buf[3] = 0x45
    param_buf[4] = 0x06
    param_buf[5] = 0x16
    param_buf[6] = 0x37
    param_buf[7] = 0x75
    param_buf[8] = 0x77
    param_buf[9] = 0x54
    param_buf[10] = 0x0C
    param_buf[11] = 0x00
    self.set_params(_GAMMASET, param_mv[:12])

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

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    param_buf[2] = 0x01
    param_buf[3] = 0x3F
    self.set_params(_CASET, param_mv[:4])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    param_buf[2] = 0x01
    param_buf[3] = 0xDF
    self.set_params(_RASET, param_mv[:4])

    time.sleep_ms(120)  # NOQA
    self.set_params(_DISPON)
    time.sleep_ms(25)  # NOQA
