# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Final, ClassVar
import display_driver_framework
import lcd_bus
import lvgl as lv  # NOQA

STATE_HIGH: Final[int]
STATE_LOW: Final[int]
STATE_PWM: Final[int]

BYTE_ORDER_RGB: Final[int]
BYTE_ORDER_BGR: Final[int]


class SSD1306(display_driver_framework.DisplayDriver):
    _pages: int

    def __init__(
        self,
        data_bus: lcd_bus.SPIBus | lcd_bus.I2CBus,
        display_width: int,
        display_height: int,
        frame_buffer1: memoryview | None = None,
        frame_buffer2: memoryview | None = None,
        reset_pin=None,
        reset_state: int = STATE_HIGH,
        power_pin=None,
        backlight_pin=None,
        backlight_on_state: int = STATE_HIGH,
        offset_x: int = 0,
        offset_y: int = 0,
        color_space: int = lv.COLOR_FORMAT.RGB888,  # NOQA
        rgb565_byte_swap: bool = False
    ):
        ...

    def set_constrast(self, value: int) -> None:
        ...

    def set_color_inversion(self, value: bool) -> None:
        ...

    def get_power(self) -> bool:
        ...

    def set_power(self, value: bool) -> None:
        ...

    def _flush_cb(self, _, area, color_p) -> None:
        ...
