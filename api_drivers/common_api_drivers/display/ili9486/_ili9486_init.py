# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA


_SLPOUT = const(0x11)
_SWRESET = const(0x01)
_INVOFF = const(0x20)
_INVON = const(0x21)
_PWRCTL1 = const(0xC0)
_PWRCTL2 = const(0xC1)
_PWRCTL3 = const(0xC2)
_COLMOD = const(0x3A)
_VCOMCTL1 = const(0xC5)
_PGC = const(0xE0)
_NGC = const(0xE1)
_MADCTL = const(0x36)
_DISPON = const(0x29)


def init(self):
    param_buf = bytearray(15)
    param_mv = memoryview(param_buf)

    time.sleep_ms(120)  # NOQA
    self.set_params(_SWRESET)

    time.sleep_ms(120)  # NOQA
    self.set_params(_SLPOUT)

    param_buf[0] = 0x0E
    param_buf[1] = 0x0E
    self.set_params(_PWRCTL1, param_mv[:2])

    param_buf[0] = 0x41
    param_buf[1] = 0x00
    self.set_params(_PWRCTL2, param_mv[:2])

    param_buf[0] = 0x55
    self.set_params(_PWRCTL3, param_mv[:1])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    param_buf[2] = 0x00
    param_buf[3] = 0x00
    self.set_params(_VCOMCTL1, param_mv[:4])

    param_buf[0] = 0x0F
    param_buf[1] = 0x1F
    param_buf[2] = 0x1C
    param_buf[3] = 0x0C
    param_buf[4] = 0x0F
    param_buf[5] = 0x08
    param_buf[6] = 0x48
    param_buf[7] = 0x98
    param_buf[8] = 0x37
    param_buf[9] = 0x0A
    param_buf[10] = 0x13
    param_buf[11] = 0x04
    param_buf[12] = 0x11
    param_buf[13] = 0x0D
    param_buf[14] = 0x00
    self.set_params(_PGC, param_mv[:15])

    param_buf[0] = 0x0F
    param_buf[1] = 0x32
    param_buf[2] = 0x2E
    param_buf[3] = 0x0B
    param_buf[4] = 0x0D
    param_buf[5] = 0x05
    param_buf[6] = 0x47
    param_buf[7] = 0x75
    param_buf[8] = 0x37
    param_buf[9] = 0x06
    param_buf[10] = 0x10
    param_buf[11] = 0x03
    param_buf[12] = 0x24
    param_buf[13] = 0x20
    param_buf[14] = 0x00
    self.set_params(_NGC, param_mv[:15])

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

    param_buf[0] = (
        self._madctl(
            self._color_byte_order,
            self._ORIENTATION_TABLE  # NOQA
        )
    )
    self.set_params(_MADCTL, param_mv[:1])

    if isinstance(self._data_bus, lcd_bus.I80Bus):
        self.set_params(_INVOFF)
    else:
        self.set_params(_INVON)

    time.sleep_ms(150)  # NOQA
    self.set_params(_DISPON)
