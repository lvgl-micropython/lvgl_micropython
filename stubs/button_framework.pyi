# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from typing import Optional, TYPE_CHECKING

import _indev_base

if TYPE_CHECKING:
    import lvgl as _lv


class ButtonDriver(_indev_base.IndevBase):
    _last_button: int = ...

    def set_button_points(self, *points: list[_lv.point_t]) -> None:
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

    def reset_long_press(self):
        ...
