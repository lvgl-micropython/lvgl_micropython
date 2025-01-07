# Copyright (c) 2024 - 2025 Kevin G. Schlosser

#  ******* NOTICE ********
# This module is no longer used.


from micropython import const  # NOQA
import machine  # NOQA

import time

try:
    import micropython  # NOQA
except ImportError:
    def ptr32(_):
        return []

    def ptr8(_):
        return []

    class micropython:
        @staticmethod
        def viper(func):
            return func


# GPIO0 ~ 31 output enable bit clear register
_GPIO_ENABLE_W1TC_REG = const(0x3FF44028)


# GPIO32 ~ 48 output enable bit clear register
_GPIO_ENABLE1_W1TC_REG = const(0x3FF44034)


class Spi3Wire:

    def __init__(self, miso, sck, cs, cs_active_high=False):
        self._miso_pin = None
        self._sck_pin = None
        self._cs_pin = None

        self._miso_pin_num = miso
        self._sck_pin_num = sck
        self._cs_pin_num = cs

        self._cs_active_high = cs_active_high

        if isinstance(cs, int):
            if cs > 31:
                self._cs_enable_reg = _GPIO_ENABLE1_W1TC_REG
                cs -= 32
            else:
                self._cs_enable_reg = _GPIO_ENABLE_W1TC_REG

            self._cs_mask = 1 << cs
        else:
            self._cs_enable_reg = None
            self._cs_mask = None

        if miso > 31:
            self._miso_enable_reg = _GPIO_ENABLE1_W1TC_REG
            miso -= 32
        else:
            self._miso_enable_reg = _GPIO_ENABLE_W1TC_REG

        self._miso_mask = 1 << miso

        if sck > 31:
            self._sck_enable_reg = _GPIO_ENABLE1_W1TC_REG
            sck -= 32
        else:
            self._sck_enable_reg = _GPIO_ENABLE_W1TC_REG

        self._sck_mask = 1 << sck

        self._cmd_bits = 8
        self._param_bits = 8

    def __del__(self):
        self.deinit()

    def init(self, cmd_bits, param_bits):
        self._cmd_bits = cmd_bits
        self._param_bits = param_bits

        self._miso_pin = machine.Pin(self._miso_pin_num, machine.Pin.OUT)
        self._sck_pin = machine.Pin(self._sck_pin_num, machine.Pin.OUT)

        if isinstance(self._cs_pin_num, int):
            self._cs_pin = machine.Pin(self._cs_pin_num, machine.Pin.OUT)
        else:
            self._cs_pin = self._cs_pin_num

    @micropython.viper
    def __deinit_sck(self):
        GPIO_ENABLE_W1TC_REG = ptr32(self._sck_enable_reg)
        sck_mask = int(ptr32(self._sck_mask))

        GPIO_ENABLE_W1TC_REG[0] |= sck_mask

    @micropython.viper
    def __deinit_miso(self):
        GPIO_ENABLE_W1TC_REG = ptr32(self._miso_enable_reg)
        miso_mask = int(ptr32(self._miso_mask))

        GPIO_ENABLE_W1TC_REG[0] |= miso_mask

    @micropython.viper
    def __deinit_cs(self):
        GPIO_ENABLE_W1TC_REG = ptr32(self._cs_enable_reg)
        cs_mask = int(ptr32(self._cs_mask))

        GPIO_ENABLE_W1TC_REG[0] |= cs_mask

    def deinit(self):
        if self._sck_pin is not None:
            del self._sck_pin
            self._sck_pin = None
            self.__deinit_sck()

        if self._miso_pin is not None:
            del self._miso_pin
            self._miso_pin = None
            self.__deinit_miso()

        if isinstance(self._cs_pin, machine.Pin):
            del self._cs_pin
            self._cs_pin = None
            self.__deinit_cs()
        else:
            del self._cs_pin
            self._cs_pin = None

    def __senddata(self, data, num_bits):

        for i in range(num_bits):
            if data & 0x80:
                self._miso_pin(1)
            else:
                self._miso_pin(0)

            data = data << 1
            self._sck_pin(0)
            time.sleep_us(1)  # NOQA
            self._sck_pin(1)
            time.sleep_us(1)  # NOQA

    def tx_param(self, cmd, params=None):
        if self._cs_pin is None:
            return

        self.__write_cmd(cmd)

        if params is None:
            return

        for param in params:
            self.__write_param(param)

    def __write_cmd(self, cmd):  # 9bit
        self._cs_pin(self._cs_active_high)
        self._miso_pin(0)
        self._sck_pin(0)
        time.sleep_us(1)  # NOQA
        self._sck_pin(1)
        time.sleep_us(1)  # NOQA

        self.__senddata(cmd, self._cmd_bits)
        self._cs_pin(not self._cs_active_high)

    def __write_param(self, param: int):
        self._cs_pin(self._cs_active_high)
        self._miso_pin(1)
        self._sck_pin(0)
        time.sleep_us(1)  # NOQA
        self._sck_pin(1)
        time.sleep_us(1)  # NOQA

        self.__senddata(param, self._param_bits)
        self._cs_pin(not self._cs_active_high)
