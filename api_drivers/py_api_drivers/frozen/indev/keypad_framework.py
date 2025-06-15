# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import lvgl as lv  # NOQA
import _indev_base


class KeypadDriver(_indev_base.IndevBase):

    def __init__(self):  # NOQA
        self._last_key = ord(' ')

        super().__init__()
        self._set_type(lv.INDEV_TYPE.KEYPAD)  # NOQA
        self._indev_drv.enable(True)  # NOQA

    def _get_key(self):
        # this method needs to be overridden.
        # the returned value from this method is going to be (state, keycode)
        # or None if no key event has occured
        raise NotImplementedError

    def _read(self, drv, data):  # NOQA
        key = self._get_key()

        if key is None:  # ignore no key
            state = self.RELEASED
            key = self._last_key
        else:
            state, key = key

        data.key = self._last_key = key
        data.state = self._current_state = state

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
