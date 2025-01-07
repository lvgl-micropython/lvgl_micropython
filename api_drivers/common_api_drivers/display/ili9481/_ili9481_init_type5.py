# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA


_WRNVRAM = const(0xD0)
_NVPROTKEY = const(0xD1)
_RDNVSTAT = const(0xD2)
_PWRCTL1 = const(0xC0)
_VCOMCTL1 = const(0xC5)
_GAMMASET = const(0xC8)
_MADCTL = const(0x36)
_COLMOD = const(0x3A)
_CASET = const(0x2A)
_PASET = const(0x2B)
_DISPON = const(0x29)
_SLPOUT = const(0x11)
_IFMODE = const(0xB0)
_INVON = const(0x21)
_IFACECTRL = const(0xC6)
_PUMPRATIOCTRL = const(0xF7)
_IFACESET = const(0xB3)
_DISPMODE = const(0xB4)
_SWRESET = const(0x01)


def init(self):
    param_buf = bytearray(12)
    param_mv = memoryview(param_buf)

    self.set_params(_SWRESET)
    time.sleep_ms(280)  # NOQA
    self.set_params(_SLPOUT)

    param_buf[0] = 0x07
    param_buf[1] = 0x41
    param_buf[2] = 0x1D
    self.set_params(_WRNVRAM, param_mv[:3])

    param_buf[0] = 0x00
    param_buf[1] = 0x1C
    param_buf[2] = 0x1F
    self.set_params(_NVPROTKEY, param_mv[:3])

    param_buf[0] = 0x01
    param_buf[1] = 0x11
    self.set_params(_RDNVSTAT, param_mv[:2])

    param_buf[0] = 0x10
    param_buf[1] = 0x3B
    param_buf[2] = 0x00
    param_buf[3] = 0x02
    param_buf[4] = 0x11
    self.set_params(_PWRCTL1, param_mv[:5])

    param_buf[0] = 0x03
    self.set_params(_VCOMCTL1, param_mv[:1])

    param_buf[0] = 0x83
    self.set_params(_IFACECTRL, param_mv[:1])

    param_buf[0] = 0x00
    param_buf[1] = 0x26
    param_buf[2] = 0x21
    param_buf[3] = 0x00
    param_buf[4] = 0x00
    param_buf[5] = 0x1F
    param_buf[6] = 0x65
    param_buf[7] = 0x23
    param_buf[8] = 0x77
    param_buf[9] = 0x00
    param_buf[10] = 0x0F
    param_buf[11] = 0x00
    self.set_params(_GAMMASET, param_mv[:12])

    param_buf[0] = 0x00
    self.set_params(_IFMODE, param_mv[:1])

    param_buf[0] = 0xA0
    self.set_params(0xE4, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0xF0, param_mv[:1])

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

    if not isinstance(self._data_bus, lcd_bus.I80Bus):
        self.set_params(_INVON)

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    param_buf[2] = 0x01
    param_buf[3] = 0x3F
    self.set_params(_CASET, param_mv[:4])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    param_buf[2] = 0x01
    param_buf[3] = 0xDF
    self.set_params(_PASET, param_mv[:4])

    time.sleep_ms(120)  # NOQA
    self.set_params(_DISPON)
    time.sleep_ms(25)  # NOQA
