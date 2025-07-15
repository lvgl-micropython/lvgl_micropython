# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import pointer_framework
import machine  # NOQA
import time


_POINTS_REG = const(0X01)

I2C_ADDR = 0x63
BITS = 8


class AXS5106(pointer_framework.PointerDriver):

    def _read_reg(self, reg, num_bytes):
        self._tx_buf[0] = reg
        self._rx_mv[:num_bytes] = bytearray([0x00] * num_bytes)
        self._device.write(self._tx_mv[:1])
        self._device.read(buf=self._rx_mv[:num_bytes])

    def __init__(
        self,
        device,
        reset_pin=None,
        touch_cal=None,
        startup_rotation=pointer_framework.lv.DISPLAY_ROTATION._0,  # NOQA
        debug=False
    ):
        self._tx_buf = bytearray(1)
        self._tx_mv = memoryview(self._tx_buf)
        self._rx_buf = bytearray(14)
        self._rx_mv = memoryview(self._rx_buf)

        self._device = device

        self.__x = 0
        self.__y = 0
        self.__last_state = self.RELEASED

        if isinstance(reset_pin, int):
            reset_pin = machine.Pin(reset_pin, machine.Pin.OUT)

        self._reset_pin = reset_pin
        self.hw_reset()
        super().__init__(
            touch_cal=touch_cal, startup_rotation=startup_rotation, debug=debug
        )

    def hw_reset(self):
        if self._reset_pin:
            self._reset_pin.value(1)
            time.sleep_ms(10)  # NOQA
            self._reset_pin.value(0)
            time.sleep_ms(10)  # NOQA
            self._reset_pin.value(1)
            time.sleep_ms(10)  # NOQA

    def _get_coords(self):
        self._read_reg(_POINTS_REG, 14)
        touch_cnt = self._rx_buf[1] & 0x0F

        if touch_cnt:
            x = ((self._rx_buf[2] & 0x0F) << 8) | self._rx_buf[3]
            y = ((self._rx_buf[4] & 0x0F) << 8) | self._rx_buf[5]

            self.__x = x
            self.__y = y
            self.__last_state = self.PRESSED

        elif touch_cnt == 0:
            self.__last_state = self.RELEASED

        return self.__last_state, self.__x, self.__y
