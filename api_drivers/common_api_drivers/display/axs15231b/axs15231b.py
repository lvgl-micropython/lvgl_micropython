# Based on the work by straga (https://github.com/straga)
# https://github.com/straga/micropython_lcd/blob/master/device/JC3248W535/driver/axs15231b/axs15231b.py
# Copyright (c) 2024 - 2025 Kevin G. Schlosser


import display_driver_framework
from micropython import const  # NOQA

import lcd_bus
import gc

import lvgl as lv  # NOQA


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR

_RASET = const(0x02002B00)
_CASET = const(0x02002A00)
_MADCTL = const(0x02003600)

_RAMWR = const(0x32002C00)
_RAMWRC = const(0x32003C00)

_WRDISBV = const(0x02005100)


class AXS15231B(display_driver_framework.DisplayDriver):

    def __init__(
        self,
        data_bus,
        display_width,
        display_height,
        frame_buffer1=None,
        frame_buffer2=None,
        reset_pin=None,
        reset_state=STATE_HIGH,
        power_pin=None,
        power_on_state=STATE_HIGH,
        backlight_pin=None,
        backlight_on_state=STATE_HIGH,
        offset_x=0,
        offset_y=0,
        color_byte_order=BYTE_ORDER_RGB,
        color_space=lv.COLOR_FORMAT.RGB888,  # NOQA
        rgb565_byte_swap=False
    ):
        self.__tx_color_count = 0
        self._brightness = 0xD0
        color_size = lv.color_format_get_size(self._color_space)
        buf_size = display_width * display_height * color_size
        if isinstance(data_bus, lcd_bus.RGBBus):
            buf_size = int(buf_size // 10)

        if frame_buffer1 is None:
            gc.collect()

            if isinstance(data_bus, lcd_bus.RGBBus):
                memory_flags = (lcd_bus.MEMORY_INTERNAL, lcd_bus.MEMORY_SPIRAM)
            else:
                memory_flags = (
                    lcd_bus.MEMORY_INTERNAL | lcd_bus.MEMORY_DMA,
                    lcd_bus.MEMORY_SPIRAM | lcd_bus.MEMORY_DMA,
                    lcd_bus.MEMORY_INTERNAL,
                    lcd_bus.MEMORY_SPIRAM
                )

            for flags in memory_flags:
                try:
                    frame_buffer1 = (
                        data_bus.allocate_framebuffer(buf_size, flags)
                    )

                    if (flags | lcd_bus.MEMORY_DMA) == flags:
                        frame_buffer2 = (
                            data_bus.allocate_framebuffer(buf_size, flags)
                        )

                    break

                except MemoryError:
                    frame_buffer1 = data_bus.free_framebuffer(frame_buffer1)

            if frame_buffer1 is None:
                raise MemoryError(
                    f'Unable to allocate memory for frame buffer ({buf_size})'
                    # NOQA
                )

        if not isinstance(data_bus, lcd_bus.RGBBus):
            if len(frame_buffer1) != buf_size:
                raise MemoryError(f'The frame buffer is too small ({buf_size})')

        super().__init__(
            data_bus,
            display_width,
            display_height,
            frame_buffer1,
            frame_buffer2,
            reset_pin,
            reset_state,
            power_pin,
            power_on_state,
            backlight_pin,
            backlight_on_state,
            offset_x,
            offset_y,
            color_byte_order,
            color_space,  # NOQA
            rgb565_byte_swap,
            _cmd_bits=32
        )

    def _init_bus(self):
        buffer_size = len(self._frame_buffer1)

        self._data_bus.init(
            self.display_width,
            self.display_height,
            lv.color_format_get_size(self._color_space) * 8,
            buffer_size,
            self._rgb565_byte_swap,
            self._cmd_bits,
            self._param_bits
        )

        self._disp_drv.set_flush_cb(self._flush_cb)

        if isinstance(self._data_bus, lcd_bus.RGBBus):
            setattr(
                self,
                '_set_memory_location',
                self._dummy_set_memory_location
            )
            render_mode = lv.DISPLAY_RENDER_MODE.PARTIAL  # NOQA
        else:
            render_mode = lv.DISPLAY_RENDER_MODE.DIRECT  # NOQA

        self._disp_drv.set_buffers(
            self._frame_buffer1,
            self._frame_buffer2,
            len(self._frame_buffer1),
            render_mode
        )

        self._data_bus.register_callback(self._flush_ready_cb)
        self.set_default()
        self._disp_drv.add_event_cb(
            self._on_size_change,
            lv.EVENT.RESOLUTION_CHANGED,  # NOQA
            None
        )

        self._displays.append(self)

    def set_rotation(self, value):
        if not isinstance(self._data_bus, lcd_bus.RGBBus):
            raise NotImplementedError

        super().set_rotation(value)

    def set_brightness(self, value):
        value = int(value / 100.0 * 255)
        value = max(0x00, min(value, 0xFF))

        self._brightness = value

        self._param_buf[0] = value
        self.set_params(_WRDISBV, self._param_mv[:1])

    def get_brightness(self):
        return round(self._brightness / 255.0 * 100.0, 1)

    def _flush_ready_cb(self, *_):
        self.__tx_color_count += 1
        if self.__tx_color_count == 2:
            self._disp_drv.flush_ready()
            self.__tx_color_count = 0

    def _set_memory_location(self, x1: int, y1: int, x2: int, y2: int):
        param_buf = self._param_buf  # NOQA

        param_buf[0] = (x1 >> 8) & 0xFF
        param_buf[1] = x1 & 0xFF
        param_buf[2] = (x2 >> 8) & 0xFF
        param_buf[3] = x2 & 0xFF
        self.set_params(_CASET, self._param_mv)

        return _RAMWR

    def _flush_cb(self, _, area, color_p):
        x1 = area.x1 + self._offset_x
        x2 = area.x2 + self._offset_x

        y1 = area.y1 + self._offset_y
        y2 = area.y2 + self._offset_y

        cmd = self._set_memory_location(x1, y1, x2, y2)

        width = x2 - x1 + 1
        height = y2 - y1 + 1

        color_size = lv.color_format_get_size(self._color_space)
        size = width * height * color_size
        chunk_size = int(width * height // 2) * color_size

        data_view = color_p.__dereference__(size)

        self._data_bus.tx_color(cmd, data_view[:chunk_size], x1, y1, x2, y2,
                                self._rotation, self._disp_drv.flush_is_last())
        self._data_bus.tx_color(_RAMWRC, data_view[chunk_size:], x1, y1, x2, y2,
                                self._rotation, self._disp_drv.flush_is_last())
