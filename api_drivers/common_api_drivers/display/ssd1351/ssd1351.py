# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import display_driver_framework


_CASET = const(0x15)
_PASET = const(0x75)
_RAMWR = const(0x5C)

STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class SSD1351(display_driver_framework.DisplayDriver):
    _INVOFF = 0xA6
    _INVON = 0xA7

    def _set_memory_location(self, x1, y1, x2, y2):
        # Column addresses
        param_buf = self._param_buf

        param_buf[0] = (x1 >> 8) & 0xFF
        param_buf[1] = x1 & 0xFF
        param_buf[2] = (x2 >> 8) & 0xFF
        param_buf[3] = x2 & 0xFF

        self._data_bus.tx_param(_CASET, self._param_mv[:4])

        # Page addresses
        param_buf[0] = (y1 >> 8) & 0xFF
        param_buf[1] = y1 & 0xFF
        param_buf[2] = (y2 >> 8) & 0xFF
        param_buf[3] = y2 & 0xFF

        self._data_bus.tx_param(_PASET, self._param_mv[:4])

        return _RAMWR
