# Copyright (c) 2024 - 2025 Kevin G. Schlosser


from typing import Final
import display_driver_framework
import rgb_display_framework  # NOQA
import lcd_bus

import lvgl as lv



STATE_HIGH: Final[int]
STATE_LOW: Final[int]
STATE_PWM: Final[int]

BYTE_ORDER_RGB: Final[int]
BYTE_ORDER_BGR: Final[int]


class NV3041A_RGB(rgb_display_framework.RGBDisplayDriver):
    pass


class NV3041A(display_driver_framework.DisplayDriver):

    __flush_ready_count: int

    @staticmethod
    def __quad_spi_cmd_modifier(cmd: int) -> int:
        ...

    @staticmethod
    def __quad_spi_color_cmd_modifier(cmd: int) -> int:
        ...

    @staticmethod
    def __dummy_cmd_modifier(cmd: int) -> int:
        ...

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
        rgb565_byte_swap: bool = False,  # NOQA
    ):
        ...

    def _flush_ready_cb(self, *_) -> None:
        ...

    def set_params(self, cmd: int, params: memoryview | None = None) -> None:
        ...

    def _set_memory_location(self, x1: int, y1: int, x2: int, y2: int) -> int:
        ...

    def _dummy_set_memory_location(self, x1: int, y1: int, x2: int, y2: int) -> int:
        ...

    def _flush_cb(self, _, area, color_p) -> None:
        ...
