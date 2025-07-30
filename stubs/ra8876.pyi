# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Final, ClassVar
import display_driver_framework


STATE_HIGH: Final[int]
STATE_LOW: Final[int]
STATE_PWM: Final[int]

BYTE_ORDER_RGB: Final[int]
BYTE_ORDER_BGR: Final[int]


IS42SM16160D: Final[int]  # integrated silicon solution dram IC
IS42S16320B: Final[int]  # integrated silicon solution dram IC
IS42S16400F: Final[int]  # integrated silicon solution dram IC
M12L32162A: Final[int]  # elite semiconductor dram IC
M12L2561616A: Final[int]  # elite semiconductor dram IC
W9825G6JH: Final[int]  # winbond dram IC
W9812G6JH: Final[int]  # winbond dram IC
MT48LC4M16A: Final[int]  # micron dram IC
K4S641632N: Final[int]  # samsung dram IC
K4S281632K: Final[int]  # samsung dram IC


class RA8876(display_driver_framework.DisplayDriver):
    WAIT_TIMEOUT: ClassVar[int]
    _wait_pin: None
    _wait_state: int

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
        rgb565_byte_swap: bool = False,
        wait_pin=None,
        wait_state: int = STATE_HIGH
    ):
        ...

    def set_color_inversion(self, value: bool) -> None:
        raise NotImplementedError

    def set_rotation(self, value: int) -> None:
        raise NotImplementedError

    def get_rotation(self) -> int:
        raise NotImplementedError

    def _wait(self) -> None:
        ...

    def reset(self) -> None:
        ...

    def init( # NOQA
        self,
        dram_ic: int = W9812G6JH,
        osc_freq: int = 10,  # crystal clock (MHz)
        dram_freq: int = 100,  # SDRAM clock frequency (MHz)
        core_freq: int = 100,  # core (system) clock frequency (MHz)
        scan_freq: int = 50,  # pixel scan clock frequency (MHz)
        hndr: int = 160,  # horizontal non-display period or back porch
        hstr: int = 160,  # horizontal start position or front porch
        hpwr: int = 70,  # HSYNC pulse width
        vndr: int = 23,  # vertical non-display period
        vstr: int = 12,  # vertical start position
        vpwr: int = 10,  # VSYNC pulse width
    ) -> None:
        ...

    def _set_memory_location(self, x1: int, y1: int, x2: int, y2: int) -> int:
        ...

