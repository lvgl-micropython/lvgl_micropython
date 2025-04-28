# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import io_expander_framework


EXIO1 = 0x01
EXIO2 = 0x02
EXIO3 = 0x03
EXIO4 = 0x04
EXIO5 = 0x05
EXIO6 = 0x06
EXIO7 = 0x07
EXIO8 = 0x08


# 0x40, 0x42, 0x44, 0x46, 0x48, 0x4A, 0x4C, 0x4E
I2C_ADDR = 0x40
BITS = 8


class Pin(io_expander_framework.Pin):
    _output_states = 0xFF
    _reg_int_pins = []

    @property
    def __bit(self):
        return 1 << self._id

    def __read_reg(self):
        self._buf[0] = 0
        Pin._device.read(buf=self._mv[:1])
        return self._buf[0]

    def __write_reg(self, value):
        self._buf[0] = value & 0xFF
        Pin._device.write(buf=self._mv[:1])

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
