# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import lvgl as lv  # NOQA
import keypad_framework


class Keyboard(keypad_framework.KeypadDriver):
    def __init__(self, device, debug=False):  # NOQA
        self._device = device
        self._debug = debug
        super().__init__()

    def _get_key(self):
        # lv.KEY.ESC = 0x1B
        # lv.KEY.DEL = 0x7F
        # lv.KEY.NEXT = 0x09
        # lv.KEY.PREV = 0x0B
        # lv.KEY.HOME = 0x02
        # lv.KEY.END = 0x03
        key = bytearray(self._device.read(1))[0]
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
            else:
                k = hex(k)
            print('RAW KEY:', hex(k))

        return self.PRESSED, key

