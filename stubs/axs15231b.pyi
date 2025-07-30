# Based on the work by straga (https://github.com/straga)
# https://github.com/straga/micropython_lcd/blob/master/device/JC3248W535/driver/axs15231b/axs15231b.py
# Copyright (c) 2024 - 2025 Kevin G. Schlosser


from typing import Final, Union, TYPE_CHECKING

import display_driver_framework

import lcd_bus

if TYPE_CHECKING:
    import machine
    import io_expander_framework


STATE_HIGH: Final[int]
STATE_LOW: Final[int]
STATE_PWM: Final[int]

BYTE_ORDER_RGB: Final[int]
BYTE_ORDER_BGR: Final[int]

_pin_type = Union[io_expander_framework.Pin, machine.Pin, int, str, None]
_bus_type = Union[lcd_bus.RGBBus | lcd_bus.I80Bus | lcd_bus.I2CBus | lcd_bus.SPIBus]


class AXS15231B(display_driver_framework.DisplayDriver):
    _brightness: int
    __qspi: bool

    def __init__(
        self,
        data_bus: _bus_type,
        display_width: int,
        display_height: int,
        frame_buffer1: memoryview | None = None,
        frame_buffer2: memoryview | None = None,
        reset_pin: _pin_type = None,
        reset_state: int = STATE_HIGH,
        power_pin: _pin_type = None,
        power_on_state: int = STATE_HIGH,
        backlight_pin: _pin_type = None,
        backlight_on_state: int = STATE_HIGH,
        offset_x: int = 0,
        offset_y: int = 0,
        color_byte_order: int = BYTE_ORDER_RGB,
        color_space: int = lv.COLOR_FORMAT.RGB888,  # NOQA
        rgb565_byte_swap: bool = False
    ):
        ...

    def reset(self) -> None:
        ...

    def init(self, type: int | None = None) -> None:  # NOQA
        ...

    def set_brightness(self, value: int | float) -> None:
        ...

    def get_brightness(self) -> float:
        ...

    def set_params(self, cmd: int, params: memoryview | None = None):
        ...

    def _set_memory_location(self, x1: int, y1: int, x2: int, y2: int) -> int:
        ...
