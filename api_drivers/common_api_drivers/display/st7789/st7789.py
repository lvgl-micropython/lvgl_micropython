# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import display_driver_framework
import lcd_bus
import lvgl as lv


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR

_MADCTL_MV = const(0x20)  # X, Y = Y, X
_MADCTL_MX = const(0x40)  # Mirror X
_MADCTL_MY = const(0x80)  # Mirror Y


class ST7789(display_driver_framework.DisplayDriver):
    _ORIENTATION_TABLE = (
        0x0,
        _MADCTL_MV,
        _MADCTL_MX | _MADCTL_MY,
        _MADCTL_MV | _MADCTL_MX | _MADCTL_MY
    )
