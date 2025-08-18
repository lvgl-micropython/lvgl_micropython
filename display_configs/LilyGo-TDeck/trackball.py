# Copyright (c) 2024 - 2025 Kevin G. Schlosser
import lvgl as lv  # NOQA
import keypad_framework
import machine
from micropython import const  # NOQA
import keyboard_s3

_UP = const(0x01)
_DOWN = const(0x02)
_LEFT = const(0x04)
_RIGHT = const(0x08)
_PRESS = const(0x10)


group = keyboard_s3.group


class TrackBall(keypad_framework.KeypadDriver):

    def __init__(self, up_pin=3, down_pin=15, left_pin=1, right_pin=2, press_pin=0):  # NOQA

        self._up_pin = machine.Pin(up_pin, machine.Pin.IN)
        self._down_pin = machine.Pin(down_pin, machine.Pin.IN)
        self._left_pin = machine.Pin(left_pin, machine.Pin.IN)
        self._right_pin = machine.Pin(right_pin, machine.Pin.IN)
        self._press_pin = machine.Pin(press_pin, machine.Pin.IN)
        self._key = 0x00

        self._key_state = self.RELEASED

        super().__init__()

        self.set_group(g)
        self.enable(1)

        self._indev_drv.set_mode(lv.INDEV_MODE.EVENT)  # NOQA
        self.__timer = lv.timer_create(self.__callback, 33, None)  # NOQA
        self.__timer.set_repeat_count(-1)  # NOQA

    def __callback(self, _):
        self.read()
        last_state = self._key_state

        while self._key_state == self.PRESSED:
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
            if self._press_pin.value():
                key |= _PRESS

        elif self._current_state == self.PRESSED:
            return self.RELEASED, self._last_key

        if key == 0x00:
            self._key = key
            self._key_state = self.RELEASED
            return self.RELEASED, self._last_key
        else:
            self._key_state = self.PRESSED

        if key & _UP:
            key &= ~_UP
            self._key = key
            return self.PRESSED, lv.KEY.UP
        elif key & _DOWN:
            key &= ~_DOWN
            self._key = key
            return self.PRESSED, lv.KEY.DOWN
        elif key & _LEFT:
            key &= ~_LEFT
            self._key = key
            return self.PRESSED, lv.KEY.LEFT
        elif key & _RIGHT:
            key &= ~_RIGHT
            self._key = key
            return self.PRESSED, lv.KEY.RIGHT
        elif key & _PRESS:
            key &= ~_PRESS
            self._key = key
            return self.PRESSED, lv.KEY.ENTER
        else:
            self._key = 0x00
            return None
