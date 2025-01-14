# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA


def init(self):
    param_buf = bytearray(8)
    param_mv = memoryview(param_buf)

    param_buf[0] = 0x23
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
    param_buf[1] = 0x33
    param_buf[2] = 0x33
    self.set_params(0xE6, param_mv[:3])

    param_buf[0] = 0x20
    param_buf[1] = 0x00
    param_buf[2] = 799 >> 8
    param_buf[3] = 799 & 0xFF
    param_buf[4] = 479 >> 8
    param_buf[5] = 479 & 0xFF
    param_buf[6] = 0x00
    self.set_params(0xB0, param_mv[:7])

    param_buf[0] = 0x04
    param_buf[1] = 0x1F
    param_buf[2] = 0x00
    param_buf[3] = 0xD2
    param_buf[4] = 0x00
    param_buf[5] = 0x00
    param_buf[6] = 0x00
    param_buf[7] = 0x00
    self.set_params(0xB4, param_mv[:8])

    param_buf[0] = 0x02
    param_buf[1] = 0x0C
    param_buf[2] = 0x00
    param_buf[3] = 0x22
    param_buf[4] = 0x00
    param_buf[5] = 0x00
    param_buf[6] = 0x00
    self.set_params(0xB6, param_mv[:7])

    param_buf[0] = 0x0F
    param_buf[1] = 0x01
    self.set_params(0xB8, param_mv[:2])

    param_buf[0] = 0x01
    self.set_params(0xBA, param_mv[:1])

    param_buf[0] = 0x21  # | TFT_MAD_COLOR_ORDER
    self.set_params(0x36, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xF0, param_mv[:1])

    param_buf[0] = 0x40
    param_buf[1] = 0x80
    param_buf[2] = 0x40
    param_buf[3] = 0x01
    self.set_params(0xBC, param_mv[:4])

    time.sleep_ms(10)  # NOQA
    self.set_params(0x29)

    param_buf[0] = 0x06
    param_buf[1] = 0x80
    param_buf[2] = 0x01
    param_buf[3] = 0xF0
    param_buf[4] = 0x00
    param_buf[5] = 0x00
    self.set_params(0xBE, param_mv[:6])

    param_buf[0] = 0x0D
    self.set_params(0xD0, param_mv[:1])
