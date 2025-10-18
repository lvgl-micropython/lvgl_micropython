# Copyright (c) 2024 - 2025 Kevin G. Schlosser
from micropython import const
import display_driver_framework


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


_MADCTL_MV = const(0x20)  # 0=Normal, 1=Row/column exchange
_MADCTL_MX = const(0x40)  # 0=Left to Right, 1=Right to Left
_MADCTL_MY = const(0x80)  # 0=Top to Bottom, 1=Bottom to Top


class ILI9343C(display_driver_framework.DisplayDriver):
    _ORIENTATION_TABLE = (0x00, _MADCTL_MV | _MADCTL_MX,
                          _MADCTL_MY | _MADCTL_MX, _MADCTL_MV | _MADCTL_MY)
