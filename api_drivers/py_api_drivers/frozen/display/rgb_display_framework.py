# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import lvgl as lv
import display_driver_framework

STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class RGBDisplayDriver(display_driver_framework.DisplayDriver):

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
        rgb565_byte_swap=False,
        spi_3wire=None,
        spi_3wire_shared_pins=False,
        _cmd_bits=8,
        _param_bits=8,
        _init_bus=True
    ):

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
            color_space=color_space,
            rgb565_byte_swap=rgb565_byte_swap,
            _cmd_bits=_cmd_bits,
            _param_bits=_param_bits,
            _init_bus=_init_bus
        )

    def set_params(self, cmd, params=None):
        if self._spi_3wire is not None:
            self._spi_3wire.tx_param(cmd, params)
        else:
            display_driver_framework.DisplayDriver.set_params(self, cmd, params)

    def get_params(self, cmd, params):
        pass

    def _set_memory_location(self, x1, y1, x2, y2):  # NOQA
        return -1

    def _spi_3wire_init(self, *args, **kwargs):
        raise NotImplementedError

    def init(self, type=None):  # NOQA
        if self._spi_3wire is not None:
            self._spi_3wire.init(self._cmd_bits, self._param_bits)

            self._spi_3wire_init(type)

            if self._bus_shared_pins:
                # shut down the spi3wire prior to initilizing the data bus.
                # so we don't have a conflict between the bus and the 3wire
                self._spi_3wire.deinit()

        if not self._init_disp_bus:
            self._init_bus()  # NOQA

        self._initilized = True
