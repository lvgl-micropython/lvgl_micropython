import display_driver_framework
from micropython import const  # NOQA

import lvgl as lv

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

    def _set_memory_location(self, x1, y1, x2, y2):
        # Column addresses
        param_buf = self._param_buf  # NOQA

        param_buf[0] = (x1 >> 8) & 0xFF
        param_buf[1] = x1 & 0xFF
        param_buf[2] = (x2 >> 8) & 0xFF
        param_buf[3] = x2 & 0xFF

        self._data_bus.tx_param(0x2A, self._param_mv)

        # Page addresses
        param_buf[0] = (y1 >> 8) & 0xFF
        param_buf[1] = y1 & 0xFF
        param_buf[2] = (y2 >> 8) & 0xFF
        param_buf[3] = y2 & 0xFF

        self._data_bus.tx_param(0x2B, self._param_mv)
        self._data_bus.tx_param(0x29, self._param_mv)

        return 0x2C

    def get_backlight(self):
        raise NotImplementedError

    def set_backlight(self, value):
        value = int(value / 100.0 * 255)

        if value > 255:
            mapped_level = 255

        if 255 > value >= 102:
            mapped_level = 52 + (value - 102) * (255 - 52) / (255 - 102)
        elif value < 102:
            mapped_level = 52 - (102 - value) * 52 / 102

        self._param_buf[0] = mapped_level
        self.set_params(0x51, self._param_mv[:1])
