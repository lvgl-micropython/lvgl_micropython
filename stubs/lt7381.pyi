# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Final, ClassVar
import display_driver_framework
import lcd_bus

import lvgl as lv


STATE_HIGH: Final[int]
STATE_LOW: Final[int]
STATE_PWM: Final[int]


BYTE_ORDER_RBG: Final[int]
BYTE_ORDER_GRB: Final[int]
BYTE_ORDER_GBR: Final[int]
BYTE_ORDER_BRG: Final[int]
BYTE_ORDER_BGR: Final[int]

TYPE_ER_TFTMC050_3: Final[int]


class LT7381(display_driver_framework.DisplayDriver):
    WAIT_TIMEOUT: ClassVar[int]

    _wait_pin: None
    _wait_state: int
    _dpcr: int
    _macr: int
    _aw_color: int
    _pmuxr: int
    _pcfgr: int
    _backlight: int

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
        wait_pin=None,
        wait_state: int = STATE_HIGH
    ):
        ...

    def _on_size_change(self, _) -> None:
        ...

    def _write_reg(self, reg: int, param: int) -> None:
        ...

    def set_color_inversion(self, value: int) -> None:
        raise NotImplementedError

    def _wait(self) -> None:
        ...

    def reset(self) -> None:
        ...

    def _set_memory_location(self, x1: int, y1: int, x2: int, y2: int) -> int:
        ...

    def _flush_cb(self, _, area, color_p) -> None:
        ...
