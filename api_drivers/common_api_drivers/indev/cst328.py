# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import pointer_framework
import time
import machine  # NOQA


I2C_ADDR = 0x1A
BITS = 16

# 4 byte read
# PROJECT_ID = byte2 << 8 | byte3
# CHIP_TYPE = byte0 << 8 | byte1
_ID_REG = const(0xD204)

# BYTE 3:FW_MAJOR
# BYTE 2:FW_MINOR
# BYTE 1 ~ BYTE 0:FW_BUILD
# 4 byte read
# MAJOR = byte0
# MINOR = byte1
# MICRO = byte2 << 8 | byte3
_FW_VERSION_REG = const(0xD208)


_MODE_NORMAL_REG = const(0xD109)


# touch information register
# MODE_NORMAL
# BYTE0:
# BITS 0 - 3: Touch state (0x06) == touched
# BITS 4 - 7: N/A

# BYTE1, BYTE2, BYTE3
# BYTE1: BIT 7 ~ BIT 0: X high 8 bits
# BYTE2: BIT 7 ~ BIT 0: Y high 8 bits
# BYTE3: BITS 0 - 3: Y low 4 bits, BITS 4 - 7 X low 4 bits
# x = BYTE1 << 4 | BYTE3 >> 4
# y = BYTE2 << 4 | BYTE3 & 0x0F

# BYTE4:
# BITS 0 - 7: Touch pressure

# BYTE5:
# BITS 0 - 3: Finger count
# BITS 4 - 7: N/A
_TOUCH_DATA_REG = const(0xD000)
_TOUCH_STATE_MASK = const(0x0F)
_TOUCH_STATE_PRESSED = const(0x06)
_TOUCH_COUNT_MASK = const(0x0F)


class CST328(pointer_framework.PointerDriver):

    def _read_reg(self, reg, num_bytes):
        self._tx_buf[0] = reg >> 8
        self._tx_buf[1] = reg & 0xFF

        self._rx_buf[:num_bytes] = bytearray([0x00] * num_bytes)

        self._device.write_readinto(self._tx_mv[:2], self._rx_mv[:num_bytes])

    def _write_reg(self, reg, value=None):
        self._tx_buf[0] = reg >> 8
        self._tx_buf[1] = reg & 0xFF

        if value is None:
            self._device.write(self._tx_mv[:2])
        else:
            self._tx_buf[2] = value
            self._device.write(self._tx_mv[:3])

    def __init__(
        self,
        device,
        reset_pin=None,
        touch_cal=None,
        startup_rotation=pointer_framework.lv.DISPLAY_ROTATION._0,  # NOQA
        debug=False
    ):
        self._tx_buf = bytearray(3)
        self._tx_mv = memoryview(self._tx_buf)
        self._rx_buf = bytearray(6)
        self._rx_mv = memoryview(self._rx_buf)

        self._device = device

        if not isinstance(reset_pin, int):
            self._reset_pin = reset_pin
        else:
            self._reset_pin = machine.Pin(reset_pin, machine.Pin.OUT)

        if self._reset_pin:
            self._reset_pin.value(1)

        self.hw_reset()

        self._read_reg(_ID_REG, 4)
        chip_id = (self._rx_buf[0] << 8) | self._rx_buf[1]
        project_id = (self._rx_buf[2] << 8) | self._rx_buf[3]

        print(f'Chip ID: 0x{hex(chip_id)[2:].upper()}')
        print(f'Proj ID: 0x{hex(project_id)[2:].upper()}')

        self._read_reg(_FW_VERSION_REG, 4)
        version = f'{self._rx_buf[0]}.{self._rx_buf[1]}.{(self._rx_buf[2] << 8) | self._rx_buf[3]}'
        print('FW Version:', version)

        self._write_reg(_MODE_NORMAL_REG)

        self._touch_thresh = 10

        super().__init__(
            touch_cal=touch_cal, startup_rotation=startup_rotation, debug=debug
        )

    @property
    def touch_threshold(self):
        return self._touch_thresh

    @touch_threshold.setter
    def touch_threshold(self, value):
        if value < 1:
            value = 1
        elif value > 255:
            value = 255
        self._touch_thresh = value

    def hw_reset(self):
        if self._reset_pin is None:
            return

        self._reset_pin(0)
        time.sleep_ms(1)  # NOQA
        self._reset_pin(1)
        time.sleep_ms(50)  # NOQA

    def _get_coords(self):
        self._read_reg(_TOUCH_DATA_REG, 6)

        touch_count = self._rx_buf[5] & _TOUCH_COUNT_MASK
        if touch_count:
            if self._rx_buf[0] & _TOUCH_STATE_MASK == _TOUCH_STATE_PRESSED:
                state = self.PRESSED

                if self._rx_buf[4] < self._touch_thresh:
                    return None
            else:
                state = self.RELEASED

            x = (self._rx_buf[1] << 4) | (self._rx_buf[3] >> 4)
            y = (self._rx_buf[2] << 4) | (self._rx_buf[3] & 0x0F)

            return state, x, y
