import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA
import display_driver_framework


_SWRESET = const(0x01)
_SLPOUT = const(0x11)
_DISPON = const(0x29)
_COLMOD = const(0x3A)
_MADCTL = const(0x36)
_TEON = const(0x35)
_TEARLINE = const(0x44)
_SETOSC = const(0xB0)
_SETPWR1 = const(0xB1)
_SETRGB = const(0xB3)
_SETCOM = const(0xB6)
_SETCYC = const(0xB4)
_SETC = const(0xB9)
_SETSTBA = const(0xC0)
_SETPANEL = const(0xCC)
_SETGAMMA = const(0xE0)

STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class HX8357D(display_driver_framework.DisplayDriver):

    def init(self):
        param_buf = bytearray(34)
        param_mv = memoryview(param_buf)

        time.sleep_ms(300)
        param_buf[0] = 0xFF
        param_buf[1] = 0x83
        param_buf[2] = 0x57
        self.set_params(_SETC, param_mv[:3])

        param_buf[0] = 0x80
        self.set_params(_SETRGB, param_mv[:1])

        param_buf[0] = 0x0
        param_buf[1] = 0x06
        param_buf[2] = 0x06
        param_buf[3] = 0x25
        self.set_params(_SETCOM, param_mv[:4])

        param_buf[0] = 0x68
        self.set_params(_SETOSC, param_mv[:1])

        param_buf[0] = 0x05
        self.set_params(_SETPANEL, param_mv[:1])

        param_buf[0] = 0x00
        param_buf[1] = 0x15
        param_buf[2] = 0x1C
        param_buf[3] = 0x1C
        param_buf[4] = 0x83
        param_buf[5] = 0xAA
        self.set_params(_SETPWR1, param_mv[:6])

        param_buf[0] = 0x50
        param_buf[1] = 0x50
        param_buf[2] = 0x01
        param_buf[3] = 0x3C
        param_buf[4] = 0x1E
        param_buf[5] = 0x08
        self.set_params(_SETSTBA, param_mv[:6])

        param_buf[0] = 0x02
        param_buf[1] = 0x40
        param_buf[2] = 0x00
        param_buf[3] = 0x2A
        param_buf[4] = 0x2A
        param_buf[5] = 0x0D
        param_buf[6] = 0x78
        self.set_params(_SETCYC, param_mv[:7])

        param_buf[0] = 0x02
        param_buf[1] = 0x0A
        param_buf[2] = 0x11
        param_buf[3] = 0x1d
        param_buf[4] = 0x23
        param_buf[5] = 0x35
        param_buf[6] = 0x41
        param_buf[7] = 0x4b
        param_buf[8] = 0x4b
        param_buf[9] = 0x42
        param_buf[10] = 0x3A
        param_buf[11] = 0x27
        param_buf[12] = 0x1B
        param_buf[13] = 0x08
        param_buf[14] = 0x09
        param_buf[15] = 0x03
        param_buf[16] = 0x02
        param_buf[17] = 0x0A
        param_buf[18] = 0x11
        param_buf[19] = 0x1d
        param_buf[20] = 0x23
        param_buf[21] = 0x35
        param_buf[22] = 0x41
        param_buf[23] = 0x4b
        param_buf[24] = 0x4b
        param_buf[25] = 0x42
        param_buf[26] = 0x3A
        param_buf[27] = 0x27
        param_buf[28] = 0x1B
        param_buf[29] = 0x08
        param_buf[30] = 0x09
        param_buf[31] = 0x03
        param_buf[32] = 0x00
        param_buf[33] = 0x01
        self.set_params(_SETGAMMA, param_mv[:34])

        param_buf[0] = (
            self._madctl(
                self._color_byte_order,
                display_driver_framework._ORIENTATION_TABLE  # NOQA
            )
        )
        self.set_params(_MADCTL, param_mv[:1])

        color_size = lv.color_format_get_size(self._color_space)
        if color_size == 2:  # NOQA
            pixel_format = 0x55
        else:
            raise RuntimeError(
                f'{self.__class__.__name__} IC only supports '
                'lv.COLOR_FORMAT.RGB565'
            )

        param_buf[0] = pixel_format
        self.set_params(_COLMOD, param_mv[:1])

        param_buf[0] = 0x00
        self.set_params(_TEON, param_mv[:1])

        param_buf[0] = 0x00
        param_buf[1] = 0x02
        self.set_params(_TEARLINE, param_mv[:2])

        time.sleep_ms(150)
        self.set_params(_SLPOUT)

        time.sleep_ms(50)
        self.set_params(_DISPON)

        display_driver_framework.DisplayDriver.init(self)

