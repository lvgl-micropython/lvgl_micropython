# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA


def init(self):
    param_buf = bytearray(8)
    param_mv = memoryview(param_buf)

    param_buf[0] = 0x1E
    param_buf[1] = 0x02
    param_buf[2] = 0x54
    self.set_params(0xE2, param_mv[:3])

    param_buf[0] = 0x01
    self.set_params(0xE0, param_mv[:1])

    time.sleep_ms(10)  # NOQA
    param_buf[0] = 0x03
    self.set_params(0xE0, param_mv[:1])

    time.sleep_ms(10)  # NOQA
    self.set_params(0x01)

    time.sleep_ms(100)  # NOQA
    param_buf[0] = 0x03
    param_buf[1] = 0xFF
    param_buf[2] = 0xFF
    self.set_params(0xE6, param_mv[:3])

    param_buf[0] = 0x20
    param_buf[1] = 0x00
    param_buf[2] = 0x03
    param_buf[3] = 0x1F
    param_buf[4] = 0x01
    param_buf[5] = 0xDF
    param_buf[6] = 0x00
    self.set_params(0xB0, param_mv[:7])

    param_buf[0] = 0x03
    param_buf[1] = 0xA0
    param_buf[2] = 0x00
    param_buf[3] = 0x2E
    param_buf[4] = 0x30
    param_buf[5] = 0x00
    param_buf[6] = 0x0F
    param_buf[7] = 0x00
    self.set_params(0xB4, param_mv[:8])

    param_buf[0] = 0x02
    param_buf[1] = 0x0D
    param_buf[2] = 0x00
    param_buf[3] = 0x10
    param_buf[4] = 0x10
    param_buf[5] = 0x00
    param_buf[6] = 0x08
    self.set_params(0xB6, param_mv[:7])

    param_buf[0] = 0x0F
    self.set_params(0xBA, param_mv[:1])

    param_buf[0] = 0x07
    param_buf[1] = 0x01
    self.set_params(0xB8, param_mv[:2])

    param_buf[0] = 0x21  # | TFT_MAD_COLOR_ORDER
    self.set_params(0x36, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xF0, param_mv[:1])

    time.sleep_ms(1)  # NOQA
    param_buf[0] = 0x0f
    param_buf[1] = 0x01
    self.set_params(0xB8, param_mv[:2])

    param_buf[0] = 0x01
    self.set_params(0xBA, param_mv[:1])

    param_buf[0] = 0
    param_buf[1] = 0
    param_buf[2] = (479 & 0xFF00) >> 8
    param_buf[3] = 479 & 0xFF
    self.set_params(0x2A, param_mv[:4])

    param_buf[0] = 0
    param_buf[1] = 0
    param_buf[2] = (799 & 0xFF00) >> 8
    param_buf[3] = 799 & 0xFF
    self.set_params(0x2B, param_mv[:4])

    self.set_params(0x2C)

    self.set_params(0x29)

    param_buf[0] = 0x06
    param_buf[1] = 0xf0
    param_buf[2] = 0x01
    param_buf[3] = 0xf0
    param_buf[4] = 0x00
    param_buf[5] = 0x00
    self.set_params(0xBE, param_mv[:6])

    param_buf[0] = 0x0d
    self.set_params(0xd0, param_mv[:1])

    self.set_params(0x2C)
