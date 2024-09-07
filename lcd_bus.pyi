from typing import Any, Callable, Optional, Union, ClassVar, Final
import array
import spi as _spi

_BufferType = Union[bytearray, memoryview, bytes, array.array]

MEMORY_32BIT: Final[int] = ...
MEMORY_8BIT: Final[int] = ...
MEMORY_DMA: Final[int] = ...
MEMORY_SPIRAM: Final[int] = ...
MEMORY_INTERNAL: Final[int] = ...
MEMORY_DEFAULT: Final[int] = ...
DEBUG_ENABLED: Final[int] = ...


class SPI3Wire:

    def __init__(
        self,
        *,
        mosi: int,
        sclk: int,
        freq: int = 500000,
        cs: int = -1,
        cs_high_active: bool = False,
        keep_cs_inactive: bool = True,
        lsb_first: bool = False,
        dc_zero_on_data: bool = False,
        use_dc_bit: bool = False,
    ):
        ...

    def init(self, cmd_bits: int, param_bits: int, /) -> None:
        ...

    def deinit(self) -> None:
        ...

    def tx_param(self, cmd: int, params: Optional[_BufferType] = None, /) -> None:
        ...


class I2CBus:

    def __init__(
        self,
        *,
        sda: int,
        scl: int,
        addr: int,
        host: Optional[int] = 0,
        control_phase_bytes: Optional[int] = 1,
        dc_bit_offset: Optional[int] = 6,
        freq: Optional[int] = 10000000,
        dc_low_on_data: Optional[bool] = False,
        sda_pullup: Optional[bool] = True,
        scl_pullup: Optional[bool] = True,
        disable_control_phase: Optional[bool] = False
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

    def tx_color(self, cmd: int, data: _BufferType, start_x: int, start_y: int, end_x: int, end_y: int, /) -> None:
        ...

    def rx_param(self, cmd: int, params: _BufferType, /) -> None:
        ...

    def get_lane_count(self) -> int:
        ...

    def allocate_framebuffer(self, size: int, caps: int, /) -> Union[None, memoryview]:
        ...


class SPIBus:

    def __init__(
        self,
        *,
        spi_bus: _spi.SPI,
        freq: int,
        dc: int,
        cs: Optional[int] = -1,
        polarity: int = 0,
        phase: int = 0,
        firstbit: int = _spi.SPI.MSB,
        cs_high_active: bool = False,
        dc_low_on_data: bool = False,
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

    def tx_color(self, cmd: int, data: _BufferType, start_x: int, start_y: int, end_x: int, end_y: int, /) -> None:
        ...

    def rx_param(self, cmd: int, params: _BufferType, /) -> None:
        ...

    def get_lane_count(self) -> int:
        ...

    def allocate_framebuffer(self, size: int, caps: int, /) -> Union[None, memoryview]:
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

    def tx_color(self, cmd: int, data: _BufferType, start_x: int, start_y: int, end_x: int, end_y: int, /) -> None:
        ...

    def rx_param(self, cmd: int, params: _BufferType, /) -> None:
        ...

    def get_lane_count(self) -> int:
        ...

    def allocate_framebuffer(self, size: int, caps: int, /) -> Union[None, memoryview]:
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
        disp: int,
        pclk: int,
        data0: int,
        data1: int,
        data2: int,
        data3: int,
        data4: int,
        data5: int,
        data6: int,
        data7: int,
        data8: Optional[int] = -1,
        data9: Optional[int] = -1,
        data10: Optional[int] = -1,
        data11: Optional[int] = -1,
        data12: Optional[int] = -1,
        data13: Optional[int] = -1,
        data14: Optional[int] = -1,
        data15: Optional[int] = -1,
        freq: Optional[int] = 8000000,
        hsync_front_porch: Optional[int] = 0,
        hsync_back_porch: Optional[int] = 0,
        hsync_pulse_width: Optional[int] = 1,
        hsync_idle_low: Optional[bool] = False,
        vsync_front_porch: Optional[int] = 0,
        vsync_back_porch: Optional[int] = 0,
        vsync_pulse_width: Optional[int] = 1,
        vsync_idle_low: Optional[bool] = False,
        de_idle_high: Optional[bool] = False,
        pclk_idle_high: Optional[bool] = False,
        pclk_active_low: Optional[bool] = False,
        disp_active_low: Optional[bool] = False,
        refresh_on_demand: Optional[bool] = False
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

    def tx_color(self, cmd: int, data: _BufferType, start_x: int, start_y: int, end_x: int, end_y: int, /) -> None:
        ...

    def rx_param(self, cmd: int, params: _BufferType, /) -> None:
        ...
    
    def get_lane_count(self) -> int:
        ...

    def allocate_framebuffer(self, size: int, caps: int, /) -> Union[None, memoryview]:
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
        data8: Optional[int] = -1,
        data9: Optional[int] = -1,
        data10: Optional[int] = -1,
        data11: Optional[int] = -1,
        data12: Optional[int] = -1,
        data13: Optional[int] = -1,
        data14: Optional[int] = -1,
        data15: Optional[int] = -1,
        cs: Optional[int] = -1,
        freq: Optional[int] = 10000000,
        dc_idle_high: Optional[bool] = False,
        dc_cmd_high: Optional[bool] = False,
        dc_dummy_high: Optional[bool] = False,
        dc_data_high: Optional[bool] = True,
        cs_active_high: Optional[bool] = False,
        reverse_color_bits: Optional[bool] = False,
        swap_color_bytes: Optional[bool] = False,
        pclk_active_low: Optional[bool] = False,
        pclk_idle_low: Optional[bool] = False,
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

    def tx_color(self, cmd: int, data: _BufferType, start_x: int, start_y: int, end_x: int, end_y: int, /) -> None:
        ...

    def tx_param(self, cmd: int, data: _BufferType, /) -> None:
        ...

    def rx_param(self, cmd: int, params: _BufferType, /) -> None:
        ...

    def get_lane_count(self) -> int:
        ...

    def allocate_framebuffer(self, size: int, caps: int, /) -> Union[None, memoryview]:
        ...


del Any
del Callable
del Optional
del Union
del array
del _BufferType
