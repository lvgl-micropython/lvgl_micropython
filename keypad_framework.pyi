# Copyright (c) 2024 - 2025 Kevin G. Schlosser

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


