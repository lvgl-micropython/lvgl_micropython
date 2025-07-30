# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import ClassVar, Final
import display_driver_framework


STATE_HIGH: Final[int]
STATE_LOW: Final[int]
STATE_PWM: Final[int]

BYTE_ORDER_RGB: Final[int]
BYTE_ORDER_BGR: Final[int]


class RM68120(display_driver_framework.DisplayDriver):
    _INVOFF: ClassVar[int]
    _INVON: ClassVar[int]

    def _set_memory_location(self, x1: int, y1: int, x2: int, y2: int) -> int:
        ...

    def _on_size_change(self, _) -> None:
        ...
