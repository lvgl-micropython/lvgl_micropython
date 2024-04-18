from typing import Optional, Tuple, TYPE_CHECKING
import _indev_base


if TYPE_CHECKING:
    import touch_cal_data as _touch_cal_data
    import display_driver_framework as _display_driver_framework
    import lvgl as _lv  # NOQA


def _remap(value: int, old_min: int, old_max: int, new_min: int, new_max: int) -> int:
    """
    Internal use

    handles the rotation of a display touch panel to convert the x, y
    coordinates into the proper values based on the rotation of the touch panel

    :param value: value to map into the new range
    :param old_min: minimum value for the old range
    :param old_max: maximum value for the old range
    :param new_min: minimum value for the new range
    :param new_max: maximum value for the new range
    :return: new value that is mapped into the new range
    """
    ...

class PointerDriver(_indev_base.IndevBase):
    _last_x: int = ...
    _last_y: int = ...
    _config: _touch_cal_data.TouchCalData = ...

    def __init__(self, touch_cal: Optional[_touch_cal_data.TouchCalData] = None):
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

    def get_vect(self, point: _lv.point_t):
        ...

    def get_scroll_obj(self) -> Optional[_lv.obj]:
        ...

    def get_scroll_dir(self) -> int:
        ...

    def get_gesture_dir(self) -> int:
        ...

    def get_point(self, point: _lv.point_t) -> None:
        ...

    def set_cursor(self, cur_obj) -> None:
        ...

    def reset_long_press(self) -> None:
        ...
