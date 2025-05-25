# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Any, Callable, Optional, Union, ClassVar, Final
import array
import machine

_BufferType = Union[bytearray, memoryview, bytes, array.array]

MEMORY_32BIT: Final[int] = ...
MEMORY_8BIT: Final[int] = ...
MEMORY_DMA: Final[int] = ...
MEMORY_SPIRAM: Final[int] = ...
MEMORY_INTERNAL: Final[int] = ...
MEMORY_DEFAULT: Final[int] = ...
DEBUG_ENABLED: Final[int] = ...


class I2CBus:

    def __init__(
        self,
        *,
        sda: int,
        scl: int,
        addr: int,
        host: int = 0,
        control_phase_bytes: int = 1,
        dc_bit_offset: int = 6,
        freq: int = 10000000,
        dc_low_on_data: bool = False,
        sda_pullup: bool = True,
        scl_pullup: bool = True,
        disable_control_phase: bool = False
    ):
        ...

    def init(
        self, width: int, height: int, bpp: int, buffer_size: int,
        rgb565_byte_swap: bool, cmd_bits: int, param_bits: int, /
    ) -> None:
        ...

    def deinit(self) -> None:
        ...

    def register_callback(self, callback: Callable[[Any, Any], None], /) -> None:
        ...

    def tx_param(self, cmd: int, params: Optional[_BufferType] = None, /) -> None:
        ...

    def tx_color(self, cmd: int, data: _BufferType, start_x: int, start_y: int, end_x: int, end_y: int, rotation: int, last_update: bool, /) -> None:
        ...

    def get_lane_count(self) -> int:
        ...

    def allocate_framebuffer(self, size: int, caps: int, /) -> Union[None, memoryview]:
        ...

    def free_framebuffer(self, framebuffer: memoryview, /) -> None:
        ...


class SPIBus:

    def __init__(
        self,
        *,
        spi_bus: machine.SPI.Bus | machine.SPI.DualBus | machine.SPI.QuadBus | machine.SPI.OctalBus,
        dc: int,
        freq: int,
        cs: int = -1,
        dc_low_on_data: bool = False,
        lsb_first: bool = False,
        cs_high_active: bool = False,
        spi_mode: int = 0,
        dual: bool = False,
        quad: bool = False,
        octal: bool = False
    ):
        ...

    def init(
        self, width: int, height: int, bpp: int, buffer_size: int,
        rgb565_byte_swap: bool, cmd_bits: int, param_bits: int, /
    ) -> None:
        ...

    def deinit(self) -> None:
        ...

    def register_callback(self, callback: Callable[[Any, Any], None], /) -> None:
        ...

    def tx_param(self, cmd: int, params: Optional[_BufferType] = None, /) -> None:
        ...

    def tx_color(self, cmd: int, data: _BufferType, start_x: int, start_y: int, end_x: int, end_y: int, rotation: int, last_update: bool, /) -> None:
        ...

    def get_lane_count(self) -> int:
        ...

    def allocate_framebuffer(self, size: int, caps: int, /) -> Union[None, memoryview]:
        ...

    def free_framebuffer(self, framebuffer: memoryview, /) -> None:
        ...


class SDLBus:
    WINDOW_FULLSCREEN: ClassVar[int] = ...
    WINDOW_FULLSCREEN_DESKTOP: ClassVar[int] = ...
    WINDOW_BORDERLESS: ClassVar[int] = ...
    WINDOW_MINIMIZED: ClassVar[int] = ...
    WINDOW_MAXIMIZED: ClassVar[int] = ...
    WINDOW_ALLOW_HIGHDPI: ClassVar[int] = ...
    WINDOW_ALWAYS_ON_TOP: ClassVar[int] = ...
    WINDOW_SKIP_TASKBAR: ClassVar[int] = ...
    WINDOW_UTILITY: ClassVar[int] = ...
    WINDOW_TOOLTIP: ClassVar[int] = ...
    WINDOW_POPUP_MENU: ClassVar[int] = ...

    def __init__(
        self,
        *,
        flags: int
    ):
        ...

    def init(
        self, width: int, height: int, bpp: int, buffer_size: int,
        rgb565_byte_swap: bool, cmd_bits: int, param_bits: int, /
    ) -> None:
        ...

    def deinit(self) -> None:
        ...

    def register_mouse_callback(
        self,
        callback: Callable[[list], None],
        /
    ) -> None:
        ...

    def register_window_callback(
        self,
        callback: Callable[[list], None],
        /
    ) -> None:
        ...

    def register_keypad_callback(
        self,
        callback: Callable[[list], None],
        /
    ) -> None:
        ...

    def register_quit_callback(
        self,
        callback: Callable[[], None],
        /
    ) -> None:
        ...

    def set_window_size(
        self,
        width: int,
        height: int,
        px_format: int,
        ignore_size_chg: bool,
        /
    ) -> None:
        ...

    def register_callback(
        self,
        callback: Callable[[Any, Any], None],
        /
    ) -> None:
        ...

    def tx_param(
        self,
        cmd: int,
        params: Optional[_BufferType] = None,
        /
    ) -> None:
        ...

    def tx_color(self, cmd: int, data: _BufferType, start_x: int, start_y: int, end_x: int, end_y: int, rotation: int, last_update: bool, /) -> None:
        ...

    def get_lane_count(self) -> int:
        ...

    def allocate_framebuffer(self, size: int, caps: int, /) -> Union[None, memoryview]:
        ...

    def free_framebuffer(self, framebuffer: memoryview, /) -> None:
        ...

    def poll_events(self):
        ...

class RGBBus:

    def __init__(
        self,
        *,
        hsync: int,
        vsync: int,
        de: int,
        pclk: int,
        data0: int,
        data1: int,
        data2: int,
        data3: int,
        data4: int,
        data5: int,
        data6: int,
        data7: int,
        data8: int = -1,
        data9: int = -1,
        data10: int = -1,
        data11: int = -1,
        data12: int = -1,
        data13: int = -1,
        data14: int = -1,
        data15: int = -1,
        freq: int = 8000000,
        hsync_front_porch: int = 0,
        hsync_back_porch: int = 0,
        hsync_pulse_width: int = 1,
        hsync_idle_low: bool = False,
        vsync_front_porch: int = 0,
        vsync_back_porch: int = 0,
        vsync_pulse_width: int = 1,
        vsync_idle_low: bool = False,
        de_idle_high: bool = False,
        pclk_idle_high: bool = False,
        pclk_active_low: bool = False,
        disp_active_low: bool = False,
        refresh_on_demand: bool = False,
        rgb565_dither: bool = False
    ):
        ...

    def init(
        self, width: int, height: int, bpp: int, buffer_size: int,
        rgb565_byte_swap: bool, cmd_bits: int, param_bits: int, /
    ) -> None:
        ...

    def deinit(self) -> None:
        ...

    def register_callback(self, callback: Callable[[Any, Any], None], /) -> None:
        ...

    def tx_param(self, cmd: int, params: Optional[_BufferType] = None, /) -> None:
        ...

    def tx_color(self, cmd: int, data: _BufferType, start_x: int, start_y: int, end_x: int, end_y: int, rotation: int, last_update: bool, /) -> None:
        ...

    def get_lane_count(self) -> int:
        ...

    def allocate_framebuffer(self, size: int, caps: int, /) -> Union[None, memoryview]:
        ...

    def free_framebuffer(self, framebuffer: memoryview, /) -> None:
        ...


class I80Bus:

    def __init__(
        self,
        *,
        dc: int,
        wr: int,
        data0: int,
        data1: int,
        data2: int,
        data3: int,
        data4: int,
        data5: int,
        data6: int,
        data7: int,
        data8: int = -1,
        data9: int = -1,
        data10: int = -1,
        data11: int = -1,
        data12: int = -1,
        data13: int = -1,
        data14: int = -1,
        data15: int = -1,
        cs: int = -1,
        freq: int = 10000000,
        dc_idle_high: bool = False,
        dc_cmd_high: bool = False,
        dc_dummy_high: bool = False,
        dc_data_high: bool = True,
        cs_active_high: bool = False,
        reverse_color_bits: bool = False,
        swap_color_bytes: bool = False,
        pclk_active_low: bool = False,
        pclk_idle_low: bool = False,
    ):
        ...

    def init(
        self, width: int, height: int, bpp: int, buffer_size: int,
        rgb565_byte_swap: bool, cmd_bits: int, param_bits: int, /
    ) -> None:
        ...

    def deinit(self) -> None:
        ...

    def register_callback(self, callback: Callable[[Any, Any], None], /) -> None:
        ...

    def tx_color(self, cmd: int, data: _BufferType, start_x: int, start_y: int, end_x: int, end_y: int, rotation: int, last_update: bool, /) -> None:
        ...

    def tx_param(self, cmd: int, data: _BufferType, /) -> None:
        ...

    def get_lane_count(self) -> int:
        ...

    def allocate_framebuffer(self, size: int, caps: int, /) -> Union[None, memoryview]:
        ...

    def free_framebuffer(self, framebuffer: memoryview, /) -> None:
        ...


def _pump_main_thread() -> None:
    ...


del Any
del Callable
del Optional
del Union
del array
del _BufferType
