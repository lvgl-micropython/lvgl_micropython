# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA

import i2c
import pointer_framework
import time


I2C_ADDR = 0x41
BITS = 8

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

_FIFO_STA_REG = const(0x4B)
_FIFO_STA_RESET = const(0x01)

_TSC_I_DRIVE_50MA = const(0x01)


class STMPE610(pointer_framework.PointerDriver):

    def _read_reg(self, reg, num_bytes):
        self._tx_buf[0] = reg

        self._device.write_readinto(
            self._tx_mv[:num_bytes],
            self._rx_mv[:num_bytes]
        )

    def _write_reg(self, reg, value):
        self._tx_buf[0] = reg
        self._tx_buf[1] = value
        self._device.write(self._tx_mv[:2])

    def __init__(
        self,
        device,
        touch_cal=None,
        startup_rotation=pointer_framework.lv.DISPLAY_ROTATION._0,  # NOQA
        debug=False
    ):
        self._device = device

        self._tx_buf = bytearray(4)
        self._tx_mv = memoryview(self._tx_buf)

        self._rx_buf = bytearray(4)
        self._rx_mv = memoryview(self._rx_buf)

        if isinstance(device, i2c.I2C.Device):
            _TSC_FRACTION_Z_REG = 0x56
            _TSC_I_DRIVE_REG = 0x58
            self._FIFO_SIZE_REG = 0x4C
        else:
            _TSC_FRACTION_Z_REG = 0x58
            _TSC_I_DRIVE_REG = 0x56
            self._FIFO_SIZE_REG = 0xCC

        self._write_reg(_SYS_CTRL1_REG, _SYS_CTRL1_RESET)
        time.sleep(0.001)

        self._write_reg(_SYS_CTRL2_REG, 0x00)
        self._write_reg(_TSC_CTRL_REG, _TSC_CTRL_XYZ | _TSC_CTRL_EN)
        self._write_reg(_INT_EN_REG, _INT_EN_TOUCHDET)
        self._write_reg(_ADC_CTRL1_REG, _ADC_CTRL1_10BIT | (0x6 << 4))
        self._write_reg(_ADC_CTRL2_REG, _ADC_CTRL2_6_5MHZ)
        self._write_reg(
            _TSC_CFG_REG,
            _TSC_CFG_4SAMPLE | _TSC_CFG_DELAY_1MS | _TSC_CFG_SETTLE_5MS
        )
        self._write_reg(_TSC_FRACTION_Z_REG, 0x06)
        self._write_reg(_FIFO_TH_REG, 0x01)
        self._write_reg(_FIFO_STA_REG, _FIFO_STA_RESET)
        self._write_reg(_FIFO_STA_REG, 0x00)
        self._write_reg(_TSC_I_DRIVE_REG, _TSC_I_DRIVE_50MA)
        self._write_reg(_INT_STA_REG, 0xFF)
        self._write_reg(_INT_CTRL_REG, _INT_CTRL_DISABLE)

        super().__init__(
            touch_cal=touch_cal, startup_rotation=startup_rotation, debug=debug
        )

    def _get_coords(self):
        self._read_reg(self._FIFO_SIZE_REG, 1)
        touch_count = self._rx_buf[0]

        if not touch_count:
            return None

        while touch_count:
            self._read_reg(0xD7, 4)
            touch_count -= 1

        x = self._rx_buf[0] << 4 | self._rx_buf[1] >> 4
        y = (self._rx_buf[1] & 0xF) << 8 | self._rx_buf[2]

        self._write_reg(_INT_STA_REG, 0xFF)

        return self.PRESSED, x, y
