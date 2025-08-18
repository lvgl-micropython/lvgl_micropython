# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import lvgl as lv  # NOQA
import keypad_framework
from micropython import const  # NOQA
import lcd_utils

_LILYGO_KB_BRIGHTNESS_CMD = const(0x01)
_LILYGO_KB_ALT_B_BRIGHTNESS_CMD = const(0x02)

group = lv.group_create()
group.set_default()


class Keyboard(keypad_framework.KeypadDriver):
    def __init__(self, device, debug=False):  # NOQA
        self._device = device
        self._debug = debug
        self._brightness = 0
        self._brightness_default = 127

        self._buffer = bytearray(1)
        self._mv = memoryview(self._buffer)

        super().__init__()

        self.set_group(group)
        self.enable(1)

    def set_default_brioghtness(self, value):
        value = lcd_utils.remap(float(value), 0.0, 100.0, 30.0, 255.0)
        value = int(round(value))

        # clamp value at 29 - 255
        if value <= 30:
            value = 0

        if value > 255:
            value = 255

        self._brightness_default = value

        value = bytearray([value])
        self._device.write_mem(_LILYGO_KB_ALT_B_BRIGHTNESS_CMD, value)

    def get_brightness_default(self):
        value = self._brightness_default
        if value == 0:
            value = 30

        value = lcd_utils.remap(float(value), 30.0, 255.0, 0.0, 100.0)
        return round(value, 1)

    def set_brightness(self, value):
        value = lcd_utils.remap(float(value), 0.0, 100.0, 30.0, 255.0)
        value = int(round(value))

        # clamp value at 29 - 255
        if value <= 30:
            value = 0

        if value > 255:
            value = 255

        self._brightness = value
        value = bytearray([value])
        self._device.write_mem(_LILYGO_KB_BRIGHTNESS_CMD, value)

    def get_brightness(self):
        value = self._brightness
        if value == 0:
            value = 30

        value = lcd_utils.remap(float(value), 30.0, 255.0, 0.0, 100.0)
        return round(value, 1)

    def _get_key(self):
        # lv.KEY.ESC = 0x1B
        # lv.KEY.DEL = 0x7F
        # lv.KEY.NEXT = 0x09
        # lv.KEY.PREV = 0x0B
        # lv.KEY.HOME = 0x02
        # lv.KEY.END = 0x03

        self._device.read(buf=self._mv)
        key = self._buffer[0]

        if key == 0x00:  # no key
            return None
        elif key == 0x08:  # backspace
            key = lv.KEY.BACKSPACE  # 0x08  # NOQA
        elif key == 0x0D:  # enter
            key = lv.KEY.ENTER  # 0x0A  # NOQA
        elif key == 0x0C:  # alt + c
            return None

        if self._debug:
            k = key
            # check if key is in human readable ascii range and if it
            # is then convert it form it's decimal value to the actual ascii
            # key else convert to hex
            if 127 > k >= 32:
                k = chr(k)
                print('RAW KEY:', k)
            else:
                k = hex(k)
                print('RAW KEY:', hex(k))

        return self.PRESSED, key

