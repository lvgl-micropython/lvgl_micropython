from micropython import const  # NOQA
import machine  # NOQA

import time
import struct

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


# GPIO0 ~ 31 output register
_GPIO_OUT_REG = const(0x3FF44004)

# GPIO0 ~ 31 output bit set register
_GPIO_OUT_W1TS_REG = const(0x3FF44008)

# GPIO0 ~ 31 output bit clear register
_GPIO_OUT_W1TC_REG = const(0x3FF4400C)

# GPIO0 ~ 31 output enable register
_GPIO_ENABLE_REG = const(0x3FF44020)

# GPIO0 ~ 31 output enable bit set register
_GPIO_ENABLE_W1TS_REG = const(0x3FF44024)

# GPIO0 ~ 31 output enable bit clear register
_GPIO_ENABLE_W1TC_REG = const(0x3FF44028)


# GPIO32 ~ 48 output register
_GPIO_OUT1_REG = const(0x3FF44010)

# GGPIO32 ~ 48 output bit set registe
_GPIO_OUT1_W1TS_REG = const(0x3FF44014)

# GPIO32 ~ 48 output bit clear register
_GPIO_OUT1_W1TC_REG = const(0x3FF44018)


# GPIO32 ~ 48 output enable register
_GPIO_ENABLE1_REG = const(0x3FF4402C)

# GPIO32 ~ 48 output enable bit set register
_GPIO_ENABLE1_W1TS_REG = const(0x3FF44030)

# GPIO32 ~ 48 output enable bit clear register
_GPIO_ENABLE1_W1TC_REG = const(0x3FF44034)


# GPIO0 ~ 31 interrupt status bit clear register
_GPIO_STATUS_W1TC_REG = const(0x3FF4404C)


# GPIO32 ~ 48 interrupt status bit clear register
_GPIO_STATUS1_W1TC_REG = const(0x3FF44058)


# 0x3FF44074 + (0x4 * pin_num)
# GPIO_PINn_PAD_DRIVER
# GPIO_PINn_INT_TYPE


class LOW_SPEED_SPI:

    def __init__(self, miso, sck, cs, cs_active_high=False):
        self._miso_pin = None
        self._sck_pin = None
        self._cs_pin = None

        self._miso_pin_num = miso
        self._sck_pin_num = sck
        self._cs_pin_num = cs

        if cs > 31:
            if cs_active_high:
                self._cs_set_reg = _GPIO_OUT1_W1TC_REG
                self._cs_clear_reg = _GPIO_OUT1_W1TS_REG
            else:
                self._cs_set_reg = _GPIO_OUT1_W1TS_REG
                self._cs_clear_reg = _GPIO_OUT1_W1TC_REG

            self._cs_enable_reg = _GPIO_ENABLE1_W1TC_REG
            cs -= 32
        else:
            if cs_active_high:
                self._cs_set_reg = _GPIO_OUT_W1TC_REG
                self._cs_clear_reg = _GPIO_OUT_W1TS_REG
            else:
                self._cs_set_reg = _GPIO_OUT_W1TS_REG
                self._cs_clear_reg = _GPIO_OUT_W1TC_REG

            self._cs_enable_reg = _GPIO_ENABLE_W1TC_REG

        self._cs_mask = 1 << cs

        if miso > 31:
            self._miso_set_reg = _GPIO_OUT1_W1TS_REG
            self._miso_clear_reg = _GPIO_OUT1_W1TC_REG
            self._miso_enable_reg = _GPIO_ENABLE1_W1TC_REG
            miso -= 32
        else:
            self._miso_set_reg = _GPIO_OUT_W1TS_REG
            self._miso_clear_reg = _GPIO_OUT_W1TC_REG
            self._miso_enable_reg = _GPIO_ENABLE_W1TC_REG

        self._miso_mask = 1 << miso

        if sck > 31:
            self._sck_set_reg = _GPIO_OUT1_W1TS_REG
            self._sck_clear_reg = _GPIO_OUT1_W1TC_REG
            self._sck_enable_reg = _GPIO_ENABLE1_W1TC_REG
            sck -= 32
        else:
            self._sck_set_reg = _GPIO_OUT_W1TS_REG
            self._sck_clear_reg = _GPIO_OUT_W1TC_REG
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
        self._cs_pin = machine.Pin(self._cs_pin_num, machine.Pin.OUT)

    @micropython.viper
    def __deinit_sck(self):
        GPIO_ENABLE_W1TC_REG = ptr32(self._sck_enable_reg)
        GPIO_ENABLE_W1TC_REG[0] |= int(ptr32(self._sck_mask))

    @micropython.viper
    def __deinit_miso(self):
        GPIO_ENABLE_W1TC_REG = ptr32(self._miso_enable_reg)
        GPIO_ENABLE_W1TC_REG[0] |= int(ptr32(self._miso_mask))

    @micropython.viper
    def __deinit_cs(self):
        GPIO_ENABLE_W1TC_REG = ptr32(self._cs_enable_reg)
        GPIO_ENABLE_W1TC_REG[0] |= int(ptr32(self._cs_mask))

    def deinit(self):
        if self._sck_pin is not None:
            del self._sck_pin
            self._sck_pin = None
            self.__deinit_sck()

        if self._miso_pin is not None:
            del self._miso_pin
            self._miso_pin = None
            self.__deinit_miso()

        if self._cs_pin is not None:
            del self._cs_pin
            self._cs_pin = None
            self.__deinit_cs()

    def delay(self, t):  # NOQA
        time.sleep_us(t)  # NOQA

    @micropython.viper
    def __senddata(self, data: int, num_bits: int):
        dta = int(data)
        miso_set = ptr32(self._miso_set_reg)
        miso_clear = ptr32(self._miso_clear_reg)
        miso_mask = int(ptr32(self._miso_mask))

        sck_set = ptr32(self._sck_set_reg)
        sck_clear = ptr32(self._sck_clear_reg)
        sck_mask = int(ptr32(self._sck_mask))

        for i in range(0, num_bits):
            if dta & 0x80:
                miso_set[0] |= miso_mask
            else:
                miso_clear[0] |= miso_mask

            dta = dta << 1
            sck_clear[0] |= sck_mask
            time.sleep_us(1)  # NOQA
            sck_set[0] |= sck_mask
            time.sleep_us(1)  # NOQA

    def tx_param(self, cmd, params):
        if self._cs_pin is None:
            return

        self.__cs_active()
        self.__write_cmd(cmd)
        self.__cs_inactive()

        self.__cs_active()
        for param in params:
            self.__write_param(param)
        self.__cs_inactive()

    @micropython.viper
    def __cs_inactive(self):
        cs_set = ptr32(self._sck_enable_reg)
        cs_mask = int(ptr32(self._cs_mask))

        cs_set[0] |= cs_mask

    @micropython.viper
    def __cs_active(self):
        cs_clear = ptr32(self._sck_enable_reg)
        cs_mask = int(ptr32(self._cs_mask))

        cs_clear[0] |= cs_mask

    @micropython.viper
    def __write_cmd(self, cmd: int):  # 9bit
        miso_clear = ptr32(self._miso_clear_reg)
        miso_mask = int(ptr32(self._miso_mask))
        sck_set = ptr32(self._sck_set_reg)
        sck_clear = ptr32(self._sck_clear_reg)
        sck_mask = int(ptr32(self._sck_mask))
        cmd_bits = int(ptr8(self._cmd_bits))

        miso_clear[0] |= miso_mask
        sck_clear[0] |= sck_mask
        time.sleep_us(1)  # NOQA
        sck_set[0] |= sck_mask
        time.sleep_us(1)  # NOQA

        self.__senddata(cmd, cmd_bits)

    @micropython.viper
    def __write_param(self, param: int):
        miso_set = ptr32(self._miso_set_reg)
        miso_mask = int(ptr32(self._miso_mask))
        sck_set = ptr32(self._sck_set_reg)
        sck_clear = ptr32(self._sck_clear_reg)
        sck_mask = int(ptr32(self._sck_mask))
        param_bits = int(ptr8(self._param_bits))

        miso_set[0] |= miso_mask
        sck_clear[0] |= sck_mask
        time.sleep_us(1)  # NOQA
        sck_set[0] |= sck_mask
        time.sleep_us(1)  # NOQA

        self.__senddata(param, param_bits)
