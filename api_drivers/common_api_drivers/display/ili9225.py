import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA
import display_driver_framework


_PWRCTRL1 = const(0x10)
_PWRCTRL2 = const(0x11)
_PWRCTRL3 = const(0x12)
_PWRCTRL4 = const(0x13)
_PWRCTRL5 = const(0x14)
_DRVROUTCTRL = const(0x01)
_ACDRVCTRL = const(0x02)
_ENTRYMODE = const(0x03)
_DISPCTRL1 = const(0x07)
_BLKPERDCTRL1 = const(0x08)
_FRAMECYLCTRL = const(0x0B)
_IFACECTRL = const(0x0C)
_OSCCTRL = const(0x0F)
_VCIREC = const(0x15)
_RAMADDRSET1 = const(0x20)
_RAMADDRSET2 = const(0x21)
_GSCANCTRL = const(0x30)
_VSCRLCTRL1 = const(0x31)
_VSCRLCTRL2 = const(0x32)
_VSCRLCTRL3 = const(0x33)
_PARDRVPOS1 = const(0x34)
_PARDRVPOS2 = const(0x35)
_HWINADDR1 = const(0x36)
_HWINADDR2 = const(0x37)
_VWINADDR1 = const(0x38)
_VWINADDR2 = const(0x39)
_GAMMACTRL1 = const(0x50)
_GAMMACTRL2 = const(0x51)
_GAMMACTRL3 = const(0x52)
_GAMMACTRL4 = const(0x53)
_GAMMACTRL5 = const(0x54)
_GAMMACTRL6 = const(0x55)
_GAMMACTRL7 = const(0x56)
_GAMMACTRL8 = const(0x57)
_GAMMACTRL9 = const(0x58)
_GAMMACTRL10 = const(0x59)

_RAMWR = const(0x22)


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

    def set_rotation(self, value):
        rot0 = lv.DISPLAY_ROTATION._0
        rot90 = lv.DISPLAY_ROTATION._90
        rot180 = lv.DISPLAY_ROTATION._180
        rot270 = lv.DISPLAY_ROTATION._270

        if (
            (
                self._rotation in (rot0, rot180) and
                value in (rot90, rot270)
            ) or (
                self._rotation in (rot90, rot270) and
                value in (rot0, rot180)
            )
        ):
            width = self._disp_drv.get_horizontal_resolution()
            height = self._disp_drv.get_vertical_resolution()
            self._disp_drv.set_resolution(height, width)

            self._offset_x, self._offset_y = self._offset_y, self._offset_x

        self._rotation = value

        if self._initilized:
            if value <= 1:
                value = int(not value)

            self._param_buf[0] = value
            self._param_buf[1] = 0x1C
            self.set_params(_DRVROUTCTRL, self._param_mv[:2])

            self._param_buf[0] = self._color_byte_order

            if value in (1, 2):
                self._param_buf[1] = 0x30
            else:
                self._param_buf[1] = 0x38

            self.set_params(_ENTRYMODE, self._param_mv[:2])

    def init(self):
        param_buf = bytearray(2)
        param_mv = memoryview(param_buf)

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_PWRCTRL1, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_PWRCTRL2, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_PWRCTRL3, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_PWRCTRL4, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_PWRCTRL5, param_mv[:2])

        time.sleep_ms(40)  # NOQA

        param_buf[0] = 0x00
        param_buf[1] = 0x18
        self.set_params(_PWRCTRL2, param_mv[:2])

        param_buf[0] = 0x61
        param_buf[1] = 0x21
        self.set_params(_PWRCTRL3, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x6F
        self.set_params(_PWRCTRL4, param_mv[:2])

        param_buf[0] = 0x49
        param_buf[1] = 0x5F
        self.set_params(_PWRCTRL5, param_mv[:2])

        param_buf[0] = 0x08
        param_buf[1] = 0x00
        self.set_params(_PWRCTRL1, param_mv[:2])

        time.sleep_ms(10)  # NOQA

        param_buf[0] = 0x10
        param_buf[1] = 0x3B
        self.set_params(_PWRCTRL2, param_mv[:2])

        time.sleep_ms(50)  # NOQA

        param_buf[0] = 0x01
        param_buf[1] = 0x00
        self.set_params(_ACDRVCTRL, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_DISPCTRL1, param_mv[:2])

        param_buf[0] = 0x08
        param_buf[1] = 0x08
        self.set_params(_BLKPERDCTRL1, param_mv[:2])

        param_buf[0] = 0x11
        param_buf[1] = 0x00
        self.set_params(_FRAMECYLCTRL, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_IFACECTRL, param_mv[:2])

        param_buf[0] = 0x0D
        param_buf[1] = 0x01
        self.set_params(_OSCCTRL, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x20
        self.set_params(_VCIREC, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_RAMADDRSET1, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_RAMADDRSET2, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_GSCANCTRL, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0xDB
        self.set_params(_VSCRLCTRL1, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_VSCRLCTRL2, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_VSCRLCTRL3, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0xDB
        self.set_params(_PARDRVPOS1, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_PARDRVPOS2, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0xAF
        self.set_params(_HWINADDR1, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_HWINADDR2, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0xDB
        self.set_params(_VWINADDR1, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_VWINADDR2, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_GAMMACTRL1, param_mv[:2])

        param_buf[0] = 0x08
        param_buf[1] = 0x08
        self.set_params(_GAMMACTRL2, param_mv[:2])

        param_buf[0] = 0x08
        param_buf[1] = 0x0A
        self.set_params(_GAMMACTRL3, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x0A
        self.set_params(_GAMMACTRL4, param_mv[:2])

        param_buf[0] = 0x0A
        param_buf[1] = 0x08
        self.set_params(_GAMMACTRL5, param_mv[:2])

        param_buf[0] = 0x08
        param_buf[1] = 0x08
        self.set_params(_GAMMACTRL6, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_GAMMACTRL7, param_mv[:2])

        param_buf[0] = 0x0A
        param_buf[1] = 0x00
        self.set_params(_GAMMACTRL8, param_mv[:2])

        param_buf[0] = 0x07
        param_buf[1] = 0x10
        self.set_params(_GAMMACTRL9, param_mv[:2])

        param_buf[0] = 0x07
        param_buf[1] = 0x10
        self.set_params(_GAMMACTRL10, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x12
        self.set_params(_DISPCTRL1, param_mv[:2])

        time.sleep_ms(50)  # NOQA

        param_buf[0] = 0x10
        param_buf[1] = 0x17
        self.set_params(_DISPCTRL1, param_mv[:2])

        display_driver_framework.DisplayDriver.init(self)
