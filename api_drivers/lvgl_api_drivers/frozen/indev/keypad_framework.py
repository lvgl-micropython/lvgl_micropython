import lvgl as lv  # NOQA
import _indev_base


class KeypadDriver(_indev_base.IndevBase):

    def __init__(self):  # NOQA
        self._last_key = -1

        super().__init__()
        self._set_type(lv.INDEV_TYPE_KEYPAD)

    def _get_key(self):
        # this method needs to be overridden.
        # the returned value from this method is going to be (state, keycode)
        # or None if no key event has occured
        raise NotImplementedError

    def _read(self, drv, data):  # NOQA
        key = self._get_key()

        if key is None:  # ignore no key
            if self._current_state != self.RELEASED:
                self._current_state = self.RELEASED
                res = True
            else:
                res = False

            data.key = self._last_key
            data.state = self._current_state
            data.continue_reading = False
            return res

        state, key = key

        self._last_key = key

        if self._current_state == state == self.RELEASED:
            res = False
            data.continue_reading = False
        else:
            res = True
            data.continue_reading = True

        self._current_state = state

        data.key = self._last_key
        data.state = self._current_state

        return res
