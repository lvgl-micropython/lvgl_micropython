# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA
import machine  # NOQA

import lvgl as lv
import display_driver_framework


BYTE_ORDER_RGB = 0x00
BYTE_ORDER_RBG = 0x01
BYTE_ORDER_GRB = 0x02
BYTE_ORDER_GBR = 0x03
BYTE_ORDER_BRG = 0x04
BYTE_ORDER_BGR = 0x05

STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

_SRR = const(0x00)
_SSR_RST = const(0x40)

_MRWDP = const(0x04)

_CVSSA_1 = const(0x50)
_CVSSA_2 = const(0x51)
_CVSSA_3 = const(0x52)
_CVSSA_4 = const(0x53)

_CVS_IMWTH_L = const(0x54)
_CVS_IMWTH_H = const(0x55)

_AWUL_X_L = const(0x56)
_AWUL_X_H = const(0x57)

_AWUL_Y_L = const(0x58)
_AWUL_Y_H = const(0x59)

_AW_WTH_L = const(0x5A)
_AW_WTH_H = const(0x5B)

_AW_HT_L = const(0x5C)
_AW_HT_H = const(0x5D)

_CURH_L = const(0x5F)
_CURH_H = const(0x60)

_CURV_L = const(0x61)
_CURV_H = const(0x62)

_AW_COLOR = const(0x5E)
_AW_COLOR_CANVAS_MASK = const(0x03)
_AW_COLOR_CANVAS_8BPP = const(0x00)
_AW_COLOR_CANVAS_16BPP = const(0x01)
_AW_COLOR_CANVAS_24BPP = const(0x02)
_AW_COLOR_LINEAR_ADDR = const(0x04)
_AW_COLOR_BLOCK_ADDR = const(0x00)

_DPCR = const(0x12)
_DPCR_VDIR_MASK = const(0x20)
_DPCR_VDIR_TOP_BOT = const(0x00)
_DPCR_VDIR_BOT_TOP = const(0x20)

_MACR = const(0x02)
_MACR_W_MASK = const(0x06)
_MACR_W_LRTB = const(0x00)
_MACR_W_RLTB = const(0x02)
_MACR_W_TBLR = const(0x04)
_MACR_W_BTLR = const(0x06)


TYPE_ER_TFTMC050_3 = 0x01


class LT7381(display_driver_framework.DisplayDriver):
    WAIT_TIMEOUT = 100

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
        rgb565_byte_swap=False,
        wait_pin=None,
        wait_state=STATE_HIGH
    ):
        if wait_pin in (None, -1):
            raise RuntimeError('wait pin is required')

        if not isinstance(wait_pin, int):
            self._wait_pin = wait_pin
        else:
            self._wait_pin = machine.Pin(wait_pin, machine.Pin.IN)

        self._wait_state = wait_state
        self._dpcr = 0x00
        self._macr = 0x00
        self._aw_color = 0x00
        self._pmuxr = 0x00
        self._pcfgr = 0x00
        self._backlight = 0

        super().__init__(
            data_bus=data_bus,
            display_width=display_width,
            display_height=display_height,
            frame_buffer1=frame_buffer1,
            frame_buffer2=frame_buffer2,
            reset_pin=reset_pin,
            reset_state=reset_state,
            power_pin=power_pin,
            power_on_state=power_on_state,
            backlight_pin=backlight_pin,
            backlight_on_state=backlight_on_state,
            offset_x=offset_x,
            offset_y=offset_y,
            color_byte_order=color_byte_order,
            color_space=lv.COLOR_FORMAT.RGB565,  # NOQA
            rgb565_byte_swap=rgb565_byte_swap,
        )

    def _on_size_change(self, _):
        rotation = self._disp_drv.get_rotation()
        if rotation == lv.DISP_ROTATION_90:
            self._rotation = rotation
            self.set_rotation(lv.DISP_ROTATION_0)
            return

        self._width = self._disp_drv.get_horizontal_resolution()
        self._height = self._disp_drv.get_vertical_resolution()

        if rotation == self._rotation:
            return

        self._rotation = rotation

        if self._initilized:
            self._dpcr &= ~_DPCR_VDIR_MASK
            self._macr &= ~_MACR_W_MASK

            if rotation == lv.DISP_ROTATION_0:
                self._dpcr |= _DPCR_VDIR_TOP_BOT
                self._macr |= _MACR_W_LRTB

            elif rotation == lv.DISP_ROTATION_180:
                self._dpcr |= _DPCR_VDIR_BOT_TOP
                self._macr |= _MACR_W_RLTB

            elif rotation == lv.DISP_ROTATION_270:
                self._dpcr |= _DPCR_VDIR_BOT_TOP
                self._macr |= _MACR_W_TBLR

            self._write_reg(_DPCR, self._dpcr)
            self._write_reg(_MACR, self._macr)

    def _write_reg(self, reg, param):
        self._param_buf[0] = param
        self.set_params(reg, self._param_mv[:1])

    def set_invert_colors(self, value):
        raise NotImplementedError

    def _wait(self):
        start_time = time.ticks_ms()  # NOQA
        while (
            self._wait_pin.value() != self._wait_state and
            time.ticks_diff(time.ticks_ms(), start_time) < self.WAIT_TIMEOUT  # NOQA
        ):
            time.sleep_ms(1)  # NOQA

    def reset(self):
        if self._reset_pin is None:
            self._write_reg(_SRR, _SSR_RST)
            time.sleep_ms(20)  # NOQA
        else:
            display_driver_framework.DisplayDriver.reset(self)
            time.sleep_ms(50)  # NOQA

    def _set_memory_location(self, x_start, y_start, x_end, y_end):

        # set active window start X/Y
        self._write_reg(_AWUL_X_L, x_start & 0xFF)
        self._write_reg(_AWUL_X_H, (x_start >> 8) & 0x0F)
        self._write_reg(_AWUL_Y_L, y_start & 0xFF)
        self._write_reg(_AWUL_Y_H, (y_start >> 8) & 0x0F)

        # set active window width and height
        width = x_end - x_start + 1
        height = y_end - y_start + 1
        self.set_params(_AW_WTH_L, width & 0xFF)
        self.set_params(_AW_WTH_H, (width >> 8) & 0xFF)
        self.set_params(_AW_HT_L, height & 0xFF)
        self.set_params(_AW_HT_H, (height >> 8) & 0xFF)

        # set cursor
        self.set_params(_CURH_L, x_start & 0xff)
        self.set_params(_CURH_H, (x_start >> 8) & 0xFF)
        self.set_params(_CURV_L, y_start & 0xFF)
        self.set_params(_CURV_H, (y_start >> 8) & 0xFF)

        return _MRWDP

    def _flush_cb(self, _, area, color_p):
        x1 = area.x1 + self._offset_x
        x2 = area.x2 + self._offset_x

        y1 = area.y1 + self._offset_y
        y2 = area.y2 + self._offset_y

        size = (
            (x2 - x1 + 1) *
            (y2 - y1 + 1) *
            lv.color_format_get_size(self._color_space)
        )

        cmd = self._set_memory_location(x1, y1, x2, y2)

        self._wait()

        # we have to use the __dereference__ method because this method is
        # what converts from the C_Array object the binding passes into a
        # memoryview object that can be passed to the bus drivers
        data_view = color_p.__dereference__(size)

        self._data_bus.tx_color(cmd, data_view, x1, y1, x2, y2, self._rotation,
                                self._disp_drv.flush_is_last())
