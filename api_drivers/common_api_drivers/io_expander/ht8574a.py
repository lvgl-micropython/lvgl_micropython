# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import io_expander_framework
from micropython import const


EXIO1 = 0x01
EXIO2 = 0x02
EXIO3 = 0x03
EXIO4 = 0x04
EXIO5 = 0x05
EXIO6 = 0x06
EXIO7 = 0x07
EXIO8 = 0x08


# 0x70, 0x72, 0x74, 0x76, 0x78, 0x7A, 0x7C, 0x7E
I2C_ADDR = 0x70
_BITS = const(8)


class Pin(io_expander_framework.Pin):
    _output_states = 0xFF

    _int_pin = None
    _reg_int_pins = []
    _device = None

    @classmethod
    def set_device(cls, device):
        if cls._device is not None:
            raise ValueError('device has already been set')

        cls._device = device
        cls._device.set_mem_addr_size(_BITS)

    @property
    def __bit(self):
        return 1 << self._id

    def __read_reg(self):
        self._buf[0] = 0
        self._device.readinto(self._mv[:1])
        return self._buf[0]

    def __write_reg(self, value):
        self._buf[0] = value & 0xFF
        self._device.write(self._mv[:1])

    def _set_dir(self, direction):
        if direction == self.OPEN_DRAIN:
            raise ValueError('OPEN_DRAIN is not supported')

    def _set_level(self, level):
        if self._mode == self.OUT:
            if level:
                states = Pin._output_states | self.__bit
            else:
                states = Pin._output_states & ~self.__bit

            # 0nly set if there is an actual change
            if states != Pin._output_states:
                self.__write_reg(Pin._output_states)
                Pin._output_states = states

    def _get_level(self):
        if self._mode == self.IN:
            states = self.__read_reg()
        elif self.mode == self.OUT:
            states = Pin._output_states
        else:
            raise ValueError('Unsupported pin mode')

        return int(bool(states & self.__bit))

    def _set_irq(self, handler, trigger):
        pass

    def _set_pull(self, pull):
        pass
