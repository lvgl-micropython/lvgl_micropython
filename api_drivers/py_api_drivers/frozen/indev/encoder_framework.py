# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import lvgl as lv  # NOQA
import _indev_base


class EncoderDriver(_indev_base.IndevBase):
    _instance_counter = 1

    def __init__(self, touch_cal=None):  # NOQA
        self.__class__._instance_counter += 1
        self.id = self.__class__._instance_counter
        self._cursors = []

        if not lv.is_initialized():
            lv.init()

        disp = lv.display_get_default()  # NOQA

        if disp is None:
            raise RuntimeError(
                'the display driver must be initilized '
                'before the encoder driver'
            )

        self._disp_drv = disp

        displays = display_driver_framework.DisplayDriver.get_displays()  # NOQA
        for display in displays:
            if display._disp_drv == disp:  # NOQA
                self._py_disp_drv = display
                break
        else:
            raise RuntimeError(
                'Display driver needs to initilized before indev driver'
            )

        self._last_enc_value = 0
        self._last_enc_diff = 0
        self._last_key = 0
        self._current_state = lv.INDEV_STATE.RELEASED  # NOQA

        indev_drv = lv.indev_create()
        indev_drv.set_type(lv.INDEV_TYPE.ENCODER)  # NOQA
        indev_drv.set_read_cb(self._read)  # NOQA
        indev_drv.set_driver_data(self)  # NOQA
        indev_drv.set_display(disp)  # NOQA
        indev_drv.enable(True)  # NOQA
        self._indev_drv = indev_drv

        super().__init__()
        self._set_type(lv.INDEV_TYPE.ENCODER)  # NOQA

    def _get_enc(self):
        # this method needs to be overridden.
        # the returned value from this method is going to be a keycode
        # or None if no key event has occured
        raise NotImplementedError

    def _read(self, drv, data):  # NOQA
        dta = self._get_enc()

        if dta is None:  # ignore no touch & multi touch
            if self._current_state != lv.INDEV_STATE.RELEASED:  # NOQA
                self._current_state = lv.INDEV_STATE.RELEASED  # NOQA
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
            self._current_state = data.state = lv.INDEV_STATE.RELEASED  # NOQA
        else:
            self._current_state = data.state = lv.INDEV_STATE.PRESSED  # NOQA
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

    def get_type(self):
        return self._indev_drv.get_type()  # NOQA

    def read(self):
        self._indev_drv.read()  # NOQA

    def send_event(self, code, param):
        return self._indev_drv.send_event(code, param)  # NOQA

    def remove_event(self, index):
        return self._indev_drv.remove_event(index)  # NOQA

    def get_event_dsc(self, index):
        return self._indev_drv.get_event_dsc(index)  # NOQA

    def get_event_count(self):
        return self._indev_drv.get_event_count()  # NOQA

    def add_event_cb(self, event_cb, filter, user_data):  # NOQA
        self._indev_drv.add_event_cb(event_cb, filter, user_data)  # NOQA

    def search_obj(self, point):
        return self._indev_drv.search_obj(point)  # NOQA

    def delete_read_timer(self):
        self._indev_drv.delete_read_timer()  # NOQA

    def get_read_timer(self):
        return self._indev_drv.get_read_timer()  # NOQA

    def get_active_obj(self):
        return self._indev_drv.get_active_obj()  # NOQA

    def wait_release(self):
        self._indev_drv.wait_release()  # NOQA

    def get_vect(self, point):
        self._indev_drv.get_vect(point)  # NOQA

    def get_scroll_obj(self):
        return self._indev_drv.get_scroll_obj()  # NOQA

    def get_scroll_dir(self):
        return self._indev_drv.get_scroll_dir()  # NOQA

    def get_gesture_dir(self):
        return self._indev_drv.get_gesture_dir()  # NOQA

    def get_point(self, point):
        self._indev_drv.get_point(point)  # NOQA

    def get_state(self):
        return self._indev_drv.get_state()  # NOQA

    def enable(self, en):
        self._indev_drv.enable(en)  # NOQA

    def get_group(self):
        return self._indev_drv.get_group()  # NOQA

    def set_group(self, group):
        self._indev_drv.set_group(group)  # NOQA

    def set_cursor(self, cur_obj):
        self._indev_drv.set_cursor(cur_obj)  # NOQA

    def reset_long_press(self):
        self._indev_drv.reset_long_press()  # NOQA

    def reset(self, obj):
        self._indev_drv.reset(obj)  # NOQA

    def get_disp(self):
        return self._disp_drv

    @staticmethod
    def active():
        indev = lv.indev_active()  # NOQA
        return indev.get_driver_data()
