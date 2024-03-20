from typing import Union, Optional, Tuple, ClassVar
from typing import TYPE_CHECKING


if TYPE_CHECKING:
    import touch_cal_data
    import display_driver_framework
    import lvgl as lv  # NOQA


def _remap(
    value: Union[float, int],
    old_min: Union[float, int],
    old_max: Union[float, int],
    new_min: Union[float, int],
    new_max: Union[float, int]
) -> int:
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

class PointerDriver:
    _instance_counter: ClassVar[int] = ...
    id: int = ...
    _disp_drv: display_driver_framework.DisplayDriver = ...
    _last_x: int = ...
    _last_y: int = ...
    _current_state: int = ...
    _height: int = ...
    _width: int = ...
    _indev_drv: lv.indev_t = ...
    _config: touch_cal_data.TouchCalData = ...
    PRESSED: ClassVar[int] = ...
    RELEASED: ClassVar[int] = ...

    def get_width(self) -> int:
        """
        Touch panel width in pixels
        """
        ...

    def get_height(self) -> int:
        """
        Touch panel height in pixels
        """
        ...

    def get_rotation(self):
        """
        The rotation of the display
        """
        ...

    def __init__(self, touch_cal: Optional[touch_cal_data.TouchCalData] = None):
        ...

    def calibrate(self) -> None:
        ...

    @property
    def is_calibrated(self) -> bool:
        ...

    def _get_coords(self) -> Optional[Tuple[int, int]]:
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

    def _read(self, drv, data) -> bool:  # NOQA
        ...

    def get_type(self) -> int:
        ...

    def read(self) -> None:
        ...

    def send_event(self, code, param):
        ...

    def remove_event(self, index):
       ...

    def get_event_dsc(self, index):
        ...

    def get_event_count(self):
        ...

    def add_event_cb(self, event_cb, filter, user_data):
        ...

    def search_obj(self, point):
        ...

    def delete_read_timer(self):
        ...

    def get_read_timer(self):
        ...

    def get_active_obj(self):
        ...

    def wait_release(self):
        ...

    def get_vect(self, point):
        ...

    def get_scroll_obj(self):
        ...

    def get_scroll_dir(self):
        ...

    def get_gesture_dir(self):
        ...

    def get_point(self, point):
        ...

    def get_state(self):
        ...

    def enable(self, en):
        ...

    def set_button_points(self, points):
        ...

    def get_group(self):
        ...

    def set_group(self, group):
        ...

    def set_cursor(self, cur_obj):
        ...

    def reset_long_press(self):
        ...

    def reset(self, obj):
        ...

    def get_disp(self):
        ...

    @staticmethod
    def active():
        ...
