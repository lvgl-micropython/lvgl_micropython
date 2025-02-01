# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA
import machine  # NOQA

import lvgl as lv
import lcd_bus  # NOQA
import display_driver_framework


IS42SM16160D = 0
IS42S16320B = 1
IS42S16400F = 3
M12L32162A = 4
M12L2561616A = 5
W9825G6JH = 6
W9812G6JH = 7
MT48LC4M16A = 8
K4S641632N = 9
K4S281632K = 10


#  Software Reset Register (SRR)
_SRR = const(0x00)
#  Memory Access Control Register (MACR)
_MACR = const(0x02)
#  Input Control Register (ICR)
_ICR = const(0x03)
#  Memory Data Read/Write Port (MRWDP)
_MRWDP = const(0x04)
#  Active Window Upper-Left corner X-coordinates 0 (AWUL_X0)
_AWUL_X0 = const(0x56)
#  Active Window Upper-Left corner X-coordinates 1 (AWUL_X1)
_AWUL_X1 = const(0x57)
#  Active Window Upper-Left corner Y-coordinates 0 (AWUL_Y0)
_AWUL_Y0 = const(0x58)
#  Active Window Upper-Left corner Y-coordinates 1 (AWUL_Y1)
_AWUL_Y1 = const(0x59)
#  Active Window Width 0 (AW_WTH0)
_AW_WTH0 = const(0x5A)
#  Active Window Width 1 (AW_WTH1)
_AW_WTH1 = const(0x5B)
#  Active Window Height 0 (AW_HT0)
_AW_HT0 = const(0x5C)
#  Active Window Height 1 (AW_HT1)
_AW_HT1 = const(0x5D)
#  Color Depth of Canvas & Active Window (AW_COLOR)
_AW_COLOR = const(0x5E)
#  Graphic Read/Write position Horizontal Position Register 0 (CURH0)
_CURH0 = const(0x5F)
#  Graphic Read/Write position Horizontal Position Register 1 (CURH1)
_CURH1 = const(0x60)
#  Graphic Read/Write position Vertical Position Register 0 (CURV0)
_CURV0 = const(0x61)
#  Graphic Read/Write position Vertical Position Register 1 (CURV1)
_CURV1 = const(0x62)


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = 0x00
BYTE_ORDER_RBG = 0x01
BYTE_ORDER_GRB = 0x02
BYTE_ORDER_GBR = 0x03
BYTE_ORDER_BRG = 0x04
BYTE_ORDER_BGR = 0x05


class RA8876(display_driver_framework.DisplayDriver):
    WAIT_TIMEOUT = 100

    SCAN_FREQ = 0
    OSC_FREQ = 0
    DRAM_FREQ = 0
    CORE_FREQ = 0
    DRAM_TYPE = 0

    HSYNC_HIGH_ACTIVE = False
    HSYNC_IDLE_HIGH = False

    VSYNC_HIGH_ACTIVE = False
    VSYNC_IDLE_HIGH = False

    DE_IDLE_HIGH = False
    DE_LOW_ACTIVE = False

    PCLK_IDLE_HIGH = False
    PCLK_FALLING = False

    H_BACK_PORCH = 0
    H_FRONT_PORCH = 0
    H_PULSE_WIDTH = 0

    V_BACK_PORCH = 0
    V_FRONT_PORCH = 0
    V_PULSE_WIDTH = 0

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
        spi_3wire=None,
        wait_pin=None,
        wait_state=STATE_HIGH
    ):
        if not isinstance(data_bus, lcd_bus.I80Bus):
            raise RuntimeError('Only the I8080 bus is supported by the driver')

        if wait_pin in (None, -1):
            self._wait_pin = None
        elif not isinstance(wait_pin, int):
            self._wait_pin = wait_pin
        else:
            self._wait_pin = machine.Pin(wait_pin, machine.Pin.IN)

        self._wait_state = wait_state

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
            spi_3wire=spi_3wire,
            spi_3wire_shared_pins=True,
            _sw_rotate=True,
            _init_bus=False
        )

    def _write_reg(self, reg, param):
        self._param_buf[0] = param
        self.set_params(reg, self._param_mv[:1])

    def _write_ssd2828(self, cmd, *params):
        params = [param & 0xFF for param in params]

        if params:
            self._param_buf[:2] = bytearray(params)
            self.set_params(cmd, self._param_mv[:2])
        else:
            self.set_params(cmd)

    def _spi_3wire_init(self):
        self._write_ssd2828(0xb7, 0x50, 0x00)
        self._write_ssd2828(0xb8, 0x00, 0x00)
        self._write_ssd2828(0xb9, 0x00, 0x00)
        self._write_ssd2828(0xBA, 0x14, 0x42)
        self._write_ssd2828(0xBB, 0x03, 0x00)
        self._write_ssd2828(0xb9, 0x01, 0x00)
        self._write_ssd2828(0xDE, 0x00, 0x00)
        self._write_ssd2828(0xc9, 0x02, 0x23)
        self._write_ssd2828(0xb7, 0x50, 0x00)
        self._write_ssd2828(0xb8, 0x00, 0x00)
        self._write_ssd2828(0xb9, 0x00, 0x00)
        self._write_ssd2828(0xBA, 0x2d, 0x82)
        self._write_ssd2828(0xBB, 0x07, 0x00)
        self._write_ssd2828(0xb9, 0x01, 0x00)
        self._write_ssd2828(0xc9, 0x02, 0x23)
        time.sleep_ms(100)  # NOQA
        self._write_ssd2828(0xCA, 0x01, 0x23)
        self._write_ssd2828(0xCB, 0x10, 0x05)
        self._write_ssd2828(0xCC, 0x05, 0x10)
        self._write_ssd2828(0xD0, 0x00, 0x00)
        self._write_ssd2828(0xB1, self.H_PULSE_WIDTH, self.V_PULSE_WIDTH)
        self._write_ssd2828(0xB2, self.H_BACK_PORCH, self.V_BACK_PORCH)
        self._write_ssd2828(0xB3, self.H_FRONT_PORCH, self.V_FRONT_PORCH)
        self._write_ssd2828(0xB4, 0x90, 0x01)
        self._write_ssd2828(0xB5, 0x00, 0x05)
        self._write_ssd2828(0xB6, 0x0b, 0xc0)
        self._write_ssd2828(0xDE, 0x03, 0x00)
        self._write_ssd2828(0xD6, 0x01, 0x00)
        self._write_ssd2828(0xDB, 0x58, 0x00)
        self._write_ssd2828(0xB7, 0x4B, 0x02)
        self._write_ssd2828(0x2c)

    def set_invert_colors(self, value):
        raise NotImplementedError

    @property
    def orientation(self):
        raise NotImplementedError

    @orientation.setter
    def orientation(self, value):
        raise NotImplementedError

    def _wait(self):
        start_time = time.ticks_ms()  # NOQA
        if self._wait_pin is not None:
            while (
                self._wait_pin.value() != self._wait_state and
                time.ticks_diff(time.ticks_ms(), start_time) < self.WAIT_TIMEOUT  # NOQA
            ):
                time.sleep_ms(1)  # NOQA
        else:
            while time.ticks_diff(time.ticks_ms(), start_time) < self.WAIT_TIMEOUT:  # NOQA
                time.sleep_ms(1)  # NOQA

    def reset(self):
        if self._reset_pin is None:
            self.set_params(_SRR)
            time.sleep_ms(20)  # NOQA
        else:
            display_driver_framework.DisplayDriver.reset(self)
            time.sleep_ms(50)  # NOQA

    def _set_memory_location(self, x_start, y_start, x_end, y_end):
        buf = self._param_buf
        mv = self._param_mv

        # set active window start X/Y
        buf[0] = x_start & 0xFF
        self.set_params(_AWUL_X0, mv[:1])
        buf[0] = (x_start >> 8) & 0xFF
        self.set_params(_AWUL_X1, mv[:1])
        buf[0] = y_start & 0xFF
        self.set_params(_AWUL_Y0, mv[:1])
        buf[0] = (y_start >> 8) & 0xFF
        self.set_params(_AWUL_Y1, mv[:1])

        # set active window width and height
        buf[0] = (x_end - x_start) & 0xFF
        self.set_params(_AW_WTH0, mv[:1])
        buf[0] = ((x_end - x_start) >> 8) & 0xFF
        self.set_params(_AW_WTH1, mv[:1])
        buf[0] = (y_end - y_start) & 0xFF
        self.set_params(_AW_HT0, mv[:1])
        buf[0] = ((y_end - y_start) >> 8) & 0xFF
        self.set_params(_AW_HT1, mv[:1])

        # set cursor
        buf[0] = x_start & 0xff
        self.set_params(_CURH0, mv[:1])
        buf[0] = (x_start >> 8) & 0xFF
        self.set_params(_CURH1, mv[:1])
        buf[0] = y_start & 0xFF
        self.set_params(_CURV0, mv[:1])
        buf[0] = (y_start >> 8) & 0xFF
        self.set_params(_CURV1, mv[:1])

        return _MRWDP
