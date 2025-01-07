# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA


def init(self):
    param_buf = bytearray(17)
    param_mv = memoryview(param_buf)

    param_buf[0] = 0x5a
    param_buf[1] = 0x5a
    self.set_params(0xf0, param_mv[:2])

    param_buf[0] = 0x5a
    param_buf[1] = 0x5a
    self.set_params(0xfc, param_mv[:2])

    param_buf[0] = 0x01
    self.set_params(0x26, param_mv[:1])

    param_buf[:15] = bytearray(
        [
            0x02, 0x1f, 0x00, 0x10, 0x22, 0x30, 0x38,
            0x3A, 0x3A, 0x3A, 0x3A, 0x3A, 0x3d, 0x02, 0x01
        ]
    )
    self.set_params(0xfa, param_mv[:15])

    param_buf[:15] = bytearray(
        [
            0x21, 0x00, 0x02, 0x04, 0x07, 0x0a, 0x0b,
            0x0c, 0x0c, 0x16, 0x1e, 0x30, 0x3f, 0x01, 0x02
        ]
    )
    self.set_params(0xfb, param_mv[:15])

    param_buf[:11] = bytearray(
        [
            0x00, 0x00, 0x00, 0x17, 0x10,
            0x00, 0x01, 0x01, 0x00, 0x1f, 0x1f
        ]
    )
    self.set_params(0xfd, param_mv[:11])

    param_buf[:15] = bytearray(
        [
            0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x3f,
            0x07, 0x00, 0x3C, 0x36, 0x00, 0x3C, 0x36, 0x00
        ]
    )
    self.set_params(0xf4, param_mv[:15])

    param_buf[:13] = bytearray(
        [
            0x00, 0x70, 0x66, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x6d, 0x66, 0x06
        ]
    )
    self.set_params(0xf5, param_mv[:13])

    param_buf[:11] = bytearray(
        [
            0x02, 0x00, 0x3f, 0x00, 0x00,
            0x00, 0x02, 0x00, 0x06, 0x01, 0x00
        ]
    )
    self.set_params(0xf6, param_mv[:11])

    param_buf[:17] = bytearray(
        [
            0x00, 0x01, 0x03, 0x08, 0x08, 0x04, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x01, 0x00, 0x00, 0x04, 0x08, 0x08
        ]
    )
    self.set_params(0xf2, param_mv[:17])

    param_buf[0] = 0x11
    self.set_params(0xf8, param_mv[:1])

    param_buf[:4] = bytearray([0xc8, 0x20, 0x00, 0x00])
    self.set_params(0xf7, param_mv[:4])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(0xf3, param_mv[:2])

    self.set_params(0x11)
    time.sleep_ms(50)  # NOQA

    param_buf[0] = 0x00
    param_buf[1] = 0x01
    self.set_params(0xf3, param_mv[:2])

    time.sleep_ms(50)  # NOQA

    param_buf[0] = 0x00
    param_buf[1] = 0x03
    self.set_params(0xf3, param_mv[:2])

    time.sleep_ms(50)  # NOQA

    param_buf[0] = 0x00
    param_buf[1] = 0x07
    self.set_params(0xf3, param_mv[:2])

    time.sleep_ms(50)  # NOQA

    param_buf[0] = 0x00
    param_buf[1] = 0x0f
    self.set_params(0xf3, param_mv[:2])

    time.sleep_ms(50)  # NOQA

    param_buf[:15] = bytearray(
        [
            0x00, 0x04, 0x00, 0x00, 0x00, 0x3f, 0x3f,
            0x07, 0x00, 0x3C, 0x36, 0x00, 0x3C, 0x36, 0x00
        ]
    )
    self.set_params(0xf4, param_mv[:15])

    time.sleep_ms(50)  # NOQA

    param_buf[0] = 0x00
    param_buf[1] = 0x1f
    self.set_params(0xf3, param_mv[:2])

    time.sleep_ms(50)  # NOQA

    param_buf[0] = 0x00
    param_buf[1] = 0x7f
    self.set_params(0xf3, param_mv[:2])

    time.sleep_ms(50)  # NOQA

    param_buf[0] = 0x00
    param_buf[1] = 0xff
    self.set_params(0xf3, param_mv[:2])

    time.sleep_ms(50)  # NOQA

    param_buf[:11] = bytearray(
        [
            0x00, 0x00, 0x00, 0x17, 0x10,
            0x00, 0x00, 0x01, 0x00, 0x16, 0x16
        ]
    )
    self.set_params(0xfd, param_mv[:11])

    param_buf[:15] = bytearray(
        [
            0x00, 0x09, 0x00, 0x00, 0x00, 0x3f, 0x3f,
            0x07, 0x00, 0x3C, 0x36, 0x00, 0x3C, 0x36, 0x00
        ]
    )
    self.set_params(0xf4, param_mv[:15])

    param_buf[0] = 0xC8
    self.set_params(0x36, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x35, param_mv[:1])

    param_buf[0] = 0x05
    self.set_params(0x3a, param_mv[:1])

    time.sleep_ms(150)  # NOQA

    self.set_params(0x29)
    self.set_params(0x2c)
