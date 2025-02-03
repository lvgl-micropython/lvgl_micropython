# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA
import display_driver_framework


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


_MADCTL_MH = const(0x04)  # Refresh 0=Left to Right, 1=Right to Left
_MADCTL_BGR = const(0x08)  # BGR color order
_MADCTL_ML = const(0x10)  # Refresh 0=Top to Bottom, 1=Bottom to Top
_MADCTL_MV = const(0x20)  # 0=Normal, 1=Row/column exchange
_MADCTL_MX = const(0x40)  # 0=Left to Right, 1=Right to Left
_MADCTL_MY = const(0x80)  # 0=Top to Bottom, 1=Bottom to Top


class GC9107(display_driver_framework.DisplayDriver):

    _ORIENTATION_TABLE = (
        0,
        _MADCTL_MX | _MADCTL_MV,
        _MADCTL_MY | _MADCTL_MX,
        _MADCTL_MY | _MADCTL_MV
    )

    def _set_memory_location(self, x1, y1, x2, y2):
        if self._rotation in (0, 2):
            x1 += 2
            x2 += 2
            y1 += 1
            y2 += 1
        else:
            x1 += 1
            x2 += 1
            y1 += 2
            y2 += 2

        return display_driver_framework.DisplayDriver(self, x1, y1, x2, y2)
