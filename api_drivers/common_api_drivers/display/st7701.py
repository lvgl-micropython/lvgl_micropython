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
_Command1 = const(0x00)

_SWRESET = const(0x01)  # Software Reset
_SLPIN = const(0x10)  # Sleep On
_SLPOUT = const(0x11)  # Sleep Off
_PTLON = const(0x12)  # Partial Display Mode On
_NORON = const(0x13)  # Normal Display Mode On
_ALLPOFF = const(0x22)  # All Pixel Off
_ALLPON = const(0x23)  # All Pixel On
_GAMSET = const(0x26)  # Gamma Set
_DISPOFF = const(0x28)  # Display Off
_DISPON = const(0x29)  # Display On
_TEOFF = const(0x34)  # Tearing Effect Line Off
_TEON = const(0x35)  # Tearing Effect Line On
_MADCTL = const(0x36)  # Display data access control
_IDMOFF = const(0x38)  # Idle Mode Off
_IDMON = const(0x39)  # Idle Mode On
_COLMOD = const(0x3A)  # Interface Pixel Format
_WRDISBV = const(0x51)  # Display Brightness Value
_WRCTRLD = const(0x53)  # CTRL Display
_WRCACE = const(0x55)  # Adaptive Brightness Control and Color Enhancement
_WRCABCMB = const(0x5E)  # CABC Minimum Brightness


_CND2BKxSEL = const(0xFF)

# Command2_BK0
# SET _CND2BKxSEL TO 0x10
_Command2_BK0 = const(0x10)

_PVGAMCTRL = const(0xB0)  # Positive Voltage Gamma Control
_NVGAMCTRL = const(0xB1)  # Negative Voltage Gamma Control
_DGMEN = const(0xB8)  # Digital Gamma Enable    0x00 = Off, 0x10 = On   1 byte
_DGMLUTR = const(0xB9)  # Digital Gamma Look-up Table for Redm    16 bytes
_DGMLUTB = const(0xBA)  # Digital Gamma Look-up Table for Blue   16 bytes
_PWM_CLK = const(0xBC)
_LNESET = const(0xC0)  # Display Line Setting

# Porch Control  2 bytes
# byte 1: Back-Porch Vertical line setting for display.  0x02 default
# byte 2: Front-Porch Vertical line setting for display. 0x04 default
_PORCTRL = const(0xC1)

# Inversion selection & Frame Rate Control   2 bytes
# byte 1: inversion selection
#         0x30: 1 dot
#         0x32: 2 dot
#         0x37: column
# default: 0x30

# byte 2: minimum number of pclk in each line
#         min value: 0x00
#         max value: 0x1F
#         value = (PCLK - 512) / 16
# default: 0x10 (768)
_INVSET = const(0xC2)

# RGB Control   3 bytes
# byte 1:
#         bit 7: RGB Mode Selection 1: HV mode, 0: DE mode
#         bit 3: VSYNC Polarity 1: high, 0: low (default: 1)
#         bit 2: HSYNC Polarity 1: high, 0: low (default: 0)
#         bit 1: DOTCLK Polarity 1: high, 0: low (default: 0)
#         bit 0: ENABLE Polarity 1: high, 0: low (default: 0)
#
# bytes 2 and 3: RGB interface Vsync back porch setting for HV mode
#                min value: 0x02
#                default: 0x1000
_RGBCTRL = const(0xC3)

# Partial Mode Control  4 bytes
# byte 1 & 2: start line address, max value 0x03FF (1023)
# byte 3 & 4: end line address,  max value 0x03FF (1023)
_PARCTRL = const(0xC5)

# X-direction Control  1 byte
# 0x00: source form 0 to 479
# 0x04: source form 479 to 0
# default: 0x00
_SDIR = const(0xC7)

_PDOSET = const(0xC8)  # Pseudo-Dot inversion diving setting
_PROMACT = const(0xCC)
_COLCTRL = const(0xCD)  # Color Control
_SSCTRL = const(0xCE)  # Sunlight Readable Enhancememnt


# Command2_BK1
# SET _CND2BKxSEL TO 0x11
_Command2_BK1 = const(0x11)
_VRHS = const(0xB0)  # VOP Amplitude setting
_VCOMS = const(0xB1)  # VCOM amplitude setting
_VGHSS = const(0xB2)  # VGH Voltage setting
_TESTCMD = const(0xB3)  # TEST Command Setting
_VGLS = const(0xB5)  # VGL Voltage setting
_VRHDV = const(0xB6)
_PWCTRL1 = const(0xB7)  # Power Control 1
_PWCTRL2 = const(0xB8)  # Power Control 2
_PWCTRL3 = const(0xB9)  # Power Control 3
_PCLKS1 = const(0xBA)  # Power pumping clk selection 1
_PCLKS2 = const(0xBB)  # Power pumping clk selection 2
_PCLKS3 = const(0xBC)  # Power pumping clk selection 3
_PDR1 = const(0xC1)  # Source pre_drive timing set1  SPD1??
_PDR2 = const(0xC2)  # Source EQ2 Setting  SPD2??
_MIPISET1 = const(0xD0)  # MIPI Setting 1
_MIPISET2 = const(0xD1)  # MIPI Setting 2
_MIPISET3 = const(0xD2)  # MIPI Setting 3
_MIPISET4 = const(0xD3)  # MIPI Setting 4
_SRCTRL = const(0xE0)
_NRCTRL = const(0xE1)  # Noise Reduce Control  **
_SECTRL = const(0xE2)  # Sharpness Control  **
_CCCTRL = const(0xE3)  # Color Calibration Control  **
_SKCTRL = const(0xE4)  # Skin Tone Preservation Control  **
_NVMSETE = const(0xEA)  # NVM Address Setting Enable  **
_CABCCTRL = const(0xEE)  # CABC Control **

# Command2_BK3
# SET _CND2BKxSEL TO 0x13
_Command2_BK3 = const(0x13)
_NVMEN = const(0xC8)
_NVMSET = const(0xCA)


COLOR_ENHANCE_OFF = 0x00
COLOR_ENHANCE_LOW = 0x01
COLOR_ENHANCE_MED = 0x02
COLOR_ENHANCE_HI = 0x04


ADAPT_IMAGE_MODE_OFF = 0x00
ADAPT_IMAGE_MODE_USER = 0x01
ADAPT_IMAGE_MODE_PICTURE = 0x02
ADAPT_IMAGE_MODE_VIDEO = 0x03


class ST7701(display_driver_framework.DisplayDriver):
    _INVOFF = 0x20  # Color Inversion Off
    _INVON = 0x21  # Color Inversion On

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

        self._spi_3wire = spi_3wire
        self._bus_shared_pins = bus_shared_pins

        self._wrctrld = 0x00
        self._wrcace = 0x00

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
            _cmd_bits=8,
            _param_bits=8,
            _init_bus=False
        )

    def set_params(self, cmd, params=None):
        if (
            not self._initilized or
            not self._bus_shared_pins
        ):
            self._spi_3wire.tx_param(cmd, params)

    def get_params(self, cmd, params):
        pass

    def _set_memory_location(self, x1, y1, x2, y2):  # NOQA
        return -1

    def init(self):
        self._spi_3wire.init(16, 8)
        self._init()
        if self._bus_shared_pins:
            # shut down the spi3wire prior to initilizing the data bus.
            # so we don't have a conflict between the bus and the 3wire
            self._spi_3wire.deinit()

        self._init_bus()

        display_driver_framework.DisplayDriver.init(self)

    def set_noise_reduction(self, value):
        if value:
            value -= 1
            if value > 0x03:
                raise ValueError('invalid noise reduction level')
            value |= 0x10

        self._param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, _Command2_BK0])
        self.set_params(_CND2BKxSEL, self._param_mv[:5])

        self._param_buf[0] = value
        self.set_params(_NRCTRL, self._param_mv[:1])

        self._param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, _Command1])
        self.set_params(_CND2BKxSEL, self._param_mv[:5])

    def set_skin_tone_enhancement(self, value):
        if value:
            value -= 1
            if value > 0x03:
                raise ValueError('invalid skin tone level')
            value |= 0x10

        self._param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, _Command2_BK0])
        self.set_params(_CND2BKxSEL, self._param_mv[:5])

        self._param_buf[0] = value
        self.set_params(_SKCTRL, self._param_mv[:1])

        self._param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, _Command1])
        self.set_params(_CND2BKxSEL, self._param_mv[:5])

    def set_sharpness(self, value):
        if value:
            value -= 1

            if value > 0x0F:
                raise ValueError('invalid sharpness level')

            value |= 0x10

        self._param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, _Command2_BK0])
        self.set_params(_CND2BKxSEL, self._param_mv[:5])

        self._param_buf[0] = value
        self.set_params(_SECTRL, self._param_mv[:1])

        self._param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, _Command1])
        self.set_params(_CND2BKxSEL, self._param_mv[:5])

    def set_sunlight_readable_enhancement(self, value):
        if value:
            value -= 1

            if value > 0x0F:
                raise ValueError('invalid sunlight enhancement level')

            value |= 0x10

        self._param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, _Command2_BK0])
        self.set_params(_CND2BKxSEL, self._param_mv[:5])

        self._param_buf[0] = value
        self.set_params(_SECTRL, self._param_mv[:1])

        self._param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, _Command1])
        self.set_params(_CND2BKxSEL, self._param_mv[:5])

    def set_adaptive_image_mode(self, value):
        if value > 3:
            raise ValueError('invalid adaptive image mode')

        if self._wrcace & 0x01:
            self._wrcace ^= 0x01

        if self._wrcace & 0x02:
            self._wrcace ^= 0x02

        self._wrcace |= value

        self._param_buf[0] = self._wrcace
        self.set_params(_WRCACE, self._param_mv[:1])

    def set_color_enhancement(self, value):
        if value:
            if not self._wrcace & 0x80:
                self._wrcace |= 0x80

            value -= 1

            if value > 3 or value == 2:
                raise ValueError('invalid color enhance value')
            value <<= 4

            if self._wrcace & 0x10:
                self._wrcace ^= 0x10
            if self._wrcace & 0x20:
                self._wrcace ^= 0x20

            self._wrcace |= value
        else:
            if self._wrcace & 0x80:
                self._wrcace ^= 0x80

        self._param_buf[0] = self._wrcace
        self.set_params(_WRCACE, self._param_mv[:1])

    def set_auto_brightness(self, value):
        if int(bool(value)):
            self._wrctrld |= 0x20
        elif self._wrctrld & 0x20:
            self._wrctrld ^= 0x20

        self._param_buf[0] = self._wrctrld
        self.set_params(_WRCTRLD, self._param_mv[:1])

    def set_display_dimming(self, value):
        if int(bool(value)):
            self._wrctrld |= 0x08
        elif self._wrctrld & 0x08:
            self._wrctrld ^= 0x08

        self._param_buf[0] = self._wrctrld
        self.set_params(_WRCTRLD, self._param_mv[:1])

    def set_backlight_control(self, value):
        if int(bool(value)):
            self._wrctrld |= 0x04
        elif self._wrctrld & 0x04:
            self._wrctrld ^= 0x04

        self._param_buf[0] = self._wrctrld
        self.set_params(_WRCTRLD, self._param_mv[:1])

    def set_brightness(self, value):
        self._param_buf[0] = int((value / 100.0) * 255.0)
        self.set_params(_WRDISBV, self._param_mv[:1])

    def _init(self):
        param_buf = bytearray(16)
        param_mv = memoryview(param_buf)

        # Command2 OFF
        param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, _Command1])
        self.set_params(_CND2BKxSEL, param_mv[:5])

        self.set_params(_SLPOUT)
        time.sleep_ms(120)  # NOQA

        self.set_params(_SWRESET)
        time.sleep_ms(5)  # NOQA

        color_size = lv.color_format_get_size(self._color_space)
        if color_size == 2:  # NOQA
            pixel_format = 0x50
        elif color_size == 3:
            pixel_format = 0x70
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
        param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, _Command2_BK3])
        self.set_params(_CND2BKxSEL, param_mv[:5])

        param_buf[0] = 0x08
        self.set_params(0xEF, param_mv[:1])

        # Command2_BK0
        param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, _Command2_BK0])
        self.set_params(_CND2BKxSEL, param_mv[:5])

        param_buf[0] = 0x3B
        param_buf[1] = 0x00
        self.set_params(_LNESET, param_mv[:2])

        param_buf[0] = 0x0B
        param_buf[1] = 0x02
        self.set_params(_PORCTRL, param_mv[:2])

        param_buf[0] = 0x00
        param_buf[1] = 0x02
        self.set_params(_INVSET, param_mv[:2])

        param_buf[0] = 0x10
        self.set_params(_PROMACT, param_mv[:1])

        param_buf[0] = 0x08
        self.set_params(_COLCTRL, param_mv[:1])

        param_buf[:16] = bytearray([
            0x00, 0x13, 0x5A, 0x0F, 0x12, 0x07, 0x09, 0x08,
            0x08, 0x24, 0x07, 0x13, 0x12, 0x6B, 0x73, 0xFF])
        self.set_params(_PVGAMCTRL, param_mv)

        param_buf[:16] = bytearray([
            0x00, 0x13, 0x5A, 0x0F, 0x12, 0x07, 0x09, 0x08,
            0x08, 0x24, 0x07, 0x13, 0x12, 0x6B, 0x73, 0xFF])
        self.set_params(_NVGAMCTRL, param_mv)

        # Command2_BK1
        param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, _Command2_BK1])
        self.set_params(_CND2BKxSEL, param_mv[:5])

        param_buf[0] = 0x5D
        self.set_params(_VRHS, param_mv[:1])

        param_buf[0] = 0x43
        self.set_params(_VCOMS, param_mv[:1])

        param_buf[0] = 0x81
        self.set_params(_VGHSS, param_mv[:1])

        param_buf[0] = 0x80
        self.set_params(_TESTCMD, param_mv[:1])

        param_buf[0] = 0x43
        self.set_params(_VGLS, param_mv[:1])

        param_buf[0] = 0x85
        self.set_params(_PWCTRL1, param_mv[:1])

        param_buf[0] = 0x20
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
            0x03, 0xA0, 0x00, 0x00, 0x04, 0xA0,
            0x00, 0x00, 0x00, 0x20, 0x20])
        self.set_params(_NRCTRL, param_mv[:11])

        param_buf[:13] = bytearray([
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
        self.set_params(_SECTRL, param_mv[:13])

        param_buf[:4] = bytearray([0x00, 0x00, 0x11, 0x00])
        self.set_params(_CCCTRL, param_mv[:4])

        param_buf[0] = 0x22
        param_buf[1] = 0x00
        self.set_params(_SKCTRL, param_mv[:2])

        param_buf[:16] = bytearray([
            0x05, 0xEC, 0xA0, 0xA0, 0x07, 0xEE, 0xA0, 0xA0,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
        self.set_params(0xE5, param_mv)

        param_buf[:4] = bytearray([0x00, 0x00, 0x11, 0x00])
        self.set_params(0xE6, param_mv[:4])

        param_buf[0] = 0x22
        param_buf[1] = 0x00
        self.set_params(0xE7, param_mv[:2])

        param_buf[:16] = bytearray([
            0x06, 0xED, 0xA0, 0xA0, 0x08, 0xEF, 0xA0, 0xA0,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
        self.set_params(0xE8, param_mv)

        param_buf[:7] = bytearray([0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00])
        self.set_params(0xEB, param_mv[:7])

        param_buf[:16] = bytearray([
            0xFF, 0xFF, 0xFF, 0xBA, 0x0A, 0xBF, 0x45, 0xFF,
            0xFF, 0x54, 0xFB, 0xA0, 0xAB, 0xFF, 0xFF, 0xFF])
        self.set_params(0xED, param_mv)

        param_buf[:6] = bytearray([0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F])
        self.set_params(0xEF, param_mv[:6])

        # Command2 OFF
        param_buf[:5] = bytearray([0x77, 0x01, 0x00, 0x00, _Command1])
        self.set_params(_CND2BKxSEL, param_mv[:5])

        self.set_params(_DISPON)
        time.sleep_ms(10)  # NOQA
