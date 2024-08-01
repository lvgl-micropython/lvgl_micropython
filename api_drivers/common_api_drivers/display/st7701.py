#
# This display is a combination of SPI and RGB display. You need to create
# an additional oibject to pass to the display driver.
#
# It will be along the lines of...
#
#
# import lcd_bus
#
#
# spi3wire = lcd_bus.SPI3Wire(
#     sclk=10,
#     mosi=11,
#     use_dc_bit=True,
#     dc_zero_on_data=False,
#     lsb_first=False,
#     cs_high_active=False
#     keep_cs_inactive=True
# )
#
# display = ST7701(
#     display_bus,
#     spi3wire
#     width,
#     height,
#     ...
#     color_space=lv.COLOR_FORMAT.RGB565,
#     # this is if you are sharing the rgb data lines with the 3wire
#     bus_shared_pins=False
# )
#
#
# display.set_power(True)
# display.init()
# display.set_backlight(100)
#
# and that's it...


import time
from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA
import display_driver_framework


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR

# SET _CND2BKxSEL TO 0X00

_SWRESET = const(0x01)
_SLPIN = const(0x10)
_SLPOUT = const(0x11)
_PTLON = const(0x12)
_NORON = const(0x13)
_ALLPOFF = const(0x22)
_ALLPON = const(0x23)
_GAMSET = const(0x26)
_DISPOFF = const(0x28)
_DISPON = const(0x29)
_TEOFF = const(0x34)
_TEON = const(0x35)
_MADCTL = const(0x36)
_IDMOFF = const(0x38)
_IDMON = const(0x39)
_COLMOD = const(0x3A)
_WRDISBV = const(0x51)
_WRCTRLD = const(0x53)
_WRCACE = const(0x55)
_WRCABCMB = const(0x5E)

_CND2BKxSEL = const(0xFF)

# Command2_BK0
# SET _CND2BKxSEL TO 0X10
_PVGAMCTRL = const(0xB000)
_NVGAMCTRL = const(0xB100)
_DGMEN = const(0xB800)
_DGMLUTR = const(0xB900)
_DGMLUTB = const(0xBA00)
_PWM_CLK = const(0xBC00)
_LNESET = const(0xC000)
_PORCTRL = const(0xC100)
_INVSET = const(0xC200)
_RGBCTRL = const(0xC300)
_PARCTRL = const(0xC500)
_SDIR = const(0xC700)
_PDOSET = const(0xC800)
_PROMACT = const(0xCC00)
_COLCTRL = const(0xCD00)
_SSCTRL = const(0xCE00)


# Command2_BK1
# SET _CND2BKxSEL TO 0X11
_VRHS = const(0xB000)
_VCOMS = const(0xB100)
_VGHSS = const(0xB200)
_TESTCMD = const(0xB300)
_VGLS = const(0xB500)
_VRHDV = const(0xB600)
_PWCTRL1 = const(0xB700)
_PWCTRL2 = const(0xB800)
_PWCTRL3 = const(0xB900)
_PCLKS1 = const(0xBA00)
_PCLKS2 = const(0xBB00)
_PCLKS3 = const(0xBC00)
_PDR1 = const(0xC100)
_PDR2 = const(0xC200)
_MIPISET1 = const(0xD000)
_MIPISET2 = const(0xD100)
_MIPISET3 = const(0xD200)
_MIPISET4 = const(0xD300)
_SRCTRL = const(0xE000)
_NRCTRL = const(0xE100)
_SECTRL = const(0xE200)
_CCCTRL = const(0xE300)
_SKCTRL = const(0xE400)
_NVMSETE = const(0xEA00)
_CABCCTRL = const(0xEE00)

# Command2_BK3
# SET _CND2BKxSEL TO 0X13
_NVMEN = const(0xC800)
_NVMSET = const(0xCA00)


class ST7701(display_driver_framework.DisplayDriver):
    _INVON = 0x21
    _INVOFF = 0x20

    def __init__(
        self,
        data_bus,
        spi_3wire,
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
        color_space=lv.COLOR_FORMAT.RGB888,
        rgb565_byte_swap=False,
        bus_shared_pins=False
    ):
        self.set_power(True)
        self._spi_3wire = spi_3wire
        spi_3wire.init(16, 8)

        # we need to run the initilization commands prior to
        # initilizing the RGB display in case the user has shared pins with
        # the RGB display
        if bus_shared_pins:
            self.init()
            self.init = self._dummy_init
            # shut down the spi3wire prior to constructing the display driver
            # so we don't have a conflict between the bus and the 3wire
            spi_3wire.deinit()

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
            _cmd_bits=16,
            _param_bits=8
        )

    def set_params(self, cmd, params=None):
        self._spi_3wire.tx_param(cmd, params)

    def get_params(self, cmd, params):
        pass

    def _set_memory_location(self, x1, y1, x2, y2):  # NOQA
        return -1

    def _dummy_init(self):
        display_driver_framework.DisplayDriver.init(self)

    def init(self):
        param_buf = bytearray(16)
        param_mv = memoryview(param_buf)

        color_size = lv.color_format_get_size(self._color_space)
        if color_size == 2:  # NOQA
            pixel_format = 0x55
        elif color_size == 3:
            pixel_format = 0x77
        else:
            raise RuntimeError(
                'ST7701 IC only supports '
                'lv.COLOR_FORMAT.RGB565 or lv.COLOR_FORMAT.RGB888'
            )

        param_buf[0] = pixel_format
        self.set_params(_COLMOD, param_mv[:1])

        time.sleep_ms(10)  # NOQA

        param_buf[0] = (
            self._madctl(
                self._color_byte_order,
                self._ORIENTATION_TABLE  # NOQA
            )
        )
        self.set_params(_MADCTL, param_mv[:1])

        # Command2_BK3
        param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, 0x13])
        self.set_params(_CND2BKxSEL, param_mv[:5])

        param_buf[0] = 0x08
        self.set_params(0xEF, param_mv[:1])

        # Command2_BK0
        param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, 0x10])
        self.set_params(_CND2BKxSEL, param_mv[:5])

        param_buf[0] = 0x3B
        param_buf[1] = 0x00
        self.set_params(_LNESET, param_mv[:2])

        param_buf[0] = 0x10
        param_buf[1] = 0x02
        self.set_params(_PORCTRL, param_mv[:2])

        param_buf[0] = 0x20
        param_buf[1] = 0x06
        self.set_params(_INVSET, param_mv[:2])

        param_buf[0] = 0x10
        self.set_params(_PROMACT, param_mv[:1])

        param_buf[:16] = bytearray([
            0x00, 0x13, 0x5A, 0x0F, 0x12, 0x07, 0x09, 0x08,
            0x08, 0x24, 0x07, 0x13, 0x12, 0x6B, 0x73, 0xFF])
        self.set_params(_PVGAMCTRL, param_mv)

        param_buf[:16] = bytearray([
            0x00, 0x13, 0x5A, 0x0F, 0x12, 0x07, 0x09, 0x08,
            0x08, 0x24, 0x07, 0x13, 0x12, 0x6B, 0x73, 0xFF])
        self.set_params(_NVGAMCTRL, param_mv)

        # Command2_BK1
        param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, 0x11])
        self.set_params(_CND2BKxSEL, param_mv[:5])

        param_buf[0] = 0x8D
        self.set_params(_VRHS, param_mv[:1])

        param_buf[0] = 0x48
        self.set_params(_VCOMS, param_mv[:1])

        param_buf[0] = 0x89
        self.set_params(_VGHSS, param_mv[:1])

        param_buf[0] = 0x80
        self.set_params(_TESTCMD, param_mv[:1])

        param_buf[0] = 0x49
        self.set_params(_VGLS, param_mv[:1])

        param_buf[0] = 0x85
        self.set_params(_PWCTRL1, param_mv[:1])

        param_buf[0] = 0x32
        self.set_params(_PWCTRL2, param_mv[:1])

        param_buf[0] = 0x78
        self.set_params(_PDR1, param_mv[:1])

        param_buf[0] = 0x78
        self.set_params(_PDR2, param_mv[:1])

        param_buf[0] = 0x88
        self.set_params(_MIPISET1, param_mv[:1])
        time.sleep_ms(100)  # NOQA

        param_buf[:3] = bytearray([0x00, 0x00, 0x02])
        self.set_params(_SRCTRL, param_mv[:3])

        param_buf[:11] = bytearray([
            0x05, 0xC0, 0x07, 0xC0, 0x04, 0xC0,
            0x06, 0xC0, 0x00, 0x44, 0x44])
        self.set_params(_NRCTRL, param_mv[:11])

        param_buf[:13] = bytearray([
            0x00, 0x00, 0x33, 0x33, 0x01, 0xC0, 0x00,
            0x00, 0x01, 0xC0, 0x00, 0x00, 0x00])
        self.set_params(_SECTRL, param_mv[:13])

        param_buf[:4] = bytearray([0x00, 0x00, 0x11, 0x11])
        self.set_params(_CCCTRL, param_mv[:4])

        param_buf[0] = 0x44
        param_buf[1] = 0x44
        self.set_params(_SKCTRL, param_mv[:2])

        param_buf[:16] = bytearray([
            0x0D, 0xF1, 0x10, 0x98, 0x0F, 0xF3, 0x10, 0x98,
            0x09, 0xED, 0x10, 0x98, 0x0B, 0xEF, 0x10, 0x98])
        self.set_params(0xE5, param_mv)

        param_buf[:4] = bytearray([0x00, 0x00, 0x11, 0x11])
        self.set_params(0xE6, param_mv[:4])

        param_buf[0] = 0x44
        param_buf[1] = 0x44
        self.set_params(0xE7, param_mv[:2])

        param_buf[:16] = bytearray([
            0x0C, 0xF0, 0x10, 0x98, 0x0E, 0xF2, 0x10, 0x98,
            0x08, 0xEC, 0x10, 0x98, 0x0A, 0xEE, 0x10, 0x98])
        self.set_params(0xE8, param_mv)

        param_buf[:7] = bytearray([0x00, 0x01, 0xE4, 0xE4, 0x44, 0x88, 0x00])
        self.set_params(0xEB, param_mv[:7])

        param_buf[:16] = bytearray([
            0xFF, 0x04, 0x56, 0x7F, 0xBA, 0x2F, 0xFF, 0xFF,
            0xFF, 0xFF, 0xF2, 0xAB, 0xF7, 0x65, 0x40, 0xFF])
        self.set_params(0xED, param_mv)

        param_buf[:6] = bytearray([0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F])
        self.set_params(0xEF, param_mv[:6])

        # Command2 OFF
        param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, 0x00])
        self.set_params(_CND2BKxSEL, param_mv[:5])

        self.set_params(_SLPOUT)
        time.sleep_ms(120)  # NOQA

        self.set_params(_DISPON)
