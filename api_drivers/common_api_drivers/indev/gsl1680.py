# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import pointer_framework
import machine  # NOQA
import time

_DATA_REG = const(0x80)
_STATUS_REG = const(0xE0)
_PAGE_REG = const(0xF0)

I2C_ADDR = 0x40
BITS = 8


class GSL1680(pointer_framework.PointerDriver):

    def _read_reg(self, reg, num_bytes=None, buf=None):
        self._tx_buf[0] = reg >> 8
        self._tx_buf[1] = reg & 0xFF
        if num_bytes is not None:
            self._device.write_readinto(self._tx_mv[:2], self._rx_mv[:num_bytes])
        else:
            self._device.write_readinto(self._tx_mv[:2], buf)

    def _write_reg(self, reg, value=None, buf=None):
        if value is not None:
            self._tx_buf[0] = value
            self._device.write_mem(reg, self._tx_mv[:1])
        elif buf is not None:
            self._device.write_mem(reg, buf)

    def __init__(
        self,
        device,
        wake_pin,
        touch_cal=None,
        startup_rotation=pointer_framework.lv.DISPLAY_ROTATION._0,  # NOQA
        debug=False
    ):
        self._tx_buf = bytearray(4)
        self._tx_mv = memoryview(self._tx_buf)
        self._rx_buf = bytearray(24)
        self._rx_mv = memoryview(self._rx_buf)

        self._device = device

        if isinstance(wake_pin, int):
            wake_pin = machine.Pin(wake_pin, machine.Pin.OUT)

        self._wake_pin = wake_pin

        self._wake_up()
        time.sleep_ms(50)  # NOQA
        self._write_reg(_STATUS_REG, 0x88)
        time.sleep_ms(10)  # NOQA
        self._write_reg(0xE4, 0x04)
        time.sleep_ms(5)  # NOQA
        self._tx_buf[:4] = bytearray([0x00, 0x00, 0x00, 0x00])
        self._write_reg(0xBC, buf=self._tx_mv[:4])
        time.sleep_ms(5)  # NOQA
        self._write_reg(0xE0, 0x00)

        super().__init__(
            touch_cal=touch_cal, startup_rotation=startup_rotation, debug=debug
        )

    def _wake_up(self):
        # toggle wake pin
        self._wake_pin.value(1)
        time.sleep_ms(1)  # NOQA
        self._wake_pin.value(0)
        time.sleep_ms(1)  # NOQA
        self._wake_pin.value(1)
        time.sleep_ms(1)  # NOQA

    def _get_coords(self):
        self._wake_up()
        self._read_reg(_DATA_REG, buf=self._rx_mv)
        data = self._rx_buf

        touch_count = data[0]
        if touch_count > 0:
            x = ((data[5] << 8) & 0xF) | data[4]
            y = ((data[7] << 8) & 0xF) | data[6]

            return self.RELEASED, x, y
