# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import io_expander_framework


EXIO1 = 0x01
EXIO2 = 0x02
EXIO3 = 0x03
EXIO4 = 0x04
EXIO5 = 0x05
EXIO6 = 0x06
EXIO7 = 0x07
EXIO8 = 0x08
EXIO9 = 0x09
EXIO10 = 0x0A
EXIO11 = 0x0B
EXIO12 = 0x0C
EXIO13 = 0x0D
EXIO14 = 0x0E
EXIO15 = 0x0F
EXIO16 = 0x10

_INPUT_PORT_REG = const(0x00)
_OUTPUT_PORT_REG = const(0x02)
_POLARITY_INVERSION_REG = const(0x04)
_CONFIGURATION_REG = const(0x06)

# 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, or 0x27
I2C_ADDR = 0x20
BITS = 8


class Pin(io_expander_framework.Pin):
    _config_settings = 0x00
    _output_states = 0x00
    _reg_int_pins = []

    @property
    def __bit(self):
        return 1 << self._id

    def __read_reg(self, reg):
        self._buf[0] = 0
        self._buf[1] = 0
        Pin._device.read_mem(reg, buf=self._mv)
        return self._buf[0] << 8 | self._buf[1]

    def __write_reg(self, reg, value):
        self._buf[0] = value >> 8 & 0xFF
        self._buf[1] = value & 0xFF
        Pin._device.write_mem(reg, buf=self._mv)

    def _set_dir(self, direction):
        if direction == self.OUT:
            Pin._config_settings &= ~self.__bit
        elif direction == self.IN:
            Pin._config_settings |= self.__bit
        else:
            raise ValueError('OPEN_DRAIN is not supported')

        self.__write_reg(_CONFIGURATION_REG, Pin._config_settings)

    def _set_level(self, level):
        if self._mode == self.OUT:
            if level:
                states = Pin._output_states | self.__bit
            else:
                states = Pin._output_states & ~self.__bit

            # 0nly set if there is an actual change
            if states != Pin._output_states:
                self.__write_reg(_OUTPUT_PORT_REG, Pin._output_states)
                Pin._output_states = states

    def _get_level(self):
        if self._mode == self.IN:
            states = self.__read_reg(_INPUT_PORT_REG)
        elif self._mode == self.OUT:
            states = Pin._output_states
        else:
            raise ValueError('Unsupported pin mode')

        return int(bool(states & self.__bit))

    def _set_irq(self, handler, trigger):
        pass

    def _set_pull(self, pull):
        pass
