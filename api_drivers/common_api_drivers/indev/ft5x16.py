# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# FT5216/FT5316

from micropython import const  # NOQA
import focaltech_touch
import pointer_framework


I2C_ADDR = 0x38
BITS = 8

_FT5x16_CHIPID = const(0x0A)


class FT5x16(focaltech_touch.FocalTechTouch):

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
            (2.24, 2.14),
            _FT5x16_CHIPID
        )
