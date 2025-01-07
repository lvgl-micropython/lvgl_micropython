# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import time


_DISPLAY_OFF = const(0xAE)
_DISPLAY_ON = const(0xAF)

_DISP_START_LINE = const(0x40)
_PAGE = const(0xB0)

_COLUMN_UPPER = const(0x10)
_COLUMN_LOWER = const(0x00)

_ADC_NORMAL = const(0xA0)
_ADC_REVERSE = const(0xA1)

_DISP_NORMAL = const(0xA6)
_DISP_REVERSE = const(0xA7)

_ALLPTS_NORMAL = const(0xA4)
_ALLPTS_ON = const(0xA5)
_BIAS_9 = const(0xA2)
_BIAS_7 = const(0xA3)

_RMW = const(0xE0)
_RMW_CLEAR = const(0xEE)
_INTERNAL_RESET = const(0xE2)
_COM_NORMAL = const(0xC0)
_COM_REVERSE = const(0xC8)
_POWER_CONTROL = const(0x28)
_RESISTOR_RATIO = const(0x20)
_VOLUME_FIRST = const(0x81)
_VOLUME_SECOND = const(0x00)
_STATIC_OFF = const(0xAC)
_STATIC_ON = const(0xAD)
_STATIC_REG = const(0x00)
_BOOSTER_FIRST = const(0xF8)
_BOOSTER_234 = const(0x00)
_BOOSTER_5 = const(0x01)
_BOOSTER_6 = const(0x03)
_NOP = const(0xE3)
_TEST = const(0xF0)


def init(self):
    self.reset()

    self.set_params(_BIAS_7)
    self.set_params(_ADC_NORMAL)
    self.set_params(_COM_NORMAL)
    self.set_params(_DISP_START_LINE)
    self.set_params(_POWER_CONTROL | 0x4)
    time.sleep_ms(50) # NOQA

    self.set_params(_POWER_CONTROL | 0x6)
    time.sleep_ms(50) # NOQA

    self.set_params(_POWER_CONTROL | 0x7)
    time.sleep_ms(10) # NOQA

    # Defaulted to 0x26 (but could also be between
    # 0x20-0x27 based on display's specs)
    self.set_params(_RESISTOR_RATIO | 0x6)

    self.set_params(_DISPLAY_ON)
    self.set_params(_ALLPTS_NORMAL)

    # Set brightness
    self.set_params(_VOLUME_FIRST)
    self.set_params(_VOLUME_SECOND | (0x18 & 0x3f))
