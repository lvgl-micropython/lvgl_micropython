# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import display_driver_framework
import rgb_display_framework  # NOQA
from micropython import const  # NOQA
import lcd_bus
import lvgl as lv
import gc


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR

_WRITE_CMD = const(0x02)
_WRITE_COLOR = const(0x32)

_RASET = const(0x2B)
_CASET = const(0x2A)
_RAMWR = const(0x2C)


class SPD2010(display_driver_framework.DisplayDriver):

    _ORIENTATION_TABLE = ()

    def _madctl(self, colormode, rotations, rotation=None):
        if rotation is None:
            rotation = ~self._rotation

        rotation = ~rotation
        value = colormode

        if rotation:
            value |= 0x03

        return value

    def set_rotation(self, value):

        if value not in (lv.DISPLAY_ROTATION._0, lv.DISPLAY_ROTATION._180):  # NOQA
            print('This rotation is not supported, only 0 and 180 is supported')
            return

        self._disp_drv.set_rotation(value)

    @staticmethod
    def __quad_spi_cmd_modifier(cmd):
        cmd <<= 8
        cmd |= _WRITE_CMD << 24
        return cmd

    @staticmethod
    def __quad_spi_color_cmd_modifier(cmd):
        cmd <<= 8
        cmd |= _WRITE_COLOR << 24
        return cmd

    @staticmethod
    def __dummy_cmd_modifier(cmd):
        return cmd

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
        rgb565_byte_swap=False,  # NOQA
    ):
        num_lanes = data_bus.get_lane_count()

        if num_lanes == 4:
            self.__cmd_modifier = self.__quad_spi_cmd_modifier
            self.__color_cmd_modifier = self.__quad_spi_color_cmd_modifier
            _cmd_bits = 32

            # we need to override the default handling for creating the frame
            # buffer is using a quad spi bus. we don't want it to create
            # partial buffers for the quad SPI display

            buf_size = display_width * display_height * lv.color_format_get_size(color_space)

            if frame_buffer1 is None:
                gc.collect()

                for flags in (
                    lcd_bus.MEMORY_INTERNAL | lcd_bus.MEMORY_DMA,
                    lcd_bus.MEMORY_SPIRAM | lcd_bus.MEMORY_DMA,
                    lcd_bus.MEMORY_INTERNAL,
                    lcd_bus.MEMORY_SPIRAM
                ):
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
                        frame_buffer1 = data_bus.free_framebuffer(frame_buffer1)  # NOQA

                if frame_buffer1 is None:
                    raise MemoryError(
                        f'Unable to allocate memory for frame buffer ({buf_size})'  # NOQA
                    )

                if len(frame_buffer1) != buf_size:
                    raise ValueError('incorrect framebuffer size')
        else:
            self.__cmd_modifier = self.__dummy_cmd_modifier
            self.__color_cmd_modifier = self.__dummy_cmd_modifier
            _cmd_bits = 8

        # store these so we do not have to keep on converting them
        self.__ramwr = self.__color_cmd_modifier(_RAMWR)
        self.__caset = self.__cmd_modifier(_CASET)
        self.__raset = self.__cmd_modifier(_RASET)

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
            # we don't need to sue RGB565 byte swap so we override it
            rgb565_byte_swap=False,
            _cmd_bits=_cmd_bits,
            _param_bits=8,
            _init_bus=True
        )

    def _flush_ready_cb(self, *_):
        # since there are 2 transfers that take place we need to
        # make sure that flush ready is only called after the second part
        # of the buffer has sent.
        self._disp_drv.flush_ready()

    def set_params(self, cmd, params=None):
        cmd = self.__cmd_modifier(cmd)
        self._data_bus.tx_param(cmd, params)

    def _set_memory_location(self, x1: int, y1: int, x2: int, y2: int):
        param_buf = self._param_buf  # NOQA

        param_buf[0] = (x1 >> 8) & 0xFF
        param_buf[1] = x1 & 0xFF
        param_buf[2] = (x2 >> 8) & 0xFF
        param_buf[3] = x2 & 0xFF

        self._data_bus.tx_param(self.__caset, self._param_mv)

        param_buf[0] = (y1 >> 8) & 0xFF
        param_buf[1] = y1 & 0xFF
        param_buf[2] = (y2 >> 8) & 0xFF
        param_buf[3] = y2 & 0xFF

        self._data_bus.tx_param(self.__raset, self._param_mv)

    def _flush_cb(self, _, area, color_p):
        x1 = area.x1 + self._offset_x
        x2 = area.x2 + self._offset_x

        y1 = area.y1 + self._offset_y
        y2 = area.y2 + self._offset_y

        self._set_memory_location(x1, y1, x2, y2)

        width = x2 - x1 + 1
        height = y2 - y1 + 1
        size = width * height * lv.color_format_get_size(self._color_space)

        data_view = color_p.__dereference__(size)
        self._data_bus.tx_color(self.__ramwr, data_view, x1, y1, x2, y2, self._rotation, False)
