# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# FT6206/FT6306

from micropython import const  # NOQA
import focaltech_touch
import pointer_framework

I2C_ADDR = 0x38
BITS = 8

_FT6206_CHIPID = const(0x06)
_FT6x06_CHIPID = const(0x11)


class FT6x06(focaltech_touch.FocalTechTouch):

    def __init__(
        self,
        device,
        touch_cal=None,
        startup_rotation=pointer_framework.lv.DISPLAY_ROTATION._0,  # NOQA
        debug=False
    ):  # NOQA

        super().__init__(
            device,
            touch_cal,
            startup_rotation,
            debug,
            None,
            _FT6206_CHIPID,
            _FT6x06_CHIPID
        )
