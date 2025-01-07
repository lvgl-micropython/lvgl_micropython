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
_VREG1ACTL = 0xC3
_VREG1BCTL = 0xC4
_VREG2ACTL = 0xC9
_DOCA = const(0xE8)
_REG_ENA2 = const(0xEF)
_SET_GAMMA1 = const(0xF0)
_SET_GAMMA2 = const(0xF1)
_SET_GAMMA3 = const(0xF2)
_SET_GAMMA4 = const(0xF3)
_REG_ENA1 = const(0xFE)


def init(self):
    param_buf = bytearray(12)
    param_mv = memoryview(param_buf)

    self.set_params(_REG_ENA2)

    param_buf[0] = 0x14
    self.set_params(0xEB, param_mv[:1])

    self.set_params(_REG_ENA1)
    self.set_params(_REG_ENA2)

    param_buf[0] = 0x14
    self.set_params(0xEB, param_mv[:1])

    param_buf[0] = 0x40
    self.set_params(0x84, param_mv[:1])

    param_buf[0] = 0xFF
    self.set_params(0x85, param_mv[:1])
    self.set_params(0x86, param_mv[:1])
    self.set_params(0x87, param_mv[:1])

    param_buf[0] = 0x0A
    self.set_params(0x88, param_mv[:1])

    param_buf[0] = 0x21
    self.set_params(0x89, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x8A, param_mv[:1])

    param_buf[0] = 0x80
    self.set_params(0x8B, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0x8C, param_mv[:1])
    self.set_params(0x8D, param_mv[:1])

    param_buf[0] = 0xFF
    self.set_params(0x8E, param_mv[:1])
    self.set_params(0x8F, param_mv[:1])

    param_buf[:2] = bytearray([0x00, 0x20])
    self.set_params(_DFC, param_mv[:2])

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

    param_buf[0] = (
        self._madctl(
            self._color_byte_order,
            self._ORIENTATION_TABLE  # NOQA
        )
    )
    self.set_params(_MADCTL, param_mv[:1])

    param_buf[:5] = bytearray([0x08, 0x08, 0x08, 0x08, 0x08])
    self.set_params(0x90, param_mv[:5])

    param_buf[0] = 0x06
    self.set_params(0xBD, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xBC, param_mv[:1])

    param_buf[:3] = bytearray([0x60, 0x01, 0x04])
    self.set_params(0xFF, param_mv[:3])

    param_buf[0] = 0x13
    self.set_params(_VREG1ACTL, param_mv[:1])
    self.set_params(_VREG1BCTL, param_mv[:1])

    param_buf[0] = 0x22
    self.set_params(_VREG2ACTL, param_mv[:1])

    param_buf[0] = 0x11
    self.set_params(0xBE, param_mv[:1])

    param_buf[:2] = bytearray([0x10, 0x0E])
    self.set_params(0xE1, param_mv[:2])

    param_buf[:3] = bytearray([0x21, 0x0c, 0x02])
    self.set_params(0xDF, param_mv[:3])

    param_buf[:6] = bytearray([0x45, 0x09, 0x08, 0x08, 0x26, 0x2A])
    self.set_params(_SET_GAMMA1, param_mv[:6])

    param_buf[:6] = bytearray([0x43, 0x70, 0x72, 0x36, 0x37, 0x6F])
    self.set_params(_SET_GAMMA2, param_mv[:6])

    param_buf[:6] = bytearray([0x45, 0x09, 0x08, 0x08, 0x26, 0x2A])
    self.set_params(_SET_GAMMA3, param_mv[:6])

    param_buf[:6] = bytearray([0x43, 0x70, 0x72, 0x36, 0x37, 0x6F])
    self.set_params(_SET_GAMMA4, param_mv[:6])

    param_buf[:2] = bytearray([0x1B, 0x0B])
    self.set_params(0xED, param_mv[:2])

    param_buf[0] = 0x77
    self.set_params(0xAE, param_mv[:1])

    param_buf[0] = 0x63
    self.set_params(0xCD, param_mv[:1])

    param_buf[:9] = bytearray([
        0x07, 0x07, 0x04, 0x0E, 0x0F, 0x09, 0x07, 0x08, 0x03])
    self.set_params(0x70, param_mv[:9])

    param_buf[0] = 0x34
    self.set_params(_DOCA, param_mv[:1])

    param_buf[:12] = bytearray([
        0x18, 0x0D, 0x71, 0xED, 0x70, 0x70, 0x18, 0x0F, 0x71, 0xEF, 0x70, 0x70])
    self.set_params(0x62, param_mv[:12])

    param_buf[:12] = bytearray([
        0x18, 0x11, 0x71, 0xF1, 0x70, 0x70, 0x18, 0x13, 0x71, 0xF3, 0x70, 0x70])
    self.set_params(0x63, param_mv[:12])

    param_buf[:7] = bytearray([0x28, 0x29, 0xF1, 0x01, 0xF1, 0x00, 0x07])
    self.set_params(0x64, param_mv[:7])

    param_buf[:10] = bytearray([
        0x3C, 0x00, 0xCD, 0x67, 0x45, 0x45, 0x10, 0x00, 0x00, 0x00])
    self.set_params(0x66, param_mv[:10])

    param_buf[:10] = bytearray([
        0x00, 0x3C, 0x00, 0x00, 0x00, 0x01, 0x54, 0x10, 0x32, 0x98])
    self.set_params(0x67, param_mv[:10])

    param_buf[:7] = bytearray([0x10, 0x85, 0x80, 0x00, 0x00, 0x4E, 0x00])
    self.set_params(0x74, param_mv[:7])

    param_buf[:2] = bytearray([0x3e, 0x07])
    self.set_params(0x98, param_mv[:2])

    self.set_params(_TEON)
    self.set_params(_INVON)

    self.set_params(_SLPOUT)
    time.sleep_ms(120)  # NOQA
    self.set_params(_DISPON)
    time.sleep_ms(20)  # NOQA
