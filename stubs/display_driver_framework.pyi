# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Optional, Tuple, Union, ClassVar, Callable, List, Any
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    import micropython  # NOQA
    import machine  # NOQA
    import lvgl as lv  # NOQA
    import array  # NOQA
    import io_expander_framework  # NOQA

import lcd_bus


# Constants

BYTE_ORDER_RGB: int = ...
BYTE_ORDER_BGR: int = ...

STATE_HIGH: int = ...
STATE_LOW: int = ...
STATE_PWM: int = ...

_BufferType = Union[bytearray, memoryview, bytes, array.array]
_PinType = Union[machine.Pin, int, io_expander_framework.Pin]
_DatabusType = Union[lcd_bus.I80Bus, lcd_bus.I2CBus, lcd_bus.RGBBus, lcd_bus.SPIBus, lcd_bus.SDLBus]


class DisplayDriver:
    _INVON: ClassVar[int] = ...
    _INVOFF: ClassVar[int] = ...

    # MADCTL values for each of the orientation constants for non-st7789 displays.
    _ORIENTATION_TABLE: ClassVar[Tuple[int, int, int, int]] = ...

    _displays: ClassVar[list[_DatabusType]] = ...

    display_width: int = ...
    display_height: int = ...
    _reset_pin: Optional[_PinType] = ...
    _reset_state: int = ...
    _power_pin: Optional[_PinType] = ...
    _power_on_state: int = ...
    _backlight_pin: Optional[_PinType] = ...
    _backlight_on_state: int = ...
    _offset_x: int = ...
    _offset_y: int = ...
    _data_bus: _DatabusType = ...
    _param_buf: bytearray = ...
    _param_mv: memoryview = ...
    _disp_drv: lv.display_driver_t = ...  # NOQA
    _color_byte_order: int = ...
    _color_space: int = ...
    _physical_width: int = ...
    _physical_height: int = ...
    _initilized: bool = ...
    _frame_buffer1: Optional[_BufferType] = ...
    _frame_buffer2: Optional[_BufferType] = ...
    _backup_set_memory_location: Optional[Callable] = ...
    _rotation: int = ...
    _spi_3wire: lcd_bus.SPI3Wire = None

    # Default values of "power" and "backlight" are reversed logic! 0 means ON.
    # You can change this by setting backlight_on and power_on arguments.
    #
    # For the ESP32 the allocation of the frame buffers can be done one of 2
    # ways depending on what is wanted in terms of performance VS memory use
    # If a single frame buffer is used then using a DMA transfer is pointless
    # to do. The frame buffer in this casse can be allocated as simple as
    #
    # buf = bytearray(buffer_size)
    #
    # If the user wants to be able to specify if the frame buffer is to be
    # created in internal memory (SRAM) or in external memory (PSRAM/SPIRAM)
    # this can be done using the heap_caps module.
    #
    # internal memory:
    # buf = heap_caps.malloc(buffer_size, heap_caps.CAP_INTERNAL)
    #
    # external memory:
    # buf = heap_caps.malloc(buffer_size, heap_caps.CAP_SPIRAM)
    #
    # If wanting to use DMA memory then use the bitwise OR "|" operator to add
    # the DMA flag to the last parameter of the malloc function
    #
    # buf = heap_caps.malloc(
    #     buffer_size, heap_caps.CAP_INTERNAL | heap_caps.CAP_DMA
    # )

    @staticmethod
    def get_default() -> "DisplayDriver":
        ...

    @staticmethod
    def get_displays() -> List["DisplayDriver"]:
        ...

    def __init__(
        self,
        data_bus: _DatabusType,
        display_width: int,
        display_height: int,
        frame_buffer1: Optional[_BufferType] = None,
        frame_buffer2: Optional[_BufferType] = None,
        reset_pin: Optional[_PinType] = None,
        reset_state: int = STATE_HIGH,
        power_pin: Optional[_PinType] = None,
        power_on_state: int = STATE_HIGH,
        backlight_pin: Optional[_PinType] = None,
        backlight_on_state: int = STATE_HIGH,
        offset_x: int = 0,
        offset_y: int = 0,
        color_byte_order: int = BYTE_ORDER_RGB,
        color_space: int = lv.COLOR_FORMAT.RGB888,  # NOQA
        rgb565_byte_swap: bool = False,
        spi_3wire: Optional[lcd_bus.SPI3Wire] = None,
        _cmd_bits: int = 8,
        _param_bits: int = 8,
        _init_bus: bool = True
    ) -> object:
        ...

    def set_physical_resolution(self, width: int, height: int) -> None:
        ...

    def get_physical_horizontal_resolution(self) -> int:
        ...

    def get_physical_vertical_resolution(self) -> int:
        ...

    def set_physical_horizontal_resolution(self, width: int) -> None:
        ...

    def set_physical_vertical_resolution(self, height: int) -> None:
        ...

    def get_next(self) -> "DisplayDriver":
        ...

    def set_offset(self, x: int, y: int) -> None:
        ...

    def get_offset_x(self) -> int:
        ...

    def get_offset_y(self) -> int:
        ...

    def get_dpi(self) -> int:
        ...

    def set_dpi(self, dpi: int) -> None:
        ...

    def set_color_format(self, color_space: int) -> None:
        ...

    def get_color_format(self) -> int:
        ...

    def set_antialiasing(self, en: bool) -> None:
        ...

    def get_antialiasing(self) -> bool:
        ...

    def is_double_buffered(self) -> bool:
        ...

    def get_screen_active(self) -> lv.obj:  # NOQA
        ...

    def get_screen_prev(self) -> lv.obj:  # NOQA
        ...

    def get_layer_top(self) -> lv.obj:  # NOQA
        ...

    def get_layer_sys(self) -> lv.obj:  # NOQA
        ...

    def get_layer_bottom(self) -> lv.obj:  # NOQA
        ...

    def add_event_cb(self, event_cb, filter, user_data) -> None:  # NOQA
        ...

    def get_event_count(self) -> int:
        ...

    def get_event_dsc(self, index: int) -> lv.event_dsc_t:
        ...

    def delete_event(self, index: int) -> bool:
        ...

    def send_event(self, code: int, param: Any) -> int:
        ...

    def set_theme(self, th: lv.theme_t) -> None:
        ...

    def get_theme(self) -> lv.theme_t:
        ...

    def get_inactive_time(self) -> int:
        ...

    def trigger_activity(self) -> None:
        ...

    def enable_invalidation(self, en: bool) -> None:
        ...

    def is_invalidation_enabled(self) -> bool:
        ...

    def get_refr_timer(self) -> lv.timer_t:
        ...

    def delete_refr_timer(self) -> None:
        ...

    def set_color_inversion(self, value: bool) -> None:
        ...

    def set_default(self) -> None:
        ...

    def get_rotation(self) -> int:
        ...

    def set_rotation(self, value: int) -> None:
        ...

    def get_horizontal_resolution(self) -> int:
        ...

    def get_vertical_resolution(self) -> int:
        ...

    def init(self) -> None:
        ...

    def set_params(self, cmd: int, params: Optional[_BufferType] = None) -> None:
        ...

    def get_params(self, cmd: int, params: _BufferType) -> None:
        ...

    def get_power(self) -> bool:
        ...

    def set_power(self, value: bool) -> None:
        ...

    def delete(self) -> None:
        ...

    def __del__(self):
        ...

    def reset(self) -> None:
        ...

    def get_backlight(self) -> Union[int, float]:
        ...

    def set_backlight(self, value: Union[int, float]) -> None:
        ...

    def _dummy_set_memory_location(self, *_, **__) -> int:  # NOQA
        ...

    # this function is handeled in the viper code emitter. This will
    # increase the performance to near C code execution times. While this is
    # not really heavy lifting in terms of work being done every cycle counts
    # and it adds up over time. Need to keep things running as fast as possible.

    def _set_memory_location(self, x1: int, y1: int, x2: int, y2: int) -> int:
        ...

    def _flush_cb(self, disp: lv.display_driver_t, area: lv.area_t, color_p: lv.CArray) -> None:  # NOQA
        ...

    # we always register this callback no matter what. This is what tells LVGL
    # that the buffer is able to be written to. If this callback doesn't get
    # registered then the flush function is going to block until the buffer
    # gets emptied. Everything is handeled internally in the bus driver if
    # using DMA and double buffer or a single buffer.

    def _flush_ready_cb(self, *param) -> None:
        ...

    def _madctl(self, colormode: int, rotations: Tuple[int, int, int, int], rotation: Optional[int] = None) -> int:
        ...

