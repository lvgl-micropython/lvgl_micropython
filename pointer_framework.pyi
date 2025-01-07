# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Optional, Tuple, TYPE_CHECKING
import _indev_base
import lcd_utils as _lcd_utils
import lvgl as _lv  # NOQA

lv = _lv


if TYPE_CHECKING:
    import touch_cal_data as _touch_cal_data
    import display_driver_framework as _display_driver_framework



remap = _lcd_utils.remap
_remap = _lcd_utils.remap


class PointerDriver(_indev_base.IndevBase):
    _last_x: int = ...
    _last_y: int = ...
    _orig_width: int = ...
    _orig_height: int = ...
    _config: _touch_cal_data.TouchCalData = ...

    def __init__(self, touch_cal: Optional[_touch_cal_data.TouchCalData] = None, startup_rotation=lv.DISPLAY_ROTATION._0, debug: bool=False):
        ...

    def calibrate(self) -> None:
        ...

    @property
    def is_calibrated(self) -> bool:
        ...

    def _get_coords(self) -> Optional[Tuple[int, int, int]]:
        """
        Reads the coordinates from the touch panel

        This is where the work is done by the touch driver. This method MUST
        be overridden when a touch driver is made.

        :return: None if there is no touch input or a tuple of (x, y)
                 if there is touch input

        :raises: NotImplimentedError if the method is not overridden
                 in the touch driver
        """
        ...

    def get_vect(self, point: lv.point_t):
        ...

    def get_scroll_obj(self) -> Optional[lv.obj]:
        ...

    def get_scroll_dir(self) -> int:
        ...

    def get_gesture_dir(self) -> int:
        ...

    def get_point(self, point: lv.point_t) -> None:
        ...

    def set_cursor(self, cur_obj) -> None:
        ...

    def reset_long_press(self) -> None:
        ...
