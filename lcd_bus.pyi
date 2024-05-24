from typing import Any, Callable, Optional, Union
import array

_BufferType = Union[bytearray, memoryview, bytes, array.array]

MEMORY_32BIT: int = ...
MEMORY_8BIT: int = ...
MEMORY_DMA: int = ...
MEMORY_SPIRAM: int = ...
MEMORY_INTERNAL: int = ...
MEMORY_DEFAULT: int = ...


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
        cmd_bits: Optional[int] = 8,
        param_bits: Optional[int] = 8,
        dc_low_on_data: Optional[bool] = False,
        sda_pullup: Optional[bool] = True,
        scl_pullup: Optional[bool] = True,
        disable_control_phase: Optional[bool] = False
    ):
        ...

    def init(self, width: int, height: int, bpp: int, buffer_size: int, rgb565_byte_swap: bool, /) -> None:
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
        dc: int,
        host: int,
        sclk: int,
        freq: int,
        mosi: int,
        miso: Optional[int] = -1,
        cs: Optional[int] = -1,
        wp: Optional[int] = -1,
        hd: Optional[int] = -1,
        quad_spi: Optional[bool] = False,
        tx_only: Optional[bool] = False,
        cmd_bits: Optional[int] = 8,
        param_bits: Optional[int] = 8,
        dc_low_on_data: Optional[bool] = False,
        sio_mode: Optional[bool] = False,
        lsb_first: Optional[bool] = False,
        cs_high_active: Optional[bool] = False,
        spi_mode: Optional[int] = 0
    ):
        ...

    def init(self, width: int, height: int, bpp: int, buffer_size: int, rgb565_byte_swap: bool, /) -> None:
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
    WINDOW_FULLSCREEN: int = ...
    WINDOW_FULLSCREEN_DESKTOP: int = ...
    WINDOW_BORDERLESS: int = ...
    WINDOW_MINIMIZED: int = ...
    WINDOW_MAXIMIZED: int = ...
    WINDOW_ALLOW_HIGHDPI: int = ...
    WINDOW_ALWAYS_ON_TOP: int = ...
    WINDOW_SKIP_TASKBAR: int = ...
    WINDOW_UTILITY: int = ...
    WINDOW_TOOLTIP: int = ...
    WINDOW_POPUP_MENU: int = ...

    def __init__(
        self,
        *,
        flags: int
    ):
        ...

    def init(
        self,
        width: int,
        height: int,
        bpp: int,
        buffer_size: int,
        rgb565_byte_swap: bool,
        /
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

    def realloc_buffer(
        self,
        size: int,
        buf_num: int,
        /
    ) -> Union[None, memoryview]:
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

    def init(self, width: int, height: int, bpp: int, buffer_size: int, rgb565_byte_swap: bool, /) -> None:
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
        cmd_bits: Optional[int] = 8,
        param_bits: Optional[int] = 8,
        cs_active_high: Optional[bool] = False,
        reverse_color_bits: Optional[bool] = False,
        swap_color_bytes: Optional[bool] = False,
        pclk_active_low: Optional[bool] = False,
        pclk_idle_low: Optional[bool] = False,
    ):
        ...

    def init(self, width: int, height: int, bpp: int, buffer_size: int, rgb565_byte_swap: bool, /) -> None:
        ...

    def deinit(self) -> None:
        ...

    def register_callback(self, callback: Callable[[Any, Any], None], /) -> None:
        ...

    def tx_color(self, cmd: int, data: _BufferType, start_x: int, start_y: int, end_x: int, end_y: int, /) -> None:
        ...

    def tx_color(self, cmd: int, data: _BufferType, /) -> None:
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
