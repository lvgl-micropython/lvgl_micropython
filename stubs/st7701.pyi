# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Final, ClassVar
import rgb_display_framework

import lcd_bus


STATE_HIGH: Final[int]
STATE_LOW: Final[int]
STATE_PWM: Final[int]

BYTE_ORDER_RGB: Final[int]
BYTE_ORDER_BGR: Final[int]

COLOR_ENHANCE_OFF: Final[int]
COLOR_ENHANCE_LOW: Final[int]
COLOR_ENHANCE_MED: Final[int]
COLOR_ENHANCE_HI: Final[int]


ADAPT_IMAGE_MODE_OFF: Final[int]
ADAPT_IMAGE_MODE_USER: Final[int]
ADAPT_IMAGE_MODE_PICTURE: Final[int]
ADAPT_IMAGE_MODE_VIDEO: Final[int]


TYPE_TL021WVC02: Final[int]
TYPE_TL034WVS05_B1477A: Final[int]
TYPE_TL032FWV01_I1440A: Final[int]
TYPE_TL040WVS03: Final[int]
TYPE_TL028WVC01: Final[int]
TYPE_HD371001C40: Final[int]
TYPE_HD458002C40: Final[int]

class ST7701(rgb_display_framework.RGBDisplayDriver):
    _INVOFF: ClassVar[int]
    _INVON: ClassVar[int]

    _wrctrld: int
    _wrcace: int

    def __init__(
        self,
        data_bus: lcd_bus.RGBBus,
        spi_3wire: int,
        display_width: int,
        display_height: int,
        frame_buffer1: memoryview | None = None,
        frame_buffer2: memoryview | None = None,
        reset_pin=None,
        reset_state: int = STATE_HIGH,
        power_pin=None,
        power_on_state: int = STATE_HIGH,
        backlight_pin=None,
        backlight_on_state: int = STATE_HIGH,
        offset_x: int = 0,
        offset_y: int = 0,
        color_byte_order: int = BYTE_ORDER_RGB,
        color_space: int = lv.COLOR_FORMAT.RGB888,  # NOQA
        rgb565_byte_swap: bool = False,
        bus_shared_pins: bool = False
    ):
        ...

    def _spi_3wire_init(self, type: int) -> None:
        ...

    def set_noise_reduction(self, value: int) -> None:
        ...

    def set_skin_tone_enhancement(self, value: int) -> None:
        ...

    def set_sharpness(self, value: int) -> None:
        ...

    def set_sunlight_readable_enhancement(self, value: int) -> None:
        ...

    def set_adaptive_image_mode(self, value: int) -> None:
        ...

    def set_color_enhancement(self, value: int) -> None:
        ...

    def set_auto_brightness(self, value: int) -> None:
        ...

    def set_display_dimming(self, value: int | None) -> None:
        ...

    def set_backlight_control(self, value: int | None) -> None:
        ...

    def set_brightness(self, value: float | int) -> None:
        ...

    def _madctl(self, *_, **__):
        raise NotImplementedError
