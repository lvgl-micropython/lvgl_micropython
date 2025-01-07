# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA


_PWRCTRL1 = const(0x10)
_PWRCTRL2 = const(0x11)
_PWRCTRL3 = const(0x12)
_PWRCTRL4 = const(0x13)
_PWRCTRL5 = const(0x14)
_ACDRVCTRL = const(0x02)
_DISPCTRL1 = const(0x07)
_BLKPERDCTRL1 = const(0x08)
_FRAMECYLCTRL = const(0x0B)
_IFACECTRL = const(0x0C)
_OSCCTRL = const(0x0F)
_VCIREC = const(0x15)
_RAMADDRSET1 = const(0x20)
_RAMADDRSET2 = const(0x21)
_GSCANCTRL = const(0x30)
_VSCRLCTRL1 = const(0x31)
_VSCRLCTRL2 = const(0x32)
_VSCRLCTRL3 = const(0x33)
_PARDRVPOS1 = const(0x34)
_PARDRVPOS2 = const(0x35)
_HWINADDR1 = const(0x36)
_HWINADDR2 = const(0x37)
_VWINADDR1 = const(0x38)
_VWINADDR2 = const(0x39)
_GAMMACTRL1 = const(0x50)
_GAMMACTRL2 = const(0x51)
_GAMMACTRL3 = const(0x52)
_GAMMACTRL4 = const(0x53)
_GAMMACTRL5 = const(0x54)
_GAMMACTRL6 = const(0x55)
_GAMMACTRL7 = const(0x56)
_GAMMACTRL8 = const(0x57)
_GAMMACTRL9 = const(0x58)
_GAMMACTRL10 = const(0x59)


def init(self):
    param_buf = bytearray(2)
    param_mv = memoryview(param_buf)

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_PWRCTRL1, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_PWRCTRL2, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_PWRCTRL3, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_PWRCTRL4, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_PWRCTRL5, param_mv[:2])

    time.sleep_ms(40)  # NOQA

    param_buf[0] = 0x00
    param_buf[1] = 0x18
    self.set_params(_PWRCTRL2, param_mv[:2])

    param_buf[0] = 0x61
    param_buf[1] = 0x21
    self.set_params(_PWRCTRL3, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x6F
    self.set_params(_PWRCTRL4, param_mv[:2])

    param_buf[0] = 0x49
    param_buf[1] = 0x5F
    self.set_params(_PWRCTRL5, param_mv[:2])

    param_buf[0] = 0x08
    param_buf[1] = 0x00
    self.set_params(_PWRCTRL1, param_mv[:2])

    time.sleep_ms(10)  # NOQA

    param_buf[0] = 0x10
    param_buf[1] = 0x3B
    self.set_params(_PWRCTRL2, param_mv[:2])

    time.sleep_ms(50)  # NOQA

    param_buf[0] = 0x01
    param_buf[1] = 0x00
    self.set_params(_ACDRVCTRL, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_DISPCTRL1, param_mv[:2])

    param_buf[0] = 0x08
    param_buf[1] = 0x08
    self.set_params(_BLKPERDCTRL1, param_mv[:2])

    param_buf[0] = 0x11
    param_buf[1] = 0x00
    self.set_params(_FRAMECYLCTRL, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_IFACECTRL, param_mv[:2])

    param_buf[0] = 0x0D
    param_buf[1] = 0x01
    self.set_params(_OSCCTRL, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x20
    self.set_params(_VCIREC, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_RAMADDRSET1, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_RAMADDRSET2, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_GSCANCTRL, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0xDB
    self.set_params(_VSCRLCTRL1, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_VSCRLCTRL2, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_VSCRLCTRL3, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0xDB
    self.set_params(_PARDRVPOS1, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_PARDRVPOS2, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0xAF
    self.set_params(_HWINADDR1, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_HWINADDR2, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0xDB
    self.set_params(_VWINADDR1, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_VWINADDR2, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_GAMMACTRL1, param_mv[:2])

    param_buf[0] = 0x08
    param_buf[1] = 0x08
    self.set_params(_GAMMACTRL2, param_mv[:2])

    param_buf[0] = 0x08
    param_buf[1] = 0x0A
    self.set_params(_GAMMACTRL3, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x0A
    self.set_params(_GAMMACTRL4, param_mv[:2])

    param_buf[0] = 0x0A
    param_buf[1] = 0x08
    self.set_params(_GAMMACTRL5, param_mv[:2])

    param_buf[0] = 0x08
    param_buf[1] = 0x08
    self.set_params(_GAMMACTRL6, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x00
    self.set_params(_GAMMACTRL7, param_mv[:2])

    param_buf[0] = 0x0A
    param_buf[1] = 0x00
    self.set_params(_GAMMACTRL8, param_mv[:2])

    param_buf[0] = 0x07
    param_buf[1] = 0x10
    self.set_params(_GAMMACTRL9, param_mv[:2])

    param_buf[0] = 0x07
    param_buf[1] = 0x10
    self.set_params(_GAMMACTRL10, param_mv[:2])

    param_buf[0] = 0x00
    param_buf[1] = 0x12
    self.set_params(_DISPCTRL1, param_mv[:2])

    time.sleep_ms(50)  # NOQA

    param_buf[0] = 0x10
    param_buf[1] = 0x17
    self.set_params(_DISPCTRL1, param_mv[:2])
