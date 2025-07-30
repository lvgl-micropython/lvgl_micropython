# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Final, ClassVar
import display_driver_framework


STATE_HIGH: Final[int]
STATE_LOW: Final[int]
STATE_PWM: Final[int]

BYTE_ORDER_RGB: Final[int]
BYTE_ORDER_BGR: Final[int]

TYPE_A: Final[int]


class HX8369(display_driver_framework.DisplayDriver):

    _ORIENTATION_TABLE: ClassVar[tuple[int, int, int, int]]

    def reset(self) -> None:
        ...

    def set_brightness(self, value: float | int) -> None:
        ...
