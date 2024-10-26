# Copyright (C) 2024  Kevin G Schlosser
# Code that is written by the above named is done under the GPL license
# and that license is able to be viewed in the LICENSE file in the root
# of this project.

from typing import Union, Optional, Tuple, ClassVar
from typing import TYPE_CHECKING


if TYPE_CHECKING:
    import lvgl as lv  # NOQA
    import display_driver_framework

import _indev_base

class EncoderDriver(_indev_base.IndevBase):
    _last_enc_value: int = ...
    _last_enc_diff: int = ...
    _last_key: int = ...

    def __init__(self):
        ...

    def _get_enc(
        self
    ) -> Optional[Union[Tuple[int, int], Tuple[None, int], Tuple[int, None]]]:
        """
        Reads the encoder position and any buttons that may exist

        This function needs to return the encoder value and the key value.
        If either of those is not being used then you can return None for
        that item.
        If both items are not being used then return a single None

        :return: None if there is no button press and there is no encoder value

        :raises: NotImplimentedError if the method is not overridden
                 in the encoder driver
        """
        ...

    def get_scroll_obj(self):
        ...

    def get_scroll_dir(self):
        ...

    def reset_long_press(self):
        ...
