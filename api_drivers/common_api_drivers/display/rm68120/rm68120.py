# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import display_driver_framework
from micropython import const  # NOQA


_CASET = const(0x2A00)
_PASET = const(0x2B00)
_RAMWR = const(0x2C00)
_MADCTL = const(0x3600)

STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class RM68120(display_driver_framework.DisplayDriver):
    _INVOFF = 0x2000
    _INVON = 0x2100

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

    def _on_size_change(self, _):
        rotation = self._disp_drv.get_rotation()
        self._width = self._disp_drv.get_horizontal_resolution()
        self._height = self._disp_drv.get_vertical_resolution()

        if rotation == self._rotation:
            return

        self._rotation = rotation

        if self._initilized:
            param_buf = bytearray([
                0x00,
                self._madctl(self._color_byte_order, ~rotation, self._ORIENTATION_TABLE)  # NOQA
            ])
            self._data_bus.tx_param(_MADCTL, param_buf)
