# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import time


_SLPOUT = const(0x11)
_MADCTL = const(0x36)
_DISPON = const(0x29)


def init(self):
    param_buf = bytearray(12)
    param_mv = memoryview(param_buf)

    time.sleep_ms(20)  # NOQA
    self.set_params(_SLPOUT)

    param_buf[0] = 0x07
    param_buf[1] = 0x42
    param_buf[2] = 0x18
    self.set_params(0xD0, param_mv[:3])

    param_buf[0] = 0x00
    param_buf[1] = 0x07
    param_buf[2] = 0x10
    self.set_params(0xD1, param_mv[:3])

    param_buf[0] = 0x01
    param_buf[1] = 0x02
    self.set_params(0xD2, param_mv[:2])

    param_buf[0] = 0x10
    param_buf[1] = 0x3B
    param_buf[2] = 0x00
    param_buf[3] = 0x02
    param_buf[4] = 0x11
    self.set_params(0xC0, param_mv[:5])

    param_buf[0] = 0x03
    self.set_params(0xC5, param_mv[:1])

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
    self.set_params(0xC8, param_mv[:12])

    param_buf[0] = 0x0A
    self.set_params(_MADCTL, param_mv[:1])

    param_buf[0] = 0x55
    self.set_params(0x3A, param_mv[:1])

    time.sleep_ms(120)  # NOQA
    self.set_params(_DISPON)

    time.sleep_ms(25)  # NOQA
