from micropython import const  # NOQA
import io_expander_framework
import machine

EXIO1 = 0x01
EXIO2 = 0x02
EXIO3 = 0x03
EXIO4 = 0x04
EXIO5 = 0x05
EXIO6 = 0x06
EXIO7 = 0x07
EXIO8 = 0x08


I2C_ADDR = 0x24  # 36 00100100
_BITS = const(8)

_CH422G_REG_IN = const(0x26)  # 38  00100110
_CH422G_REG_OUT = const(0x38)  # 56 00111000

_REG_INPUT = const(0)
_REG_OUTPUT = const(1)
_REG_DIRECTION = const(2)


class Pin(io_expander_framework.Pin):
    _output_states = 0x00
    _i2c_bus = None
    _reg_in = None
    _reg_out = None
    _reg_int_pins = []

    @classmethod
    def set_device(cls, device):
        if cls._device is not None:
            raise ValueError('device has already been set')

        cls._device = device
        cls._device.set_mem_addr_size(_BITS)

        cls._reg_in = machine.I2C.Device(
            i2c_bus=device.get_bus(),  # NOQA
            addr=_CH422G_REG_IN,

        )
        cls._reg_in.set_mem_addr_size(_BITS)

        cls._reg_out = machine.I2C.Device(
            i2c_bus=device.get_bus(),  # NOQA
            addr=_CH422G_REG_OUT
        )
        cls._reg_out.set_mem_addr_size(_BITS)

    @property
    def __bit(self):
        return 1 << self._id

    @property
    def _mode(self):
        return 1

    @_mode.setter
    def _mode(self, value):
        pass

    def _write_reg(self, reg, value):
        if reg == _REG_OUTPUT:
            value &= 0xFF

            self._buf[0] = 0x01
            self._device.write(self._mv[:1])

            self._buf[0] = value
            # _i2c_device.write_mem(_CH422G_REG_OUT, self._mv[:1])
            self._reg_out.write(self._mv[:1])
            self._output = value

        elif reg == _REG_DIRECTION:
            value &= 0xFF
            self._mode = value

    def _read_reg(self, reg):
        if reg == _REG_INPUT:
            # _i2c_device.read_mem(_CH422G_REG_IN, buf=self._mv[:1])
            self._reg_in.readinto(self._mv[:1])
            return self._buf[0]
        elif reg == _REG_OUTPUT:
            return self._output
        elif reg == _REG_DIRECTION:
            return self._mode

        return None

    def _set_dir(self, direction):
        if direction == self.OPEN_DRAIN:
            raise ValueError('OPEN_DRAIN is not supported')

    def _set_level(self, level):
        if self._mode == self.OUT:
            if level:
                # 3. High level && Set 0 to output high
                # 4. Low level && Set 0 to output low
                states = Pin._output_states & ~self.__bit
            else:
                # 1. High level && Set 1 to output high
                # 2. Low level && Set 1 to output low
                states = Pin._output_states | self.__bit

            # Write to reg only when different */
            if states != Pin._output_states:
                self._write_reg(_REG_OUTPUT, states)
                Pin._output_states = states

    def _get_level(self):
        if self._mode == self.IN:
            states = ~self._read_reg(_REG_INPUT)
        elif self._mode == self.OUT:
            states = Pin._output_states
        else:
            raise ValueError('Unsupported pin mode')

        return int(bool(states & self.__bit))

    def _set_irq(self, handler, trigger):
        pass

    def _set_pull(self, pull):
        pass
