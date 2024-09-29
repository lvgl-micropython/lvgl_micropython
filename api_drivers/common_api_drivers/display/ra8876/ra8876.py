import time
from micropython import const  # NOQA
import machine  # NOQA
import sys

import lvgl as lv
import lcd_bus  # NOQA
import display_driver_framework


_CMDWRITE = const(0x00)
_DATAWRITE = const(0x80)

# Software Reset Register (SRR)
_SRR = const(0x00)

# Memory Data Read/Write Port (MRWDP)
_MRWDP = const(0x04)

# Active Window Upper-Left corner X-coordinates 0 (AWUL_X0)
_AWUL_X0 = const(0x56)
# Active Window Upper-Left corner X-coordinates 1 (AWUL_X1)
_AWUL_X1 = const(0x57)
# Active Window Upper-Left corner Y-coordinates 0 (AWUL_Y0)
_AWUL_Y0 = const(0x58)
# Active Window Upper-Left corner Y-coordinates 1 (AWUL_Y1)
_AWUL_Y1 = const(0x59)

# Active Window Width 0 (AW_WTH0)
_AW_WTH0 = const(0x5A)
# Active Window Width 1 (AW_WTH1)
_AW_WTH1 = const(0x5B)

# Active Window Height 0 (AW_HT0)
_AW_HT0 = const(0x5C)
# Active Window Height 1 (AW_HT1)
_AW_HT1 = const(0x5D)

# Graphic Read/Write position Horizontal Position Register 0 (CURH0)
_CURH0 = const(0x5F)
# Graphic Read/Write position Horizontal Position Register 1 (CURH1)
_CURH1 = const(0x60)
# Graphic Read/Write position Vertical Position Register 0 (CURV0)
_CURV0 = const(0x61)
# Graphic Read/Write position Vertical Position Register 1 (CURV1)
_CURV1 = const(0x62)

STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class RA8876(display_driver_framework.DisplayDriver):
    display_name = 'RA8876'
    WAIT_TIMEOUT = 100

    def __init__(
        self,
        data_bus,
        spi_3wire,
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
        wait_state=STATE_HIGH,
        spi_3wire_shared_pins=False,
    ):
        if not isinstance(data_bus, lcd_bus.I80Bus):
            raise RuntimeError('Only the I8080 bus is supported by the driver')

        if wait_pin is None:
            self._wait_pin = None
        else:
            self._wait_pin = machine.Pin(wait_pin, machine.Pin.IN)
        self._wait_state = wait_state

        self._spi_3wire = spi_3wire
        self._bus_shared_pins = spi_3wire_shared_pins

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
            rgb565_byte_swap=rgb565_byte_swap,
            _cmd_bits=8,
            _param_bits=8,
            _init_bus=False
        )

    def invert_colors(self):
        raise NotImplementedError

    @property
    def orientation(self):
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
            self._reset_pin(self._reset_state)
            time.sleep_ms(1)  # NOQA
            self._reset_pin(not self._reset_state)
            time.sleep_ms(10)  # NOQA

    def _set_memory_location(self, x_start, y_start, x_end, y_end):
        # set active window start X/Y
        self.set_params(_AWUL_X0, x_start & 0xFF)
        self.set_params(_AWUL_X1, (x_start >> 8) & 0xFF)
        self.set_params(_AWUL_Y0, y_start & 0xFF)
        self.set_params(_AWUL_Y1, (y_start >> 8) & 0xFF)

        # set active window width and height
        self.set_params(_AW_WTH0, (x_end - x_start) & 0xFF)
        self.set_params(_AW_WTH1, ((x_end - x_start) >> 8) & 0xFF)
        self.set_params(_AW_HT0, (y_end - y_start) & 0xFF)
        self.set_params(_AW_HT1, ((y_end - y_start) >> 8) & 0xFF)

        # set cursor
        self.set_params(_CURH0, x_start & 0xff)
        self.set_params(_CURH1, (x_start >> 8) & 0xFF)
        self.set_params(_CURV0, y_start & 0xFF)
        self.set_params(_CURV1, (y_start >> 8) & 0xFF)

        return _MRWDP

    def init(self, type):  # NOQA
        self._spi_3wire.init(self._cmd_bits, self._param_bits)

        mod_name = f'_{self.__class__.__name__.lower()}_init_type{type}'

        mod = __import__(mod_name)
        mod.init()
        del sys.modules[mod_name]

        if self._bus_shared_pins:
            # shut down the spi3wire prior to initilizing the data bus.
            # so we don't have a conflict between the bus and the 3wire
            self._spi_3wire.deinit()

        self._init_bus()  # NOQA
        self._initilized = True

    def get_params(self, cmd, params):
        pass

    def set_params(self, cmd, params=None):
        if params is None:
            self._param_buf[0] = cmd
            self._spi_3wire.tx_param(_CMDWRITE, self._param_mv[:1])
        else:
            if isinstance(params, int):
                self._param_buf[0] = cmd
                self._spi_3wire.tx_param(_CMDWRITE, self._param_mv[:1])

                self._param_buf[0] = params
                self._spi_3wire.tx_param(_DATAWRITE, self._param_mv[:1])
            else:
                self._param_buf[3] = cmd
                self._spi_3wire.tx_param(_CMDWRITE, self._param_mv[3:4])
                self._spi_3wire.tx_param(_DATAWRITE, params)
