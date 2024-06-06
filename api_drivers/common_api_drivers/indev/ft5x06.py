from micropython import const
import pointer_framework

_DEV_MODE = const(0x00)
_GEST_ID = const(0x01)
_TD_STATUS = const(0x02)

_I2C_SLAVE_ADDR = const(0x38)
_FT5x06_CHIPID = const(0x55)


# Register of the current mode
_DEV_MODE_REG = const(0x00)

# ** Possible modes as of FT6X36_DEV_MODE_REG **
_DEV_MODE_WORKING = const(0x00)


# Status register: stores number of active touch points (0, 1, 2)
_TD_STAT_REG = const(0x02)


_MSB_MASK = const(0x0F)
_LSB_MASK = const(0xFF)

# Report rate in Active mode
_PERIOD_ACTIVE_REG = const(0x88)
_VENDID = const(0x11)
_CHIPID_REG = const(0xA3)

_FIRMWARE_ID_REG = const(0xA6)
_RELEASECODE_REG = const(0xAF)
_PANEL_ID_REG = const(0xA8)

_G_MODE = const(0xA4)


class FT5x06(pointer_framework.PointerDriver):

    def _i2c_read8(self, register_addr):
        self._i2c.read_mem(register_addr, buf=self._mv[:1])
        return self._buf[0]

    def _i2c_write8(self, register_addr, data):
        self._buf[0] = data
        self._i2c.write_mem(register_addr, self._mv[:1])

    def __init__(self, i2c_bus, touch_cal=None, debug=False):  # NOQA
        self._buf = bytearray(5)
        self._mv = memoryview(self._buf)

        self._i2c_bus = i2c_bus
        self._i2c = i2c_bus.add_device(_I2C_SLAVE_ADDR, 8)

        venid = self._i2c_read8(_PANEL_ID_REG)
        print("Touch Device ID: 0x%02x" % venid)

        chipid = self._i2c_read8(_CHIPID_REG)
        print("Touch Chip ID: 0x%02x" % chipid)

        data = self._i2c_read8(_DEV_MODE_REG)
        print("Touch Device mode: 0x%02x" % data)

        data = self._i2c_read8(_FIRMWARE_ID_REG)
        print("Touch Firmware ID: 0x%02x" % data)

        data = self._i2c_read8(_RELEASECODE_REG)
        print("Touch Release code: 0x%02x" % data)

        if chipid not in (_FT5x06_CHIPID,):
            raise RuntimeError()

        if venid != _VENDID:
            raise RuntimeError()

        self._i2c_write8(_DEV_MODE_REG, _DEV_MODE_WORKING)
        self._i2c_write8(_PERIOD_ACTIVE_REG, 0x0E)
        self._i2c_write8(_G_MODE, 0x00)
        super().__init__(touch_cal=touch_cal, debug=debug)

    def _get_coords(self):
        buf = self._buf
        mv = self._mv

        try:
            self._i2c.read_mem(_TD_STAT_REG, buf=mv)
        except OSError:
            return None

        touch_pnt_cnt = buf[0]

        if touch_pnt_cnt != 1:
            return None

        x = (
            ((buf[1] & _MSB_MASK) << 8) |
            (buf[2] & _LSB_MASK)
        )
        y = (
            ((buf[3] & _MSB_MASK) << 8) |
            (buf[4] & _LSB_MASK)
        )

        x = round((x & 0xFFF) / 2.24)
        y = round((y & 0xFFF) / 2.14)

        return self.PRESSED, x, y
