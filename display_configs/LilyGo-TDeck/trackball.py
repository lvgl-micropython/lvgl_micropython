# Copyright (c) 2024 - 2025 Kevin G. Schlosser
import lvgl as lv  # NOQA
import keypad_framework
import machine
from micropython import const  # NOQA

_UP = 0x01
_DOWN = 0x02
_LEFT = 0x04
_RIGHT = 0x08


class TrackBall(keypad_framework.KeypadDriver):

    def __init__(self, up_pin, down_pin, left_pin, right_pin):  # NOQA

        self._up_pin = machine.Pin(up_pin, machine.Pin.IN)
        self._down_pin = machine.Pin(down_pin, machine.Pin.IN)
        self._left_pin = machine.Pin(left_pin, machine.Pin.IN)
        self._right_pin = machine.Pin(right_pin, machine.Pin.IN)
        self._key = 0x00

        super().__init__()

        self._indev_drv.set_mode(lv.INDEV_MODE.EVENT)  # NOQA
        self.__timer = lv.timer_create(self.__callback, 33, None)  # NOQA
        self.__timer.set_repeat_count(-1)  # NOQA

    def __callback(self, _):
        self.read()
        last_state = self._current_state

        while self._current_state == self.PRESSED:
            # this might be too fast so I may have to put a stall in here.
            lv.refr_now(self._disp_drv)
            self.read()

        if last_state == self.PRESSED:
            lv.refr_now(self._disp_drv)

    def _get_key(self):
        key = self._key
        if key == 0x00:
            if self._up_pin.value():
                key |= _UP
            if self._down_pin.value():
                key |= _DOWN
            if self._left_pin.value():
                key |= _LEFT
            if self._right_pin.value():
                key |= _RIGHT

        if key & _UP:
            key &= ~_UP
            self._key = key
            return lv.KEY.UP
        elif key & _DOWN:
            key &= ~_DOWN
            self._key = key
            return lv.KEY.DOWN
        elif key & _LEFT:
            key &= ~_LEFT
            self._key = key
            return lv.KEY.LEFT
        elif key & _RIGHT:
            key &= ~_RIGHT
            self._key = key
            return lv.KEY.RIGHT
        else:
            self._key = 0x00
