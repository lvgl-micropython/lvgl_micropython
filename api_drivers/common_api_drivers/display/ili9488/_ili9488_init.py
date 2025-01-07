# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import time
import lvgl as lv  # NOQA
import lcd_bus  # NOQA


_IFMODE = const(0xB0)
_FRMCTR1 = const(0xB1)
_DIC = const(0xB4)
_DFC = const(0xB6)
_EM = const(0xB7)
_PWR1 = const(0xC0)
_PWR2 = const(0xC1)
_VCMPCTL = const(0xC5)
_PGC = const(0xE0)
_NGC = const(0xE1)
_AC3 = const(0xF7)
_MADCTL = const(0x36)
_COLMOD = const(0x3A)
_NOP = const(0x00)
_SLPOUT = const(0x11)
_DISPON = const(0x29)
_SWRESET = const(0x01)


def init(self):
    param_buf = bytearray(15)
    param_mv = memoryview(param_buf)

    time.sleep_ms(120)  # NOQA

    self.set_params(_SWRESET)
    time.sleep_ms(200)  # NOQA

    self.set_params(_SLPOUT)
    time.sleep_ms(120)  # NOQA

    param_buf[:15] = bytearray(
        [
            0x00, 0x03, 0x09, 0x08, 0x16,
            0x0A, 0x3F, 0x78, 0x4C, 0x09,
            0x0A, 0x08, 0x16, 0x1A, 0x0F
        ]
    )
    self.set_params(_PGC, param_mv[:15])

    param_buf[:15] = bytearray(
        [
            0x00, 0x16, 0x19, 0x03, 0x0F,
            0x05, 0x32, 0x45, 0x46, 0x04,
            0x0E, 0x0D, 0x35, 0x37, 0x0F
        ]
    )
    self.set_params(_NGC, param_mv[:15])

    param_buf[0] = 0x17
    param_buf[1] = 0x15
    self.set_params(_PWR1, param_mv[:2])

    param_buf[0] = 0x41
    self.set_params(_PWR2, param_mv[:1])

    param_buf[0] = 0x00
    param_buf[1] = 0x12
    param_buf[3] = 0x80
    self.set_params(_VCMPCTL, param_mv[:3])

    param_buf[0] = (
        self._madctl(
            self._color_byte_order,
            self._ORIENTATION_TABLE
        )
    )
    self.set_params(_MADCTL, param_mv[:1])

    color_size = lv.color_format_get_size(self._color_space)
    if color_size == 2:  # NOQA
        if isinstance(self._data_bus, lcd_bus.SPIBus):
            raise RuntimeError(
                'ILI9488 IC only supports '
                'lv.COLOR_FORMAT.RGB888 when using the SPIBus'
            )
        pixel_format = 0x55
    elif color_size == 3:
        if isinstance(self._data_bus, lcd_bus.SPIBus):
            pixel_format = 0x66
        else:
            pixel_format = 0x77
    else:
        if isinstance(self._data_bus, lcd_bus.SPIBus):
            raise RuntimeError(
                'ILI9488 IC only supports '
                'lv.COLOR_FORMAT.RGB888 when using the SPIBus'
            )
        else:
            raise RuntimeError(
                'ILI9488 IC only supports '
                'lv.COLOR_FORMAT.RGB565 or lv.COLOR_FORMAT.RGB888'
            )

    param_buf[0] = pixel_format
    self.set_params(_COLMOD, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(_IFMODE, param_mv[:1])

    param_buf[0] = 0xA0
    self.set_params(_FRMCTR1, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(_DIC, param_mv[:1])

    param_buf[0] = 0x02
    param_buf[1] = 0x02
    param_buf[2] = 0x3B
    self.set_params(_DFC, param_mv[:3])

    param_buf[0] = 0xC6
    self.set_params(_EM, param_mv[:1])

    param_buf[:4] = bytearray(
        [
            0xA9, 0x51, 0x2C, 0x02
        ]
    )
    self.set_params(_AC3, param_mv[:4])

    self.set_params(_DISPON)
    time.sleep_ms(100)  # NOQA
