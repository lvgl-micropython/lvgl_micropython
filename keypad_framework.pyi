# Copyright (C) 2024  Kevin G Schlosser
# Code that is written by the above named is done under the GPL license
# and that license is able to be viewed in the LICENSE file in the root
# of this project.

from typing import Optional, Tuple
import _indev_base


class KeypadDriver(_indev_base.IndevBase):
    _last_key: int = ...

    def __init__(self):
        ...

    def _get_key(self) -> Optional[Tuple[int, int]]:
        """
        Reads the keys from the keypad

        This function needs to return one of the LV_KEY enumerations

        :return: None if there is no keypress or the id of the key

        :raises: NotImplimentedError if the method is not overridden
                 in the keypad driver
        """
        ...

    def reset_long_press(self) -> None:
        ...


