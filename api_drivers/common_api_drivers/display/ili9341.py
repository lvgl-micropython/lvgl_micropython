import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA
import display_driver_framework


_PWR1 = const(0xC0)
_PWR2 = const(0xC1)
_VCOMCTL1 = const(0xC5)
_VCOMCTL2 = const(0xC7)
_MADCTL = const(0x36)
_COLMOD = const(0x3A)
_FRMCTR1 = const(0xB1)
_DFUNCTRL = const(0xB6)
_GAMSET = const(0x26)
_PGC = const(0xE0)
_NGC = const(0xE1)
_SLPOUT = const(0x11)
_DISPON = const(0x29)
_RASET = const(0x2B)
_CASET = const(0x2A)
_PWRCTRLB = const(0xCF)
_PWRONSQCTRL = const(0xED)
_DRVTIMCTRLA1 = const(0xE8)
_PWRCTRLA = const(0xCB)
_PUMPRATIOCTRL = const(0xF7)
_DRVTIMCTRLB = const(0xEA)
_ENA3GAMMA = const(0xF2)


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class ILI9341(display_driver_framework.DisplayDriver):

    def __init__(
        self,
        data_bus,
        display_width,
        display_height,
        frame_buffer1=None,
        frame_buffer2=None,
        reset_pin=None,
        reset_state=STATE_LOW,
        power_pin=None,
        power_on_state=STATE_HIGH,
        backlight_pin=None,
        backlight_on_state=STATE_HIGH,
        offset_x=0,
        offset_y=0,
        color_byte_order=BYTE_ORDER_RGB,  # NOQA
        color_space=lv.COLOR_FORMAT.RGB565,
        rgb565_byte_swap=False
    ):

        if (
            color_space == lv.COLOR_FORMAT.RGB565 and
            isinstance(data_bus, (lcd_bus.SPIBus, lcd_bus.I80Bus))
        ):
            rgb565_byte_swap = True

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
            color_byte_order=BYTE_ORDER_BGR,
            color_space=color_space,
            rgb565_byte_swap=rgb565_byte_swap
        )

    def init(self, sequence=1):
        param_buf = bytearray(15)
        param_mv = memoryview(param_buf)

        if sequence == 1:
            param_buf[0] = 0x03
            param_buf[1] = 0x80
            param_buf[2] = 0x02
            self.set_params(0xEF, param_mv[:3])

        param_buf[0] = 0x00
        param_buf[1] = 0XC1
        param_buf[2] = 0X30
        self.set_params(_PWRCTRLB, param_mv[:3])

        param_buf[0] = 0x64
        param_buf[1] = 0x03
        param_buf[2] = 0X12
        param_buf[3] = 0X81
        self.set_params(_PWRONSQCTRL, param_mv[:4])

        param_buf[0] = 0x85
        param_buf[1] = 0x00
        param_buf[2] = 0x78
        self.set_params(_DRVTIMCTRLA1, param_mv[:3])

        param_buf[0] = 0x39
        param_buf[1] = 0x2C
        param_buf[2] = 0x00
        param_buf[3] = 0x34
        param_buf[4] = 0x02
        self.set_params(_PWRCTRLA, param_mv[:5])

        param_buf[0] = 0x20
        self.set_params(_PUMPRATIOCTRL, param_mv[:1])

        param_buf[0] = 0x00
        param_buf[1] = 0x00
        self.set_params(_DRVTIMCTRLB, param_mv[:2])

        if sequence == 1:
            param_buf[0] = 0x23
        else:
            param_buf[0] = 0x10
        self.set_params(_PWR1, param_mv[:1])

        if sequence == 1:
            param_buf[0] = 0x10
        else:
            param_buf[0] = 0x00

        self.set_params(_PWR2, param_mv[:1])

        if sequence == 1:
            param_buf[0] = 0x3e
            param_buf[1] = 0x28
        else:
            param_buf[0] = 0x30
            param_buf[1] = 0x30

        self.set_params(_VCOMCTL1, param_mv[:2])

        if sequence == 1:
            param_buf[0] = 0x86
        else:
            param_buf[0] = 0xB7
        self.set_params(_VCOMCTL2, param_mv[:1])

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

        param_buf[0] = 0x00

        if sequence == 1:
            param_buf[1] = 0x13  # 0x18 ??
        else:
            param_buf[1] = 0x1A
        self.set_params(_FRMCTR1, param_mv[:2])

        param_buf[0] = 0x08
        param_buf[1] = 0x82
        param_buf[2] = 0x27
        self.set_params(_DFUNCTRL, param_mv[:3])

        param_buf[0] = 0x00
        self.set_params(_ENA3GAMMA, param_mv[:1])

        param_buf[0] = 0x01
        self.set_params(_GAMSET, param_mv[:1])

        param_buf[0] = 0x0F
        param_buf[4] = 0x0E
        param_buf[5] = 0x08
        param_buf[14] = 0x00

        if sequence == 1:
            param_buf[1] = 0x31
            param_buf[2] = 0x2B
            param_buf[3] = 0x0C
            param_buf[6] = 0x4E
            param_buf[7] = 0xF1
            param_buf[8] = 0x37
            param_buf[9] = 0x07
            param_buf[10] = 0x10
            param_buf[11] = 0x03
            param_buf[12] = 0x0E
            param_buf[13] = 0x09
        else:
            param_buf[1] = 0x2A
            param_buf[2] = 0x28
            param_buf[3] = 0x08
            param_buf[6] = 0x54
            param_buf[7] = 0xA9
            param_buf[8] = 0x43
            param_buf[9] = 0x0A
            param_buf[10] = 0x0F
            param_buf[11] = 0x00
            param_buf[12] = 0x00
            param_buf[13] = 0x00

        self.set_params(_PGC, param_mv[:15])

        param_buf[0] = 0x00
        param_buf[4] = 0x11
        param_buf[14] = 0x0F

        if sequence == 1:
            param_buf[1] = 0x0E
            param_buf[2] = 0x14
            param_buf[3] = 0x03
            param_buf[5] = 0x07
            param_buf[6] = 0x31
            param_buf[7] = 0xC1
            param_buf[8] = 0x48
            param_buf[9] = 0x08
            param_buf[10] = 0x0F
            param_buf[11] = 0x0C
            param_buf[12] = 0x31
            param_buf[13] = 0x36
        else:
            param_buf[1] = 0x15
            param_buf[2] = 0x17
            param_buf[3] = 0x07
            param_buf[5] = 0x06
            param_buf[6] = 0x2B
            param_buf[7] = 0x56
            param_buf[8] = 0x3C
            param_buf[9] = 0x05
            param_buf[10] = 0x10
            param_buf[11] = 0x0F
            param_buf[12] = 0x3F
            param_buf[13] = 0x3F

        self.set_params(_NGC, param_mv[:15])

        if sequence != 1:
            param_buf[0] = 0x00
            param_buf[1] = 0x00
            param_buf[2] = 0x01
            param_buf[3] = 0x3f
            self.set_params(_RASET, param_mv[:4])

            param_buf[0] = 0x00
            param_buf[1] = 0x00
            param_buf[2] = 0x00
            param_buf[3] = 0xef
            self.set_params(_CASET, param_mv[:4])

        self.set_params(_SLPOUT)
        time.sleep_ms(120)  # NOQA
        self.set_params(_DISPON)
        time.sleep_ms(20)  # NOQA

        display_driver_framework.DisplayDriver.init(self)
