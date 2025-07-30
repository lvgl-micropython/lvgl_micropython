# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import ClassVar, Final
import display_driver_framework
import lvgl as lv


STATE_HIGH: Final[int]
STATE_LOW: Final[int]
STATE_PWM: Final[int]

BYTE_ORDER_RGB: Final[int]
BYTE_ORDER_BGR: Final[int]


class RM67162(display_driver_framework.DisplayDriver):
    _ORIENTATION_TABLE: ClassVar[tuple[int, int, int, int]]

    __brightness: int
    __sunlight: int
    __ambient: int
    __high_brightness: int

    def __init__(
        self,
        data_bus,
        display_width: int,
        display_height: int,
        frame_buffer1: memoryview | None = None,
        frame_buffer2: memoryview | None = None,
        reset_pin=None,
        reset_state: int = STATE_HIGH,
        power_pin=None,
        power_on_state: int = STATE_HIGH,
        backlight_pin=None,
        backlight_on_state: int = STATE_HIGH,
        offset_x: int = 0,
        offset_y: int = 0,
        color_byte_order: int = BYTE_ORDER_RGB,
        color_space: int = lv.COLOR_FORMAT.RGB888,  # NOQA
        rgb565_byte_swap: int = False
    ):
        ...

    def set_brightness(self, val: float | int) -> None:
        ...

    def get_brightness(self)-> float:
        ...

    def set_ambient_light_level(self, val: int | float) -> None:
        ...

    def get_amblent_light_level(self) -> float:
        ...

    def set_sunlight_enhance(self, val: int) -> None:
        ...

    def get_sunlight_enhance(self) -> int:
        ...

    def set_high_brightness(self, val: bool) -> None:
        ...

    def get_high_brightness(self) -> bool:
        ...
