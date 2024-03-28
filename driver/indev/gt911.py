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


_CONFIG_START_REG = const(0x8047)

_X_OUTPUT_MAX_LOW_POS = const(0x01)
_X_OUTPUT_MAX_LOW_REG = const(0x8048)

_X_OUTPUT_MAX_HIGH_POS = const(0x02)
_X_OUTPUT_MAX_HIGH_REG = const(0x8049)

_Y_OUTPUT_MAX_LOW_POS = const(0x03)
_Y_OUTPUT_MAX_LOW_REG = const(0x804A)

_Y_OUTPUT_MAX_HIGH_POS = const(0x04)
_Y_OUTPUT_MAX_HIGH_REG = const(0x804B)

_CONFIG_CHKSUM_POS = const(0x88)
_CONFIG_CHKSUM_REG = const(0x80FF)

_CONFIG_FRESH_REG = const(0x8100)

_READ_XY_REG = const(0x814E)

_POINT_1_REG = const(0x814F)
_POINT_2_REG = const(0x8157)
_POINT_3_REG = const(0x815F)
_POINT_4_REG = const(0x8167)
_POINT_5_REG = const(0x816F)

_PRODUCT_ID_REG = const(0x8140)
_FIRMWARE_VERSION_REG = const(0x8144)
_VENDOR_ID_REG = const(0x814A)

_X_CORD_RES_REG = const(0x8146)
_Y_CORD_RES_REG = const(0x8148)

_ADDR1 = const(0x5D)
_ADDR2 = const(0x14)


class GT911(pointer_framework.PointerDriver):

    def __init__(self, i2c_bus, reset_pin=None, interrupt_pin=None, touch_cal=None):
        buf = self._buf = bytearray(7)
        mv = self._mv = memoryview(self._buf)

        self._i2c = i2c.I2CDevice(i2c_bus, _ADDR1)

        if isinstance(reset_pin, int):
            reset_pin = machine.Pin(reset_pin, machine.Pin.OUT)

        if isinstance(interrupt_pin, int):
            interrupt_pin = machine.Pin(interrupt_pin, machine.Pin.OUT)

        self._reset_pin = reset_pin
        self._interrupt_pin = interrupt_pin

        # self.touch_reset()

        self._i2c.read_mem(_PRODUCT_ID_REG, buf=mv[:4])
        print('Product id:', buf[:4])

        self._i2c.read_mem(_FIRMWARE_VERSION_REG, buf=mv[:2])
        print('Firmware version:', hex(buf[0] + (buf[1] << 8)))

        self._i2c.read_mem(_VENDOR_ID_REG, buf=mv[:1])
        print('Vendor id:', hex(buf[0]))

        self._i2c.read_mem(_X_CORD_RES_REG, buf=mv[:2])
        print('Configured width:', buf[0] + (buf[1] << 8))

        self._i2c.read_mem(_Y_CORD_RES_REG, buf=mv[:2])
        print('Configured height:', buf[0] + (buf[1] << 8))

        super().__init__(touch_cal=touch_cal)

    def touch_reset(self):
        if self._interrupt_pin and self._reset_pin:
            self._interrupt_pin.init(self._interrupt_pin.OUT)
            self._interrupt_pin(0)
            self._reset_pin(0)
            time.sleep_ms(10)
            self._interrupt_pin(0)
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

        self._i2c.read_mem(_CONFIG_START_REG, buf=mv)

        buf[_X_OUTPUT_MAX_LOW_POS] = width & 0xFF
        buf[_X_OUTPUT_MAX_HIGH_POS] = (width >> 8) & 0xFF
        buf[_Y_OUTPUT_MAX_LOW_POS] = height & 0xFF
        buf[_Y_OUTPUT_MAX_HIGH_POS] = (height >> 8) & 0xFF

        checksum = ((~sum(buf)) + 1) & 0xFF

        buf[0] = width & 0xFF
        self._i2c.write_mem(_X_OUTPUT_MAX_LOW_REG, buf=mv[:1])

        buf[0] = (width >> 8) & 0xFF
        self._i2c.write_mem(_X_OUTPUT_MAX_HIGH_REG, buf=mv[:1])

        buf[0] = height & 0xFF
        self._i2c.write_mem(_Y_OUTPUT_MAX_LOW_REG, buf=mv[:1])

        buf[0] = (height >> 8) & 0xFF
        self._i2c.write_mem(_Y_OUTPUT_MAX_HIGH_REG, buf=mv[:1])

        buf[0] = checksum
        self._i2c.write_mem(_CONFIG_CHKSUM_REG, buf=mv[:1])

        buf[0] = 0x01
        self._i2c.write_mem(_CONFIG_FRESH_REG, buf=mv[:1])

    def _get_coords(self):
        buf = self._buf
        mv = self._mv

        self._i2c.read_mem(_READ_XY_REG, buf=mv[:1])

        if (buf[0] & 0x80) == 0x00:
            buf[0] = 0x00
            self._i2c.write_mem(_READ_XY_REG, buf=mv[:1])
            return None
        else:
            touch_cnt = buf[0] & 0x0F
            if touch_cnt > 5 or touch_cnt == 0:
                buf[0] = 0x00
                self._i2c.write_mem(_READ_XY_REG, buf=mv[:1])
                return None

            self._i2c.read_mem(_READ_XY_REG, buf=mv)

            x = buf[1] + (buf[2] << 8)
            y = buf[3] + (buf[4] << 8)

            buf[0] = 0x00
            self._i2c.write_mem(_READ_XY_REG, buf=mv[:1])

            return self.PRESSED, x, y
