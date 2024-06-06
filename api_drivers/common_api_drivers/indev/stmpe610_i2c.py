from micropython import const  # NOQA
import pointer_framework
import time


_ADDR = const(0x41)

_SYS_CTRL1_REG = const(0x03)
_SYS_CTRL1_RESET = const(0x02)
_SYS_CTRL2_REG = const(0x04)

_TSC_CTRL_REG = const(0x40)
_TSC_CTRL_EN = const(0x01)
_TSC_CTRL_XYZ = const(0x00)

_INT_CTRL_REG = const(0x09)
_INT_CTRL_DISABLE = const(0x00)

_INT_EN_REG = const(0x0A)
_INT_EN_TOUCHDET = const(0x01)

_INT_STA_REG = const(0x0B)
_INT_STA_TOUCHDET = const(0x01)

_ADC_CTRL1_REG = const(0x20)
_ADC_CTRL1_10BIT = const(0x00)

_ADC_CTRL2_REG = const(0x21)
_ADC_CTRL2_6_5MHZ = const(0x02)

_TSC_CFG_REG = const(0x41)
_TSC_CFG_4SAMPLE = const(0x80)
_TSC_CFG_DELAY_1MS = const(0x20)
_TSC_CFG_SETTLE_5MS = const(0x04)

_FIFO_TH_REG = const(0x4A)
_FIFO_SIZE_REG = const(0x4C)

_FIFO_STA_REG = const(0x4B)
_FIFO_STA_RESET = const(0x01)

_TSC_I_DRIVE_REG = const(0x58)
_TSC_I_DRIVE_50MA = const(0x01)

_TSC_FRACTION_Z_REG = const(0x56)


class STMPE610(pointer_framework.PointerDriver):

    def __init__(self, i2c_bus, touch_cal=None, debug=False):
        self._buf = bytearray(4)
        self._mv = memoryview(self._buf)
        self._i2c_bus = i2c_bus
        self._i2c = i2c_bus.add_device(_ADDR, 8)

        self._buf[0] = _SYS_CTRL1_RESET
        self._i2c.write_mem(_SYS_CTRL1_REG, buf=self._mv[:1])
        time.sleep(0.001)

        self._buf[0] = 0x00
        self._i2c.write_mem(_SYS_CTRL2_REG, buf=self._mv[:1])

        self._buf[0] = _TSC_CTRL_XYZ | _TSC_CTRL_EN
        self._i2c.write_mem(_TSC_CTRL_REG, buf=self._mv[:1])

        self._buf[0] = _INT_EN_TOUCHDET
        self._i2c.write_mem(_INT_EN_REG, buf=self._mv[:1])

        self._buf[0] = _ADC_CTRL1_10BIT | (0x6 << 4)
        self._i2c.write_mem(_ADC_CTRL1_REG, buf=self._mv[:1])

        self._buf[0] = _ADC_CTRL2_6_5MHZ
        self._i2c.write_mem(_ADC_CTRL2_REG, buf=self._mv[:1])

        self._buf[0] = _TSC_CFG_4SAMPLE | _TSC_CFG_DELAY_1MS | _TSC_CFG_SETTLE_5MS
        self._i2c.write_mem(_TSC_CFG_REG, buf=self._mv[:1])

        self._buf[0] = 0x06
        self._i2c.write_mem(_TSC_FRACTION_Z_REG, buf=self._mv[:1])

        self._buf[0] = 0x01
        self._i2c.write_mem(_FIFO_TH_REG, buf=self._mv[:1])

        self._buf[0] = _FIFO_STA_RESET
        self._i2c.write_mem(_FIFO_STA_REG, buf=self._mv[:1])

        self._buf[0] = 0x00
        self._i2c.write_mem(_FIFO_STA_REG, buf=self._mv[:1])

        self._buf[0] = _TSC_I_DRIVE_50MA
        self._i2c.write_mem(_TSC_I_DRIVE_REG, buf=self._mv[:1])

        self._buf[0] = 0xFF
        self._i2c.write_mem(_INT_STA_REG, buf=self._mv[:1])

        self._buf[0] = _INT_CTRL_DISABLE
        self._i2c.write_mem(_INT_CTRL_REG, buf=self._mv[:1])

        super().__init__(touch_cal=touch_cal, debug=debug)

    def _get_coords(self):
        self._i2c.read_mem(_FIFO_SIZE_REG, self._mv[:1])
        touch_count = self._buf[0]

        if not touch_count:
            return None

        while touch_count:
            self._i2c.read_mem(0xD7, buf=self._mv)
            touch_count -= 1

        x = self._buf[0] << 4 | self._buf[1] >> 4
        y = (self._buf[1] & 0xF) << 8 | self._buf[2]

        self._buf[0] = 0xFF
        self._i2c.write_mem(_INT_STA_REG, self._mv[:1])

        return self.PRESSED, x, y
