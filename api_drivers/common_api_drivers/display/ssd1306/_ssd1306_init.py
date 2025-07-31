# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA

_SET_CONTRAST = const(0x81)
_SET_ENTIRE_ON = const(0xA4)
_SET_NORM_INV = const(0xA6)
_DISP_OFF = const(0xAE)
_DISP_ON = const(0xAF)
_SET_MEM_ADDR = const(0x20)
_SET_COL_ADDR = const(0x21)
_SET_PAGE_ADDR = const(0x22)
_SET_DISP_START_LINE = const(0x40)
_SET_SEG_REMAP = const(0xA0)
_SET_MUX_RATIO = const(0xA8)
_SET_COM_OUT_DIR = const(0xC0)
_SET_DISP_OFFSET = const(0xD3)
_SET_COM_PIN_CFG = const(0xDA)
_SET_DISP_CLK_DIV = const(0xD5)
_SET_PRECHARGE = const(0xD9)
_SET_VCOM_DESEL = const(0xDB)
_SET_CHARGE_PUMP = const(0x8D)


def init(self):
    param_buf = bytearray(1)
    param_mv = bytearray(param_buf)

    self.set_params(_DISP_OFF)

    param_buf[0] = 0x00
    self.set_params(_SET_MEM_ADDR, param_mv[:1])

    self.set_params(_SET_DISP_START_LINE | 0x00)

    self.set_params(_SET_SEG_REMAP | 0x01)

    param_buf[0] = self.display_height - 1
    self.set_params(_SET_MUX_RATIO, param_mv[:1])

    self.set_params(_SET_COM_OUT_DIR | 0x08)

    param_buf[0] = 0x00
    self.set_params(_SET_DISP_OFFSET, param_mv[:1])

    if self.width > 2 * self.display_height:
        param_buf[0] = 0x02
    else:
        param_buf[0] = 0x12
    self.set_params(_SET_COM_PIN_CFG, param_mv[:1])

    param_buf[0] = 0x80
    self.set_params(_SET_DISP_CLK_DIV, param_mv[:1])

    if self._power_pin is not None:
        param_buf[0] = 0x22
    else:
        param_buf[0] = 0xF1

    self.set_params(_SET_PRECHARGE, param_mv[:1])

    param_buf[0] = 0x30
    self.set_params(_SET_VCOM_DESEL, param_mv[:1])

    param_buf[0] = 0xFF
    self.set_params(_SET_CONTRAST, param_mv[:1])

    self.set_params(_SET_ENTIRE_ON)

    self.set_params(_SET_NORM_INV)

    if self._power_pin is not None:
        param_buf[0] = 0x10
    else:
        param_buf[0] = 0x14
    self.set_params(_SET_CHARGE_PUMP, param_mv[:1])

    self.set_params(_DISP_ON)
