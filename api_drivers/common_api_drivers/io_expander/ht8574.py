# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import io_expander_framework


EXIO1 = 1
EXIO2 = 2
EXIO3 = 3
EXIO4 = 4
EXIO5 = 5
EXIO6 = 6
EXIO7 = 7
EXIO8 = 8


BITS = 8


class Pin(io_expander_framework.Pin):
    _output_states = 0xFF

    _device = None

    @classmethod
    def set_device(cls, device):
        if cls._device is not None:
            raise ValueError('device has already been set')

        cls._device = device

    def __init__(self, id, mode=-1, value=None):  # NOQA
        if Pin._device is None:
            raise RuntimeError('The expander device has not been set')

        super().__init__(id, mode, value)

    @property
    def __bit(self):
        return 1 << self._id

    def __read_reg(self):
        self._buf[0] = 0
        self._device.read(buf=self._mv[:1])
        return self._buf[0]

    def __write_reg(self, value):
        self._buf[0] = value & 0xFF
        self._device.write(buf=self._mv[:1])

    def _set_dir(self, direction):
        pass

    def _set_level(self, level):
        if level:
            Pin._output_states |= self.__bit
        else:
            Pin._output_states &= ~self.__bit

        self.__write_reg(Pin._output_states)

    def _get_level(self):
        if self._mode == self.IN:
            states = self.__read_reg()
            return (states >> self._id) & 0x1
        else:
            return (Pin._output_states >> self._id) & 0x1
