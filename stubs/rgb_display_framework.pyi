# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Final
import display_driver_framework
import lcd_bus
import spi3wire


STATE_HIGH: Final[int]
STATE_LOW: Final[int]
STATE_PWM: Final[int]

BYTE_ORDER_RGB: Final[int]
BYTE_ORDER_BGR: Final[int]


class RGBDisplayDriver(display_driver_framework.DisplayDriver):
    _spi_3wire: spi3wire.Spi3Wire | None
    _bus_shared_pins: bool

    def __init__(
        self,
        data_bus: lcd_bus.RGBBus,
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
        rgb565_byte_swap: bool = False,
        spi_3wire: spi3wire.Spi3Wire | None = None,
        spi_3wire_shared_pins: bool = False,
        _cmd_bits: int = 8,
        _param_bits: int = 8,
        _init_bus: bool = True
    ):
        ...

    def set_params(self, cmd: int, params: memoryview | None = None) -> None:
        ...

    def get_params(self, cmd: int, params: memoryview) -> None:
        ...

    def _set_memory_location(self, x1: int, y1: int, x2: int, y2: int) -> int:  # NOQA
        ...

    def _spi_3wire_init(self, *args, **kwargs) -> None:
        raise NotImplementedError

    def init(self, type: int | None = None) -> None:  # NOQA
        ...
