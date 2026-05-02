# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA
import lvgl as lv

_SLPOUT = const(0x11)
_INVON = const(0x21)
_DISPON = const(0x29)
_TEON = const(0x35)
_MADCTL = const(0x36)
_COLMOD = const(0x3A)
_DFC = const(0xB6)
_VREG1ACTL = const(0xC3)
_VREG1BCTL = const(0xC4)
_VREG2ACTL = const(0xC9)
_REG_ENA2 = const(0xEF)
_SET_GAMMA1 = const(0xF0)
_SET_GAMMA2 = const(0xF1)
_SET_GAMMA3 = const(0xF2)
_SET_GAMMA4 = const(0xF3)
_REG_ENA1 = const(0xFE)
_MEMWR = const(0x2C)


def init(self):
    param_buf = bytearray(32)
    param_mv = memoryview(param_buf)

    self.set_params(_REG_ENA1)
    self.set_params(_REG_ENA2)

    param_buf[0] = 0xFF
    self.set_params(0x80, param_mv[:1])
    self.set_params(0x81, param_mv[:1])
    self.set_params(0x82, param_mv[:1])
    self.set_params(0x83, param_mv[:1])
    self.set_params(0x84, param_mv[:1])
    self.set_params(0x85, param_mv[:1])
    self.set_params(0x86, param_mv[:1])
    self.set_params(0x87, param_mv[:1])
    self.set_params(0x88, param_mv[:1])
    self.set_params(0x89, param_mv[:1])
    self.set_params(0x8A, param_mv[:1])
    self.set_params(0x8B, param_mv[:1])
    self.set_params(0x8C, param_mv[:1])
    self.set_params(0x8D, param_mv[:1])
    self.set_params(0x8E, param_mv[:1])
    self.set_params(0x8F, param_mv[:1])

    color_size = lv.color_format_get_size(self._color_space)
    if color_size == 2:  # NOQA
        pixel_format = 0x55
    elif color_size == 3:
        pixel_format = 0x77
    else:
        raise RuntimeError(
            f'{self.__class__.__name__} IC only supports '
            'lv.COLOR_FORMAT.RGB565 or lv.COLOR_FORMAT.RGB888'
        )

    param_buf[0] = pixel_format
    self.set_params(_COLMOD, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0xEC, param_mv[:1])

    param_buf[:7] = bytearray([0x02, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00])
    self.set_params(0x74, param_mv[:7])

    param_buf[0] = 0x3E
    self.set_params(0x98, param_mv[:1])
    self.set_params(0x99, param_mv[:1])

    param_buf[:2] = bytearray([0x0D, 0x0D])
    self.set_params(0xB5, param_mv[:2])

    param_buf[:4] = bytearray([0x38, 0x0F, 0x79, 0x67])
    self.set_params(0x60, param_mv[:4])
    param_buf[:4] = bytearray([0x38, 0x11, 0x79, 0x67])
    self.set_params(0x61, param_mv[:4])
    param_buf[:6] = bytearray([0x38, 0x17, 0x71, 0x5F, 0x79, 0x67])
    self.set_params(0x64, param_mv[:6])
    param_buf[:6] = bytearray([0x38, 0x13, 0x71, 0x5B, 0x79, 0x67])
    self.set_params(0x65, param_mv[:6])

    param_buf[:2] = bytearray([0x00, 0x00])
    self.set_params(0x6A, param_mv[:2])
    param_buf[:7] = bytearray([0x22, 0x02, 0x22, 0x02, 0x22, 0x22, 0x50])
    self.set_params(0x6C, param_mv[:7])

    param_buf[:32] = bytearray([
        0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0x0F, 0x0F, 0x0D, 0x0D, 0x0B, 0x0B, 0x09, 0x09, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x0A, 0x0C, 0x0C, 0x0E, 0x0E, 0x10, 0x10, 0x00, 0x00, 0x02, 0x02, 0x04, 0x04])
    self.set_params(0x6E, param_mv[:32])

    param_buf[0] = 0x01
    self.set_params(0xBF, param_mv[:1])
    param_buf[0] = 0x40
    self.set_params(0xF9, param_mv[:1])

    param_buf[0] = 0x3B
    self.set_params(0x9B, param_mv[:1])
    param_buf[:3] = bytearray([0x33, 0x7F, 0x00])
    self.set_params(0x93, param_mv[:3])

    param_buf[0] = 0x30
    self.set_params(0x7E, param_mv[:1])

    param_buf[:6] = bytearray([0x0D, 0x02, 0x08, 0x0D, 0x02, 0x08])
    self.set_params(0x70, param_mv[:6])
    param_buf[:3] = bytearray([0x0D, 0x02, 0x08])
    self.set_params(0x71, param_mv[:3])

    param_buf[:2] = bytearray([0x0E, 0x09])
    self.set_params(0x91, param_mv[:2])

    param_buf[0] = 0x1F
    self.set_params(_VREG1ACTL, param_mv[:1])
    self.set_params(_VREG1BCTL, param_mv[:1])
    self.set_params(_VREG2ACTL, param_mv[:1])

    param_buf[:6] = bytearray([0x53, 0x15, 0x0A, 0x04, 0x00, 0x3E])
    self.set_params(_SET_GAMMA1, param_mv[:6])

    param_buf[:6] = bytearray([0x53, 0x15, 0x0A, 0x04, 0x00, 0x3A])
    self.set_params(_SET_GAMMA2, param_mv[:6])

    param_buf[:6] = bytearray([0x56, 0xA8, 0x7F, 0x33, 0x34, 0x5F])
    self.set_params(_SET_GAMMA3, param_mv[:6])

    param_buf[:6] = bytearray([0x52, 0xA4, 0x7F, 0x33, 0x34, 0xDF])
    self.set_params(_SET_GAMMA4, param_mv[:6])

    param_buf[0] = (
        self._madctl(
            self._color_byte_order,
            self._ORIENTATION_TABLE  # NOQA
        )
    )
    self.set_params(_MADCTL, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xB0, param_mv[:1])

    param_buf[:2] = bytearray([0x00, 0x00])
    self.set_params(0xB1, param_mv[:2])

    param_buf[0] = 0x00
    self.set_params(0xB4, param_mv[:1])

    self.set_params(_SLPOUT)
    time.sleep_ms(120)  # NOQA
    self.set_params(_DISPON)
    self.set_params(_MEMWR)
    time.sleep_ms(20)  # NOQA
