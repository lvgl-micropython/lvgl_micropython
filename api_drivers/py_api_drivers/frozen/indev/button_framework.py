# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import lvgl as lv  # NOQA
import _indev_base


class ButtonDriver(_indev_base.IndevBase):

    def __init__(self):  # NOQA
        self._last_button = -1
        self._button_points = []

        super().__init__()
        self._set_type(lv.INDEV_TYPE.BUTTON)  # NOQA

    def set_button_points(self, *points):
        self._indev_drv.set_button_points(list(points))
        self._button_points = list(points)

    def _get_button(self):
        # this method needs to be overridden.
        # the returned value from this method is going to be a keycode
        # or None if no key event has occured
        raise NotImplementedError

    def _read(self, drv, data):  # NOQA
        button = self._get_button()

        if button is None:  # ignore no touch & multi touch
            if self._current_state != self.RELEASED:
                self._current_state = self.RELEASED
                res = True
            else:
                res = False

            data.enc_diff = self._last_button
            data.state = self._current_state
            data.continue_reading = False
            return res

        self._last_button = button
        self._current_state = data.state = self.PRESSED

        data.btn_id = self._last_button
        data.state = self._current_state
        data.continue_reading = True

        return True

    def reset_long_press(self):
        self._indev_drv.reset_long_press()
