# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# FT5346/FT5446/FT5446S

from micropython import const  # NOQA
import focaltech_touch
import pointer_framework

_DEV_MODE = const(0x00)
_GEST_ID = const(0x01)
_TD_STATUS = const(0x02)

I2C_ADDR = 0x38
BITS = 8

_FT5x46_CHIPID = const(0x54)


class FT5x46(focaltech_touch.FocalTechTouch):

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
            _FT5x46_CHIPID
        )
