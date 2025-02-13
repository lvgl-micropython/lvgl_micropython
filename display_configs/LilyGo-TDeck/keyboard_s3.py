# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import lvgl as lv  # NOQA
import keypad_framework


class Keyboard(keypad_framework.KeypadDriver):
    def __init__(self, device):  # NOQA
        self._device = device
        self._buf = bytearray(1)
        self._mv = memoryview(self._buf)

        super().__init__()

    def _get_key(self):
        # lv.KEY.ESC = 0x1B
        # lv.KEY.DEL = 0x7F
        # lv.KEY.NEXT = 0x09
        # lv.KEY.PREV = 0x0B
        # lv.KEY.HOME = 0x02
        # lv.KEY.END = 0x03
        self._device.read(buf=self._mv)
        key = self._buf[0]

        if key == 0x00:  # no key
            key = None
        elif key == 0x08:  # backspace
            key = lv.KEY.BACKSPACE  # 0x08  # NOQA
        elif key == 0x0D:  # enter
            key = lv.KEY.ENTER  # 0x0A  # NOQA
        elif key == 0x0C:  # alt + c
            key = None

        return key


    def _read(self, drv, data):  # NOQA
        key = self._get_key()

        if key is None:  # ignore no key
            if self._current_state != lv.INDEV_STATE.RELEASED:  # NOQA
                self._current_state = lv.INDEV_STATE.RELEASED  # NOQA
                res = True
            else:
                res = False

            data.key = self._last_key
            data.state = self._current_state
            data.continue_reading = False
            return res

        state, key = key

        self._last_key = key

        if self._current_state == state == lv.INDEV_STATE.RELEASED:  # NOQA
            res = False
            data.continue_reading = False
        else:
            res = True
            data.continue_reading = True

        self._current_state = state

        data.key = self._last_key
        data.state = self._current_state

        return res

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
