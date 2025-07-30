# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Final, ClassVar
import display_driver_framework


STATE_HIGH: Final[int]
STATE_LOW: Final[int]
STATE_PWM: Final[int]

BYTE_ORDER_RGB: Final[int]
BYTE_ORDER_BGR: Final[int]


class GC9107(display_driver_framework.DisplayDriver):

    _ORIENTATION_TABLE: ClassVar[tuple[int, int, int, int]]

    def _set_memory_location(self, x1: int, y1: int, x2: int, y2: int) -> int:
        ...
