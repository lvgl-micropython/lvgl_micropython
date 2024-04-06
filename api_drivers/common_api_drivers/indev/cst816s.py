from micropython import const  # NOQA
import pointer_framework


_ADDR = const(0x15)
_READ_REG = const(0x01)
_VERSION_REG = const(0x15)
_VERSION_INFO = const(0xA7)


class CST816S(pointer_framework.PointerDriver):

    def __init__(self, i2c_bus, touch_cal=None):
        self._buf = bytearray(8)
        self._mv = memoryview(self._buf)
        self._i2c_bus = i2c_bus
        self._i2c = i2c_bus.add_device(_ADDR, 8)

        self._i2c.read_mem(_VERSION_REG, buf=self._mv[:1])
        print('Touch version:', self._buf[0])

        self._i2c.read_mem(_VERSION_INFO, buf=self._mv[:3])
        print('Touch version info:', self._buf[:3])

        super().__init__(touch_cal=touch_cal)

    def _get_coords(self):
        self._i2c.read_mem(_READ_REG, buf=self._mv)
        x = ((self._buf[2] & 0xF) << 8) + self._buf[3]
        y = ((self._buf[4] & 0xF) << 8) + self._buf[5]

        if x or y:
            return self.PRESSED, x, y
