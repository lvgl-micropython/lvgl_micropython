import lvgl as lv  # NOQA
import _indev_base


class EncoderDriver(_indev_base.IndevBase):

    def __init__(self):  # NOQA
        self._last_enc_value = 0
        self._last_enc_diff = 0
        self._last_key = 0

        super().__init__()
        self._set_type(lv.INDEV_TYPE_ENCODER)

    def _get_enc(self):
        # this method needs to be overridden.
        # the returned value from this method is going to be a keycode
        # or None if no key event has occured
        raise NotImplementedError

    def _read(self, drv, data):  # NOQA
        dta = self._get_enc()

        if dta is None:  # ignore no touch & multi touch
            if self._current_state != self.RELEASED:
                self._current_state = self.RELEASED
                res = True
            else:
                res = False

            data.key = self._last_key
            data.enc_diff = self._last_enc_diff
            data.state = self._current_state
            data.continue_reading = False
            return res

        enc, key = dta

        if key is None:
            self._current_state = data.state = self.RELEASED
        else:
            self._current_state = data.state = self.PRESSED
            self._last_key = key

        if enc is None:
            self._last_enc_diff = 0
            self._last_enc_value = 0
        else:
            self._last_enc_diff = self._last_enc_value + enc
            self._last_enc_value = enc

        data.enc_diff = self._last_enc_diff
        data.state = self._current_state
        data.continue_reading = True

        return True

    def get_scroll_obj(self):
        return lv.indev_get_scroll_obj(self._indev_drv)

    def get_scroll_dir(self):
        return lv.indev_get_scroll_dir(self._indev_drv)
