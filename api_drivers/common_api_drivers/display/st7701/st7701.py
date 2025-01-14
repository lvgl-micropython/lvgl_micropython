# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA
import rgb_display_framework  # NOQA


STATE_HIGH = rgb_display_framework.STATE_HIGH
STATE_LOW = rgb_display_framework.STATE_LOW
STATE_PWM = rgb_display_framework.STATE_PWM

BYTE_ORDER_RGB = rgb_display_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = rgb_display_framework.BYTE_ORDER_BGR

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


TYPE_TL021WVC02 = 10
TYPE_TL034WVS05_B1477A = 11
TYPE_TL032FWV01_I1440A = 12
TYPE_TL040WVS03 = 13
TYPE_TL028WVC01 = 14
TYPE_HD371001C40 = 15
TYPE_HD458002C40 = 16


class ST7701(rgb_display_framework.RGBDisplayDriver):
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
        color_space=lv.COLOR_FORMAT.RGB888,  # NOQA
        rgb565_byte_swap=False,
        bus_shared_pins=False
    ):

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
            spi_3wire=spi_3wire,
            spi_3wire_shared_pins=bus_shared_pins,
            _cmd_bits=8,
            _param_bits=8,
            _init_bus=False,
        )

    def _spi_3wire_init(self, type):  # NOQA
        if type < 1 or type > 17:
            raise RuntimeError('Invalid display type')

        mod_name = f'_st7701_type{type}'
        mod = __import__(mod_name)
        mod.init(self)

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

    def _madctl(self, *_, **__):
        raise NotImplementedError
