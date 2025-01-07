# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import io_expander_framework


EXIO1 = 1
EXIO2 = 2
EXIO3 = 3
EXIO4 = 4
EXIO5 = 5
EXIO6 = 6
EXIO7 = 7
EXIO8 = 8
EXIO9 = 9
EXIO10 = 10
EXIO11 = 11
EXIO12 = 12
EXIO13 = 13
EXIO14 = 14
EXIO15 = 15
EXIO16 = 16

_INPUT_PORT_REG = const(0x00)
_OUTPUT_PORT_REG = const(0x02)
_POLARITY_INVERSION_REG = const(0x04)
_CONFIGURATION_REG = const(0x06)

# 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, or 0x27
I2C_ADDR = 0x20
BITS = 8


class Pin(io_expander_framework.Pin):
    _config_reg = 0
    _output_reg = 0

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

    def __read_reg(self, reg):
        self._buf[0] = 0
        self._buf[1] = 0
        self._device.read_mem(reg, buf=self._mv)
        return self._buf[0] << 8 | self._buf[1]

    def __write_reg(self, reg, value):
        self._buf[0] = value >> 8 & 0xFF
        self._buf[1] = value & 0xFF
        self._device.write_mem(reg, buf=self._mv)

    def _set_dir(self, direction):
        if direction:
            Pin._config_reg &= ~self.__bit
        else:
            Pin._config_reg |= self.__bit

        self.__write_reg(_CONFIGURATION_REG, Pin._config_reg)

    def _set_level(self, level):
        if level:
            Pin._output_reg |= self.__bit
        else:
            Pin._output_reg &= ~self.__bit

        self.__write_reg(_OUTPUT_PORT_REG, Pin._output_reg)

    def _get_level(self):
        if self._mode == self.OUT:
            Pin._output_reg = self.__read_reg(_OUTPUT_PORT_REG)
            return (Pin._output_reg >> self._id) & 0x1
        else:
            val = self.__read_reg(_INPUT_PORT_REG)
            return (val >> self._id) & 0x1
