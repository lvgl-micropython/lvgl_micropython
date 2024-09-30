import display_driver_framework
from micropython import const  # NOQA

import lvgl as lv
import time

TYPE_A = 1

STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class HX8369(display_driver_framework.DisplayDriver):

    def reset(self):
        if self._reset_pin is None:
            return

        self._reset_pin.value(self._reset_state)
        time.sleep_ms(25)  # NOQA
        self._reset_pin.value(not self._reset_state)
        time.sleep_ms(50)  # NOQA

    def set_brightness(self, value):
        value = int(value / 100.0 * 255)

        if 255 < value:
            mapped_level = 255
        elif value >= 102:
            mapped_level = 52 + (value - 102) * (255 - 52) / (255 - 102)
        else:  # value < 102:
            mapped_level = 52 - (102 - value) * 52 / 102

        self._param_buf[0] = int(mapped_level)
        self.set_params(0x51, self._param_mv[:1])
