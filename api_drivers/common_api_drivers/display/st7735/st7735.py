# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import display_driver_framework


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR

_MADCTL_MY = const(0x80)
_MADCTL_MX = const(0x40)
_MADCTL_MV = const(0x20)


TYPE_B = 1
TYPE_R_RED = 2
TYPE_R_GREEN = 3
TYPE_R_BLUE = 4


class ST7735(display_driver_framework.DisplayDriver):

    def init(self, type):  # NOQA
        if type == 1:
            ST7735._ORIENTATION_TABLE = (
                0x0,
                _MADCTL_MX | _MADCTL_MV,
                _MADCTL_MY | _MADCTL_MX,
                _MADCTL_MY | _MADCTL_MV
            )
        elif type == 2:
            ST7735._ORIENTATION_TABLE = (
                0x0,
                _MADCTL_MX | _MADCTL_MV,
                _MADCTL_MY | _MADCTL_MX,
                _MADCTL_MY | _MADCTL_MV
            )
        else:
            ST7735._ORIENTATION_TABLE = (
                0x0,
                _MADCTL_MV | _MADCTL_MY,
                _MADCTL_MY | _MADCTL_MX,
                _MADCTL_MV | _MADCTL_MX
            )
        display_driver_framework.DisplayDriver.init(self, type)  # NOQA
