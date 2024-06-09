import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA
import display_driver_framework


_WRNVRAM = const(0xD0)
_NVPROTKEY = const(0xD1)
_RDNVSTAT = const(0xD2)
_PWRCTL1 = const(0xC0)
_VCOMCTL1 = const(0xC5)
_GAMMASET = const(0xC8)
_MADCTL = const(0x36)
_COLMOD = const(0x3A)
_CASET = const(0x2A)
_PASET = const(0x2B)
_DISPON = const(0x29)
_SLPOUT = const(0x11)
_IFMODE = const(0xB0)
_INVON = const(0x21)
_IFACECTRL = const(0xC6)
_PUMPRATIOCTRL = const(0xF7)
_IFACESET = const(0xB3)
_DISPMODE = const(0xB4)
_SWRESET = const(0x01)


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class ILI9481(display_driver_framework.DisplayDriver):

    def init(self, sequence=1):
        # There are 8 different init sequences, they are numbered 1-8.
        # if your display doesn't work try another sequence

        param_buf = bytearray(12)
        param_mv = memoryview(param_buf)

        self.set_params(_SWRESET)
        time.sleep_ms(280)  # NOQA
        self.set_params(_SLPOUT)

        param_buf[0] = 0x07

        if sequence in (1, 7):
            param_buf[1] = 0x42
            if sequence == 7:
                param_buf[2] = 0x1B
            else:
                param_buf[2] = 0x18
        elif sequence == 8:
            param_buf[1] = 0x44
            param_buf[2] = 0x1E
        else:
            if sequence == 4:
                param_buf[1] = 0x40
            else:
                param_buf[1] = 0x41

            param_buf[2] = 0x1D

        self.set_params(_WRNVRAM, param_mv[:3])

        param_buf[0] = 0x00
        if sequence == 1:
            param_buf[1] = 0x07
            param_buf[2] = 0x10
        elif sequence == 4:
            param_buf[1] = 0x18
            param_buf[2] = 0x13
        elif sequence == 7:
            param_buf[1] = 0x14
            param_buf[2] = 0x1B
        elif sequence == 8:
            param_buf[1] = 0x0C
            param_buf[2] = 0x1A
        else:
            if sequence in (2, 3, 6):
                param_buf[1] = 0x2B
            else:
                param_buf[1] = 0x1C
            param_buf[2] = 0x1F

        self.set_params(_NVPROTKEY, param_mv[:3])

        param_buf[0] = 0x01

        if sequence == 1:
            param_buf[1] = 0x02
        elif sequence == 7:
            param_buf[1] = 0x12
        else:
            param_buf[1] = 0x11

        self.set_params(_RDNVSTAT, param_mv[:2])

        if sequence == 6:
            param_buf[0] = 0x10
            param_buf[1] = 0x3B
            param_buf[2] = 0x00
            param_buf[3] = 0x02
            param_buf[4] = 0x11
            param_buf[5] = 0x00
            self.set_params(_PWRCTL1, param_mv[:6])
        else:
            if sequence == 8:
                param_buf[0] = 0x00
            else:
                param_buf[0] = 0x10

            param_buf[1] = 0x3B
            param_buf[2] = 0x00
            param_buf[3] = 0x02
            if sequence == 7:
                param_buf[4] = 0x01
            else:
                param_buf[4] = 0x11

            self.set_params(_PWRCTL1, param_mv[:5])

        param_buf[0] = 0x03
        self.set_params(_VCOMCTL1, param_mv[:1])

        if sequence in (5, 6, 8):
            if sequence == 6:
                param_buf[0] = 0x80
            else:
                param_buf[0] = 0x83

            self.set_params(_IFACECTRL, param_mv[:1])

        param_buf[0] = 0x00
        param_buf[8] = 0x77

        if sequence in (1, 2, 3, 5, 6, 7, 8):
            if sequence in (5, 8):
                param_buf[5] = 0x1F
            else:
                param_buf[5] = 0x16

            param_buf[11] = 0x00
        elif sequence == 4:
            param_buf[5] = 0x08
            param_buf[11] = 0x0C

        if sequence == 1:
            param_buf[1] = 0x32
            param_buf[2] = 0x36
            param_buf[3] = 0x45
            param_buf[4] = 0x06
            param_buf[6] = 0x37
            param_buf[7] = 0x75
            param_buf[9] = 0x54
            param_buf[10] = 0x0C
        elif sequence in (2, 3, 5, 6, 8):
            param_buf[4] = 0x00
            param_buf[9] = 0x00
            param_buf[10] = 0x0F

            if sequence in (5, 8):
                param_buf[1] = 0x26
                param_buf[2] = 0x21
                param_buf[3] = 0x00
                param_buf[6] = 0x65
                param_buf[7] = 0x23
            else:
                param_buf[1] = 0x14
                param_buf[2] = 0x33
                param_buf[3] = 0x10
                param_buf[6] = 0x44
                param_buf[7] = 0x36
        elif sequence == 4:
            param_buf[1] = 0x44
            param_buf[2] = 0x06
            param_buf[3] = 0x44
            param_buf[4] = 0x0A
            param_buf[6] = 0x17
            param_buf[7] = 0x33
            param_buf[9] = 0x44
            param_buf[10] = 0x08
        elif sequence == 7:
            param_buf[1] = 0x46
            param_buf[2] = 0x44
            param_buf[3] = 0x50
            param_buf[4] = 0x04
            param_buf[6] = 0x33
            param_buf[7] = 0x13
            param_buf[9] = 0x05
            param_buf[10] = 0x0F

        self.set_params(_GAMMASET, param_mv[:12])

        if sequence in (2, 3, 4, 5, 6, 8):
            if sequence != 8:
                param_buf[0] = 0x00
                self.set_params(_IFMODE, param_mv[:1])

            param_buf[0] = 0xA0
            self.set_params(0xE4, param_mv[:1])

            if sequence in (6, 8):
                param_buf[0] = 0x08
            else:
                param_buf[0] = 0x01

            self.set_params(0xF0, param_mv[:1])

            if sequence in (2, 3, 6, 8):
                if sequence == 2:
                    param_buf[0] = 0x02
                    param_buf[1] = 0x1A
                elif sequence == 8:
                    param_buf[0] = 0x00
                    param_buf[1] = 0x2A
                else:
                    param_buf[0] = 0x40
                    param_buf[1] = 0x0A

                self.set_params(0xF3, param_mv[:2])

        if sequence == 6:
            param_buf[0] = 0x84
            self.set_params(0xF6, param_mv[:1])

            param_buf[0] = 0x80
            self.set_params(_PUMPRATIOCTRL, param_mv[:1])

            param_buf[0] = 0x00
            param_buf[1] = 0x01
            param_buf[2] = 0x06
            param_buf[3] = 0x30
            self.set_params(_IFACESET, param_mv[:4])

            param_buf[0] = 0x00
            self.set_params(_DISPMODE, param_mv[:1])

        if sequence == 8:
            param_buf[0] = 0x02
            param_buf[1] = 0x00
            param_buf[2] = 0x00
            param_buf[3] = 0x01
            self.set_params(_DISPMODE, param_mv[:4])

        param_buf[0] = (
            self._madctl(
                self._color_byte_order,
                self._ORIENTATION_TABLE  # NOQA
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

        if not isinstance(self._data_bus, lcd_bus.I80Bus):
            self.set_params(_INVON)

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        param_buf[2] = 0x01
        param_buf[3] = 0x3F

        if sequence == 7:
            self.set_params(0x22, param_mv[:4])  # ??? CASET

            param_buf[0] = 0x00
            param_buf[1] = 0x00
            param_buf[2] = 0x01
            param_buf[3] = 0xE0
        else:
            self.set_params(_CASET, param_mv[:4])

            param_buf[0] = 0x00
            param_buf[1] = 0x00
            param_buf[2] = 0x01
            param_buf[3] = 0xDF

        self.set_params(_PASET, param_mv[:4])

        time.sleep_ms(120)  # NOQA
        self.set_params(_DISPON)
        time.sleep_ms(25)  # NOQA

        display_driver_framework.DisplayDriver.init(self)
