# Copyright (c) 2025 0ut4t1m3
# Copyright (c) 2024 - 2025 Kevin G. Schlosser


from micropython import const  # NOQA
import lvgl as lv
import time


_SLPOUT = const(0x11)
_SWRESET = const(0x01)
_INVON = const(0x21)
_INVOFF = const(0x20)

_DISPOFF = const(0x28)
_DISPON = const(0x29)


_CASET = const(0x2A)
_RASET = const(0x2B)

_RAMWR = const(0x2C)
_TEOFF = const(0x34)
_MADCTL = const(0x36)

# Set Color Enhance
_IMGEHCCTR = const(0x58)

# Set Color Enhance 1
_CESLRCTR_L = const(0x5A)
_CESLRCTR_H = const(0x5B)

_COLMOD = const(0x3A)
_RGB565 = const(0x75)  # 16bit/pixel (65,536 colors)
_RGB888 = const(0x77)  # 24bit/pixel (16.7M colors)

# Write Display Brightness
# 0 to 255
_WRDISBV = const(0x51)

# Write Display Control
_WRCTRLD = const(0x53)
_BCTRL = const(0x20)
_DD = const(0x08)

# Set Dual SPI Mode
_SetDSPIMode = const(0xC4)
_DSPI_EN = const(0x21)

# CMD Mode Switch
# 0x00 == user mode
_CMDMode = const(0xFE)

# High Brightness Mode
# 0x00 disable
# 0x02 enable
_SETHBM = const(0xB0)


def init(self):
    self._br_val = 0

    buf = self._param_buf
    mv = self._param_mv

    time.sleep_ms(120)  # NOQA
    self.set_params(_SWRESET)
    time.sleep_ms(120)  # NOQA

    num_lanes = self._data_bus.get_lane_count()
    if num_lanes == 2:
        buf[0] = _DSPI_EN
        self.set_params(_SetDSPIMode, mv[:1])
    elif num_lanes not in (1, 8):
        raise RuntimeError('display only supports I8080 8 lane, SPI and DUAL SPI')

    self.set_params(_SLPOUT)
    time.sleep_ms(120)  # NOQA

    buf[0] = 0x05
    self.set_params(_CMDMode, mv[:1])

    self.set_params(0x05, mv[:1])

    buf[0] = 0x01
    self.set_params(_CMDMode, mv[:1])

    buf[0] = 0x25
    self.set_params(0x73, mv[:1])

    buf[0] = 0x00
    self.set_params(_CMDMode, mv[:1])

    self.set_params(_TEOFF)

    buf[0] = (
        self._madctl(
            self._color_byte_order,
            self._ORIENTATION_TABLE  # NOQA
        )
    )
    self.set_params(_MADCTL, mv[:1])

    color_size = lv.color_format_get_size(self._color_space)
    if color_size == 2:  # NOQA
        buf[0] = _RGB565
    elif color_size == 3:
        buf[0] = _RGB888
    else:
        raise RuntimeError(
            f'{self.__class__.__name__} IC only supports '
            'RGB565 and RGB888'
        )

    self.set_params(_COLMOD, mv[:1])

    buf[0] = _BCTRL | _DD
    self.set_params(_WRCTRLD, mv[:1])

    buf[0] = 0x7F
    self.set_params(_CESLRCTR_H, mv[:1])

    buf[0] = 0xFF
    self.set_params(_CESLRCTR_L, mv[:1])

    buf[0] = 0x00
    self.set_params(_IMGEHCCTR, mv[:1])
    self.set_params(_WRDISBV, mv[:1])
    self.set_params(_SETHBM, mv[:1])
    self.set_params(_DISPON)
