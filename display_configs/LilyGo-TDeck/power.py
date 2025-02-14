import machine  # NOQA

import lvgl as lv
import time


class Button:

    def __init__(self, pin):
        self._pin = machine.Pin(pin, machine.Pin.IN)

        if not lv.is_initialized():
            lv.init()

        # press duration <= this number will call the sleep callback
        self._sleep_time = 75
        self._sleep_callback = None
        # press duration > sleep_time and <= this number will call the menu callback
        self._menu_time = 150
        self._menu_callback = None
        # press duration >= this number will call the power off callback
        self._power_off_time = 3000
        self._power_off_callback = None

        self._timer = lv.timer_create_basic()
        self._timer.pause()
        self._timer.set_cb(self.__callback)
        self._timer.set_period(33)
        self._timer.set_repeat_count(-1)
        self._timer.resume()

    def set_menu_time(self, val):
        self._menu_time = val

    def get_menu_time(self):
        return self._menu_time

    def set_menu_callback(self, cb):
        self._menu_callback = cb

    def get_menu_callback(self):
        return self._menu_callback

    def set_sleep_time(self, val):
        self._sleep_time = val

    def get_sleep_time(self):
        return self._sleep_time

    def set_sleep_callback(self, cb):
        self._sleep_callback = cb

    def get_sleep_callback(self):
        return self._sleep_callback

    def set_power_off_time(self, val):
        self._power_off_time = val

    def get_power_off_time(self):
        return self._power_off_time

    def set_power_off_callback(self, cb):
        self._power_off_callback = cb

    def get_power_off_callback(self):
        return self._power_off_callback

    def __callback(self, _):
        value = self._pin.value()

        if value:
            start_time = time.ticks_ms()  # NOQA

            while value:
                value = self._pin.value()
                time.sleep_ms(1)  # NOQA

            end_time = time.ticks_ms()  # NOQA
            diff = time.ticks_diff(end_time, start_time)  # NOQA

            if diff <= self._sleep_time:
                if self._sleep_callback is not None:
                    self._sleep_callback(self._pin)

            elif diff <= self._menu_time:
                if self._menu_callback is not None:
                    self._menu_callback()

            elif diff >= self._power_off_time:
                if self._power_off_callback is not None:
                    self._power_off_callback(self._pin)
