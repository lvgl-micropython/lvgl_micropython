# this driver uses a special i2c bus implimentation I have written.
# This implimentation takes into consideration the ESP32 and it having
# threading available. It also has some convience methods built into it
# that figure out what is wanting to be done automatically.
# read more about it's use in the stub files.

from micropython import const
import pointer_framework
import i2c
import machine
import time


_CONFIG_START_REG_HIGH = const(0x80)
_CONFIG_START_REG_LOW = const(0x47)

_X_OUTPUT_MAX_LOW_POS = const(0x01)
_X_OUTPUT_MAX_LOW_REG_HIGH = const(0x80)
_X_OUTPUT_MAX_LOW_REG_LOW = const(0x48)

_X_OUTPUT_MAX_HIGH_POS = const(0x02)
_X_OUTPUT_MAX_HIGH_REG_HIGH = const(0x80)
_X_OUTPUT_MAX_HIGH_REG_LOW = const(0x49)

_Y_OUTPUT_MAX_LOW_POS = const(0x03)
_Y_OUTPUT_MAX_LOW_REG_HIGH = const(0x80)
_Y_OUTPUT_MAX_LOW_REG_LOW = const(0x4A)

_Y_OUTPUT_MAX_HIGH_POS = const(0x04)
_Y_OUTPUT_MAX_HIGH_REG_HIGH = const(0x80)
_Y_OUTPUT_MAX_HIGH_REG_LOW = const(0x4B)

_CONFIG_CHKSUM_POS = const(0x88)
_CONFIG_CHKSUM_REG_HIGH = const(0x80)
_CONFIG_CHKSUM_REG_LOW = const(0xFF)

_CONFIG_FRESH_REG_HIGH = const(0x81)
_CONFIG_FRESH_REG_LOW = const(0x00)

_READ_XY_REG_HIGH = const(0x81)
_READ_XY_REG_LOW = const(0x4E)

_POINT_1_REG_HIGH = const(0x81)
_POINT_1_REG_LOW = const(0x4F)

_POINT_2_RE_HIGHG = const(0x81)
_POINT_2_REG_LOW = const(0x57)

_POINT_3_REG_HIGH = const(0x81)
_POINT_3_REG_LOW = const(0x5F)

_POINT_4_REG_HIGH = const(0x81)
_POINT_4_REG_LOW = const(0x67)

_POINT_5_REG_HIGH = const(0x81)
_POINT_5_REG_LOW = const(0x6F)

_PRODUCT_ID_REG_HIGH = const(0x81)
_PRODUCT_ID_REG_LOW = const(0x40)

_FIRMWARE_VERSION_HIGH = const(0x81)
_FIRMWARE_VERSION_LOW = const(0x44)

_X_CORD_RES_HIGH = const(0x81)
_X_CORD_RES_LOW = const(0x46)

_Y_CORD_RES_HIGH = const(0x81)
_Y_CORD_RES_LOW = const(0x48)

_VENDOR_ID_HIGH = const(0x81)
_VENDOR_ID_LOW = const(0x4A)


_ADDR1 = const(0x5D)
_ADDR2 = const(0x14)


class GT911(pointer_framework.PointerDriver):

    def __init__(self, i2c_bus, reset_pin=None, interrupt_pin=None, touch_cal=None):
        buf = self._buf = bytearray(7)
        mv = self._mv = memoryview(self._buf)

        if _ADDR1 in i2c_bus.scan():
            self._addr = _ADDR1
        else:
            self._addr = _ADDR2

        self._i2c = i2c.I2CDevice(i2c_bus, self._addr)

        super().__init__(touch_cal=touch_cal)

        if isinstance(reset_pin, int):
            reset_pin = machine.Pin(reset_pin, machine.Pin.OUT)

        if isinstance(interrupt_pin, int):
            interrupt_pin = machine.Pin(interrupt_pin, machine.Pin.OUT)

        self._reset_pin = reset_pin
        self._interrupt_pin = interrupt_pin

        self.touch_reset()

        buf[0] = _PRODUCT_ID_REG_LOW
        buf[1] = _PRODUCT_ID_REG_HIGH
        self._i2c.write(mv[:2])

        buf[0] = 0x00
        buf[1] = 0x00
        self._i2c.read(buf=mv[:4])

        print('Product id:', buf[:4].decode('utf-8'))

        buf[0] = _FIRMWARE_VERSION_LOW
        buf[1] = _FIRMWARE_VERSION_HIGH
        self._i2c.write(mv[:2])

        buf[0] = 0x00
        buf[1] = 0x00
        self._i2c.read(buf=mv[:2])  # 2 bytes low byte first

        print('Firmware version:', hex(buf[0] + (buf[1] << 8)))

        buf[0] = _VENDOR_ID_LOW
        buf[1] = _VENDOR_ID_HIGH
        self._i2c.write(mv[:2])

        buf[0] = 0x00
        buf[1] = 0x00
        self._i2c.read(buf=mv[:1])

        print('Vendor id:', hex(buf[0]))

        buf[0] = _X_CORD_RES_LOW
        buf[1] = _X_CORD_RES_HIGH
        self._i2c.write(mv[:2])

        buf[0] = 0x00
        buf[1] = 0x00
        self._i2c.read(buf=mv[:2])  # 2 bytes low byte first

        print('Configured width:', buf[0] + (buf[1] << 8))

        buf[0] = _Y_CORD_RES_LOW
        buf[1] = _Y_CORD_RES_HIGH
        self._i2c.write(mv[:2])

        buf[0] = 0x00
        buf[1] = 0x00
        self._i2c.read(buf=mv[:2])  # 2 bytes low byte first

        print('Configured height:', buf[0] + (buf[1] << 8))

    def touch_reset(self):
        if self._interrupt_pin and self._reset_pin:
            self._interrupt_pin.init(self._interrupt_pin.OUT)
            self._interrupt_pin(0)
            self._reset_pin(0)
            time.sleep_ms(10)
            self._interrupt_pin(int(self._addr == _ADDR2))
            time.sleep_ms(1)
            self._reset_pin(1)
            time.sleep_ms(5)
            self._interrupt_pin(0)
            time.sleep_ms(50)
            self._interrupt_pin.init(self._interrupt_pin.IN)
            time.sleep_ms(50)

    def _reflash_config(self, width, height):
        buf = bytearray(185)
        mv = memoryview(buf)

        buf[0] = _CONFIG_START_REG_LOW
        buf[1] = _CONFIG_START_REG_HIGH
        self._i2c.write(buf[:2])

        buf[0] = 0x00
        buf[1] = 0x00
        self._i2c.read(buf=mv)

        buf[_X_OUTPUT_MAX_LOW_POS] = width & 0xFF
        buf[_X_OUTPUT_MAX_HIGH_POS] = (width >> 8) & 0xFF
        buf[_Y_OUTPUT_MAX_LOW_POS] = height & 0xFF
        buf[_Y_OUTPUT_MAX_HIGH_POS] = (height >> 8) & 0xFF

        checksum = ((~sum(buf)) + 1) & 0xFF

        buf[0] = _X_OUTPUT_MAX_LOW_REG_LOW
        buf[1] = _X_OUTPUT_MAX_LOW_REG_HIGH
        buf[2] = width & 0xFF
        self._i2c.write(mv[:3])

        buf[0] = _X_OUTPUT_MAX_HIGH_REG_LOW
        buf[1] = _X_OUTPUT_MAX_HIGH_REG_HIGH
        buf[2] = (width >> 8) & 0xFF
        self._i2c.write(mv[:3])

        buf[0] = _Y_OUTPUT_MAX_LOW_REG_LOW
        buf[1] = _Y_OUTPUT_MAX_LOW_REG_HIGH
        buf[2] = height & 0xFF
        self._i2c.write(mv[:3])

        buf[0] = _Y_OUTPUT_MAX_HIGH_REG_LOW
        buf[1] = _Y_OUTPUT_MAX_HIGH_REG_HIGH
        buf[2] = (height >> 8) & 0xFF
        self._i2c.write(mv[:3])

        buf[0] = _CONFIG_CHKSUM_REG_LOW
        buf[1] = _CONFIG_CHKSUM_REG_HIGH
        buf[2] = checksum
        self._i2c.write(mv[:3])

        buf[0] = _CONFIG_FRESH_REG_LOW
        buf[1] = _CONFIG_FRESH_REG_HIGH
        buf[2] = 0x01
        self._i2c.write(mv[:3])

    def _get_coords(self):
        buf = self._buf
        mv = self._mv
        
        buf[0] = _READ_XY_REG_LOW
        buf[1] = _READ_XY_REG_HIGH
        
        self._i2c.write(mv[:2])
        
        buf[0] = 0x00
        self._i2c.read(buf=mv[:1])

        if (buf[0] & 0x80) == 0x00:
            buf[0] = _READ_XY_REG_LOW
            buf[1] = _READ_XY_REG_HIGH
            buf[2] = 0x00
            self._i2c.write(buf=mv[:3])
            return None
        else:
            touch_cnt = buf[0] & 0x0F
            if touch_cnt > 5 or touch_cnt == 0:
                buf[0] = _READ_XY_REG_LOW
                buf[1] = _READ_XY_REG_HIGH
                buf[2] = 0x00
                self._i2c.write(buf=mv[:3])
                return None

            buf[0] = _READ_XY_REG_LOW
            buf[1] = _READ_XY_REG_HIGH
            self._i2c.write(buf=mv[:2])
            buf[1] = 0x00
            buf[2] = 0x00
            buf[3] = 0x00
            buf[4] = 0x00

            self._i2c.read(buf=mv)

            x = buf[1] + (buf[2] << 8)
            y = buf[3] + (buf[4] << 8)

            buf[0] = _READ_XY_REG_LOW
            buf[1] = _READ_XY_REG_HIGH
            buf[2] = 0x00
            self._i2c.write(buf=mv[:3])

            return self.PRESSED, x, y
