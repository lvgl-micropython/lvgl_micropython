# this driver uses a special i2c bus implimentation I have written.
# This implimentation takes into consideration the ESP32 and it having
# threading available. It also has some convience methods built into it
# that figure out what is wanting to be done automatically.
# read more about it's use in the stub files.


import pointer_framework
import i2c

from micropython import const

_GT911_ADDRESS = const(0x5D)
_GT911_READ_XY_REG = const(0x814E)
_GT911_CONFIG_REG = const(0x8047)
_GT911_PRODUCT_ID_REG = const(0x8140)


class GT911(pointer_framework.PointerDriver):

    def __init__(self, i2c_bus, touch_cal=None):
        self._buf = bytearray(5)
        self._mv = memoryview(self._buf)
        self._i2c = i2c.I2CDevice(i2c_bus, _GT911_ADDRESS, reg_bits=16)
        super().__init__(touch_cal=touch_cal)

    def _get_coords(self):
        buf = self._buf
        mv = self._mv
        self._i2c.read_mem(_GT911_READ_XY_REG, buf=mv)

        if (buf[0] & 0x80) == 0x00:
            buf[0] = 0x00
            self._i2c.write_mem(_GT911_READ_XY_REG, mv[:1])
            return None
        else:
            touch_cnt = buf[0] & 0x0F
            if touch_cnt > 5 or touch_cnt == 0:
                buf[0] = 0x00
                self._i2c.write_mem(_GT911_READ_XY_REG, mv[:1])
                return None

            self._i2c.read_mem(_GT911_READ_XY_REG + 1, buf=mv[:5])
            x = (buf[2] << 8) + buf[1]
            y = (buf[4] << 8) + buf[3]

            buf[0] = 0x00
            self._i2c.write_mem(_GT911_READ_XY_REG, mv[:1])

            return x, y

