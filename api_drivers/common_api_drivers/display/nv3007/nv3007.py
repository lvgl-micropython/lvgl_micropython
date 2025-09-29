# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus  # NOQA
import display_driver_framework

### begin straight-up copy from GC9A01 driver
STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR

_MADCTL_MH = const(0x04)  # Refresh 0=Left to Right, 1=Right to Left
_MADCTL_BGR = const(0x08)  # BGR color order
_MADCTL_ML = const(0x10)  # Refresh 0=Top to Bottom, 1=Bottom to Top
_MADCTL_MV = const(0x20)  # 0=Normal, 1=Row/column exchange
_MADCTL_MX = const(0x40)  # 0=Left to Right, 1=Right to Left
_MADCTL_MY = const(0x80)  # 0=Top to Bottom, 1=Bottom to Top

class NV3007(display_driver_framework.DisplayDriver):

    ## this is probably wrong!
    _ORIENTATION_TABLE = (
        0,
        _MADCTL_MX | _MADCTL_MV,
        _MADCTL_MY | _MADCTL_MX,
        _MADCTL_MY | _MADCTL_MV
    )

    # virtual uint8_t getMadCtl(uint8_t r) const
    # {
    #   static constexpr uint8_t madctl_table[] =
    #   {
    #                                      0,
    #     MAD_MV|MAD_MX|MAD_MH              ,
    #            MAD_MX|MAD_MH|MAD_MY|MAD_ML,
    #     MAD_MV|              MAD_MY|MAD_ML,
    #                          MAD_MY|MAD_ML,
    #     MAD_MV                            ,
    #            MAD_MX|MAD_MH              ,
    #     MAD_MV|MAD_MX|MAD_MY|MAD_MH|MAD_ML,
    #   };
    #   return madctl_table[r];
    # }


## end straight-up copy


    ## other bits to implement:
    # uint8_t getColMod(uint8_t bpp) const override { return 0x05; }
    # uint8_t getMadCtl(uint8_t r) const override { return MAD_MV | MAD_MY; }
    # _cfg.offset_y  = 12;

    ## src/lgfx/v1/panel/Panel_NV3007.hpp
    ##  _cfg.offset_y  = 12;
