# this driver uses a special i2c bus implimentation I have written.
# This implimentation takes into consideration the ESP32 and it having
# threading available. It also has some convience methods built into it
# that figure out what is wanting to be done automatically.
# read more about it's use in the stub files.

from micropython import const
import pointer_framework
import machine
import time


_CMD_REG = const(0x8040)
_CMD_CHECK_REG = const(0x8046)
_CMD_READ_DATA = const(0x01)

_ESD_CHECK_REG = const(0x8041)

_STATUS_REG = const(0x814E)
_POINT_1_REG = const(0x8150)

_PRODUCT_ID_REG = const(0x8140)
_FIRMWARE_VERSION_REG = const(0x8144)
_VENDOR_ID_REG = const(0x814A)

_X_CORD_RES_REG = const(0x8146)
_Y_CORD_RES_REG = const(0x8148)

_ADDR1 = const(0x5D)
_ADDR2 = const(0x14)


class GT911(pointer_framework.PointerDriver):

    def __init__(self, i2c_bus, reset_pin=None, interrupt_pin=None, touch_cal=None):
        self._buf = bytearray(6)
        self._mv = memoryview(self._buf)
        self._i2c_bus = i2c_bus
        self._i2c = i2c_bus.add_device(_ADDR1, 16)

        self.__x = 0
        self.__y = 0
        self.__last_state = self.RELEASED

        if isinstance(reset_pin, int):
            reset_pin = machine.Pin(reset_pin, machine.Pin.OUT)

        if isinstance(interrupt_pin, int):
            interrupt_pin = machine.Pin(interrupt_pin, machine.Pin.OUT)

        self._reset_pin = reset_pin
        self._interrupt_pin = interrupt_pin

        self.hw_reset()
        super().__init__(touch_cal=touch_cal)

    def hw_reset(self):
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

        self._buf[0] = 0x00
        self._i2c.write_mem(_ESD_CHECK_REG, buf=self._mv[:1])

        self._buf[0] = _CMD_READ_DATA
        self._i2c.write_mem(_CMD_CHECK_REG, buf=self._mv[:1])
        self._i2c.write_mem(_CMD_REG, buf=self._mv[:1])

        self._i2c.read_mem(_PRODUCT_ID_REG, buf=self._mv[:4])
        print('Touch Product id:', self._buf[:4])

        self._i2c.read_mem(_FIRMWARE_VERSION_REG, buf=self._mv[:2])
        print(
            'Touch Firmware version:',
            hex(self._buf[0] + (self._buf[1] << 8))
            )

        self._i2c.read_mem(_VENDOR_ID_REG, buf=self._mv[:1])
        print(f'Touch Vendor id: 0x{hex(self._buf[0])[2:].upper()}')
        x, y = self.hw_size
        print(f'Touch resolution: width={x}, height={y}')

    @property
    def hw_size(self):
        self._i2c.read_mem(_X_CORD_RES_REG, buf=self._mv[:2])
        x = self._buf[0] + (self._buf[1] << 8)

        self._i2c.read_mem(_Y_CORD_RES_REG, buf=self._mv[:2])
        y = self._buf[0] + (self._buf[1] << 8)

        return x, y

    @property
    def firmware_config(self):
        try:
            import gt911_extension
        except ImportError:
            raise ImportError(
                'you need to upload the gt911_extension.py file to the MCU'
            )
        return gt911_extension.GT911Extension(self, self._i2c)

    def _get_coords(self):
        self._i2c.read_mem(_STATUS_REG, buf=self._mv[:1])
        touch_cnt = self._buf[0] & 0x0F
        status = self._buf[0] & 0x80

        if status and touch_cnt == 1:
            self._i2c.read_mem(_POINT_1_REG, buf=self._mv)

            x = self._buf[0] + (self._buf[1] << 8)
            y = self._buf[2] + (self._buf[3] << 8)

            self._buf[0] = 0x00
            self._i2c.write_mem(_STATUS_REG, buf=self._mv[:1])

            self.__x = x
            self.__y = y
            self.__last_state = self.PRESSED

            self._buf[0] = 0x00
            self._i2c.write_mem(_STATUS_REG, buf=self._mv[:1])

            return self.PRESSED, x, y

        elif status and touch_cnt > 1 and self.__last_state == self.PRESSED:
            self._buf[0] = 0x00
            self._i2c.write_mem(_STATUS_REG, buf=self._mv[:1])

            return self.PRESSED, self.__x, self.__y

        self.__last_state = self.RELEASED

        self._buf[0] = 0x00
        self._i2c.write_mem(_STATUS_REG, buf=self._mv[:1])


