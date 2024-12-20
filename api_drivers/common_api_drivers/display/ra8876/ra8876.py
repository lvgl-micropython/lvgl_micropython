import time
from micropython import const  # NOQA
import machine  # NOQA

import lvgl as lv
import lcd_bus  # NOQA
import display_driver_framework

TYPE_ER_TFTM0784_2 = 1


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


#  Software Reset Register (SRR)
_SRR = const(0x00)

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

#  Graphic Read/Write position Horizontal Position Register 0 (CURH0)
_CURH0 = const(0x5F)
#  Graphic Read/Write position Horizontal Position Register 1 (CURH1)
_CURH1 = const(0x60)
#  Graphic Read/Write position Vertical Position Register 0 (CURV0)
_CURV0 = const(0x61)
#  Graphic Read/Write position Vertical Position Register 1 (CURV1)
_CURV1 = const(0x62)

#  Memory Data Read/Write Port (MRWDP)
_MRWDP = const(0x04)


class RA8876(display_driver_framework.DisplayDriver):
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

        if not isinstance(data_bus, lcd_bus.I80Bus):
            raise RuntimeError('Only the I8080 bus is supported by the driver')

        if wait_pin is None:
            self._wait_pin = None
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
            color_space=lv.COLOR_FORMAT.RGB565,
            rgb565_byte_swap=rgb565_byte_swap
        )

    def set_color_inversion(self, value):
        raise NotImplementedError

    def _wait(self):
        if self._wait_pin is not None:
            start_time = time.ticks_ms()  # NOQA

            while (
                self._wait_pin.value() != self._wait_state and
                time.ticks_diff(time.ticks_ms(), start_time) < self.WAIT_TIMEOUT  # NOQA
            ):
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
        mv = self._param_mv[:1]

        # set active window start X/Y
        buf[0] = x_start & 0xFF
        self.set_params(_AWUL_X0, mv)
        buf[0] = (x_start >> 8) & 0xFF
        self.set_params(_AWUL_X1, mv)
        buf[0] = y_start & 0xFF
        self.set_params(_AWUL_Y0, mv)
        buf[0] = (y_start >> 8) & 0xFF
        self.set_params(_AWUL_Y1, mv)

        # set active window width and height
        buf[0] = (x_end - x_start) & 0xFF
        self.set_params(_AW_WTH0, mv)
        buf[0] = ((x_end - x_start) >> 8) & 0xFF
        self.set_params(_AW_WTH1, mv)
        buf[0] = (y_end - y_start) & 0xFF
        self.set_params(_AW_HT0, mv)
        buf[0] = ((y_end - y_start) >> 8) & 0xFF
        self.set_params(_AW_HT1, mv)

        # set cursor
        buf[0] = x_start & 0xff
        self.set_params(_CURH0, mv)
        buf[0] = (x_start >> 8) & 0xFF
        self.set_params(_CURH1, mv)
        buf[0] = y_start & 0xFF
        self.set_params(_CURV0, mv)
        buf[0] = (y_start >> 8) & 0xFF
        self.set_params(_CURV1, mv)

        return _MRWDP
