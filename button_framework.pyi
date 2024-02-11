from typing import Optional, ClassVar
from typing import TYPE_CHECKING


if TYPE_CHECKING:
    import lvgl as lv  # NOQA
    import display_driver_framework


class ButtonDriver:
    _instance_counter: ClassVar[int] = ...
    id: int = ...
    _disp_drv: display_driver_framework.DisplayDriver = ...
    _last_button: int = ...
    _current_state: int = ...
    _indev_drv: lv.indev_t = ...


    def set_button_points(self, *points) -> None:
        """
        :param points: this is a varargs (*args) parameter.
               Example of use:

               button_driver.set_button_points(
                   (lv.point_t(dict(x=0, y=0)), lv.point_t(dict(x=100, y=50))),
                   (lv.point_t(dict(x=0, y=55)), lv.point_t(dict(x=100, y=110))),
                   (lv.point_t(dict(x=0, y=115)), lv.point_t(dict(x=100, y=170)))
               )

               You do not need to worry about keeping a reference to the points.
               This is done internally in the driver. You do however need to
               keep a reference to the driver.
        """

    def __init__(self):
        ...

    def _get_button(self) -> Optional[int]:
        """
        Reads the botton

        This function needs to return the id of the button that is pressed.
        In order for this to work you need to add the points to a coorsponding
        software button that you make. The id is going to be the index number
        of the points you have set.

        :return: None if there is no button pressed or the id of the button

        :raises: NotImplimentedError if the method is not overridden
                 in the button driver
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