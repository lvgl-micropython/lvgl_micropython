# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import rgb_display_framework  # NOQA
import lvgl as lv


STATE_HIGH = rgb_display_framework.STATE_HIGH
STATE_LOW = rgb_display_framework.STATE_LOW
STATE_PWM = rgb_display_framework.STATE_PWM

BYTE_ORDER_RGB = rgb_display_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = rgb_display_framework.BYTE_ORDER_BGR


class RGBDisplay(rgb_display_framework.RGBDisplayDriver):

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
    ):

        self._spi_3wire = None
        self._bus_shared_pins = False

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
            _cmd_bits=8,
            _param_bits=8,
            _init_bus=True
        )

    def init(self):
        rgb_display_framework.RGBDisplayDriver.init(self, None)
