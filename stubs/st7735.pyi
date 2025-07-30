# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Final
import display_driver_framework


STATE_HIGH: Final[int]
STATE_LOW: Final[int]
STATE_PWM: Final[int]

BYTE_ORDER_RGB: Final[int]
BYTE_ORDER_BGR: Final[int]

TYPE_B: Final[int]
TYPE_R_RED: Final[int]
TYPE_R_GREEN: Final[int]
TYPE_R_BLUE: Final[int]


class ST7735(display_driver_framework.DisplayDriver):
    ...
