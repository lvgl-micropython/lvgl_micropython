from micropython import const  # NOQA
import pointer_framework
import machine  # NOQA
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
_FIFO_SIZE_REG = const(0xCC)

_FIFO_STA_REG = const(0x4B)
_FIFO_STA_RESET = const(0x01)

_TSC_I_DRIVE_REG = const(0x56)
_TSC_I_DRIVE_50MA = const(0x01)

_TSC_FRACTION_Z_REG = const(0x58)


class STMPE610(pointer_framework.PointerDriver):

    def __init__(
        self,
        miso,
        mosi,
        clk,
        cs,
        host,
        freq=5000000,
        touch_cal=None
    ):

        self._spi = machine.SPI(
            host + 1,
            baudrate=freq,
            sck=machine.Pin(clk, machine.Pin.OUT),
            mosi=machine.Pin(mosi, machine.Pin.OUT),
            miso=machine.Pin(miso, machine.Pin.IN, pull=machine.Pin.PULL_UP)
        )

        self.cs = machine.Pin(cs, machine.Pin.OUT)
        self.cs.value(1)

        self._buf = bytearray(4)
        self._mv = memoryview(self._buf)

        self._buf[0] = _SYS_CTRL1_REG
        self._buf[1] = _SYS_CTRL1_RESET
        self._spi.write(self._mv[:2])

        time.sleep(0.001)

        self._buf[0] = _SYS_CTRL2_REG
        self._buf[1] = 0x00
        self._spi.write(self._mv[:2])

        self._buf[0] = _TSC_CTRL_REG
        self._buf[1] = _TSC_CTRL_XYZ | _TSC_CTRL_EN
        self._spi.write(self._mv[:2])

        self._buf[0] = _INT_EN_REG
        self._buf[1] = _INT_EN_TOUCHDET
        self._spi.write(self._mv[:2])

        self._buf[0] = _ADC_CTRL1_REG
        self._buf[1] = _ADC_CTRL1_10BIT | (0x06 << 4)
        self._spi.write(self._mv[:2])

        self._buf[0] = _ADC_CTRL2_REG
        self._buf[1] = _ADC_CTRL2_6_5MHZ
        self._spi.write(self._mv[:2])

        self._buf[0] = _TSC_CFG_REG
        self._buf[1] = _TSC_CFG_4SAMPLE | _TSC_CFG_DELAY_1MS | _TSC_CFG_SETTLE_5MS
        self._spi.write(self._mv[:2])

        self._buf[0] = _TSC_FRACTION_Z_REG
        self._buf[1] = 0x06
        self._spi.write(self._mv[:2])

        self._buf[0] = _FIFO_TH_REG
        self._buf[1] = 0x01
        self._spi.write(self._mv[:2])

        self._buf[0] = _FIFO_STA_REG
        self._buf[1] = _FIFO_STA_RESET
        self._spi.write(self._mv[:2])

        self._buf[0] = _FIFO_STA_REG
        self._buf[1] = 0x00
        self._spi.write(self._mv[:2])

        self._buf[0] = _TSC_I_DRIVE_REG
        self._buf[1] = _TSC_I_DRIVE_50MA
        self._spi.write(self._mv[:2])

        self._buf[0] = _INT_STA_REG
        self._buf[1] = 0xFF
        self._spi.write(self._mv[:2])

        self._buf[0] = _INT_CTRL_REG
        self._buf[1] = _INT_CTRL_DISABLE
        self._spi.write(self._mv[:2])

        super().__init__(touch_cal=touch_cal)

    def _get_coords(self):
        self._buf[0] = _FIFO_SIZE_REG
        self._spi.write_readinto(self._mv[:1], self._mv[:1])
        touch_count = self._buf[0]

        if not touch_count:
            return None

        while touch_count:
            self._buf[0] = 0xD7
            self._spi.write_readinto(self._mv[:1], self._mv)
            touch_count -= 1

        x = self._buf[0] << 4 | self._buf[1] >> 4
        y = (self._buf[1] & 0xF) << 8 | self._buf[2]

        self._buf[0] = _INT_STA_REG
        self._buf[1] = 0xFF
        self._spi.write(self._mv[:2])

        return self.PRESSED, x, y
