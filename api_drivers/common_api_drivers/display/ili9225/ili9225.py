# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import display_driver_framework
from micropython import const  # NOQA


_RAMWR = const(0x22)
_HWINADDR1 = const(0x36)
_HWINADDR2 = const(0x37)
_VWINADDR1 = const(0x38)
_VWINADDR2 = const(0x39)
_RAMADDRSET1 = const(0x20)
_RAMADDRSET2 = const(0x21)
_ENTRYMODE = const(0x03)
_DRVROUTCTRL = const(0x01)


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = 0x00
BYTE_ORDER_BGR = 0x10


class ILI9225(display_driver_framework.DisplayDriver):

    def _set_memory_location(self, x1, y1, x2, y2):
        # if self._rotation & 0x01:
        #     x1, y1 = y1, x1
        #     x2, y2 = y2, x2

        # Column addresses
        param_buf = self._param_buf

        param_buf[0] = (x1 >> 8) & 0xFF
        param_buf[1] = x1 & 0xFF
        self._data_bus.tx_param(_HWINADDR1, self._param_mv[:2])

        param_buf[0] = (x2 >> 8) & 0xFF
        param_buf[1] = x2 & 0xFF
        self._data_bus.tx_param(_HWINADDR2, self._param_mv[:2])

        # Page addresses
        param_buf[0] = (y1 >> 8) & 0xFF
        param_buf[1] = y1 & 0xFF
        self._data_bus.tx_param(_VWINADDR1, self._param_mv[:2])

        param_buf[0] = (y2 >> 8) & 0xFF
        param_buf[1] = y2 & 0xFF
        self._data_bus.tx_param(_VWINADDR2, self._param_mv[:2])

        param_buf[0] = (x1 >> 8) & 0xFF
        param_buf[1] = x1 & 0xFF
        self._data_bus.tx_param(_RAMADDRSET1, self._param_mv[:2])

        param_buf[0] = (y1 >> 8) & 0xFF
        param_buf[1] = y1 & 0xFF
        self._data_bus.tx_param(_RAMADDRSET2, self._param_mv[:2])

        return _RAMWR

    def _on_size_change(self, _):
        rotation = self._disp_drv.get_rotation()
        self._width = self._disp_drv.get_horizontal_resolution()
        self._height = self._disp_drv.get_vertical_resolution()

        if rotation == self._rotation:
            return

        self._rotation = rotation

        if self._initilized:
            if rotation <= 1:
                rotation = int(not rotation)

            self._param_buf[0] = rotation
            self._param_buf[1] = 0x1C
            self.set_params(_DRVROUTCTRL, self._param_mv[:2])

            self._param_buf[0] = self._color_byte_order

            if rotation in (1, 2):
                self._param_buf[1] = 0x30
            else:
                self._param_buf[1] = 0x38

            self.set_params(_ENTRYMODE, self._param_mv[:2])
