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
            return None
        elif key == 0x08:  # backspace
            key = lv.KEY.BACKSPACE  # 0x08  # NOQA
        elif key == 0x0D:  # enter
            key = lv.KEY.ENTER  # 0x0A  # NOQA
        elif key == 0x0C:  # alt + c
            return None

        return self.PRESSED, key

