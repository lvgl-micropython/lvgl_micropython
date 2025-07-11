from micropython import const  # NOQA
import time
import lvgl as lv


_SLPOUT = const(0x11)
_INVOFF = const(0x20)
_INVON = const(0x21)
_DISPON = const(0x29)
_CASET = const(0x2A)
_PASET = const(0x2B)
_RAMWR = const(0x2C)
_TEOFF = const(0x34)
_TEON = const(0x35)
_COLMOD = const(0x3A)


def init(self):
    param_buf = bytearray(32)
    param_mv = memoryview(param_buf)

    self.set_params(_SLPOUT)
    time.sleep_ms(120)  # NOQA

    param_buf[:2] = bytearray([0x98, 0x53])
    self.set_params(0xDF, param_mv[:2])

    param_buf[:2] = bytearray([0x98, 0x53])
    self.set_params(0xDF, param_mv[:2])

    param_buf[0] = 0x23
    self.set_params(0xB2, param_mv[:1])

    param_buf[:4] = bytearray([0x00, 0x47, 0x00, 0x6F])
    self.set_params(0xB7, param_mv[:4])

    param_buf[:6] = bytearray([0x1C, 0x1A, 0x55, 0x73, 0x63, 0xF0])
    self.set_params(0xBB, param_mv[:6])

    param_buf[:2] = bytearray([0x44, 0xA4])
    self.set_params(0xC0, param_mv[:2])

    param_buf[0] = 0x16
    self.set_params(0xC1, param_mv[:1])

    param_buf[:8] = bytearray([0x7D, 0x07, 0x14, 0x06, 0xCF, 0x71, 0x72, 0x77])
    self.set_params(0xC3, param_mv[:8])

    param_buf[:12] = bytearray(
        [0x00, 0x00, 0xA0, 0x79, 0x0B, 0x0A, 0x16, 0x79, 0x0B, 0x0A, 0x16, 0x82]
    )
    self.set_params(0xC4, param_mv[:12])

    param_buf[:32] = bytearray(
        [0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28, 0x28, 0x26, 0x25, 0x17,
         0x12, 0x0D, 0x04, 0x00, 0x3F, 0x32, 0x29, 0x29, 0x27, 0x2B, 0x27, 0x28,
         0x28, 0x26, 0x25, 0x17, 0x12, 0x0D, 0x04, 0x00]
    )
    self.set_params(0xC8, param_mv[:32])

    param_buf[:5] = bytearray([0x04, 0x06, 0x6B, 0x0F, 0x00])
    self.set_params(0xD0, param_mv[:5])

    param_buf[:2] = bytearray([0x00, 0x30])
    self.set_params(0xD7, param_mv[:2])

    param_buf[0] = 0x14
    self.set_params(0xE6, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(0xDE, param_mv[:1])

    param_buf[:5] = bytearray([0x03, 0x13, 0xEF, 0x35, 0x35])
    self.set_params(0xB7, param_mv[:5])

    param_buf[:3] = bytearray([0x14, 0x15, 0xC0])
    self.set_params(0xC1, param_mv[:3])

    param_buf[:2] = bytearray([0x06, 0x3A])
    self.set_params(0xC2, param_mv[:2])

    param_buf[:2] = bytearray([0x72, 0x12])
    self.set_params(0xC4, param_mv[:2])

    param_buf[0] = 0x00
    self.set_params(0xBE, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0xDE, param_mv[:1])

    param_buf[:3] = bytearray([0x00, 0x02, 0x00])
    self.set_params(0xE5, param_mv[:3])

    param_buf[:3] = bytearray([0x01, 0x02, 0x00])
    self.set_params(0xE5, param_mv[:3])

    param_buf[0] = 0x00
    self.set_params(0xDE, param_mv[:1])

    self.set_params(_TEOFF)

    color_size = lv.color_format_get_size(self._color_space)
    if color_size == 2:  # NOQA
        pixel_format = 0x55
    elif color_size == 3:
        pixel_format = 0x66
    else:
        raise RuntimeError(
            'IC only supports '
            'lv.COLOR_FORMAT.RGB565 or lv.COLOR_FORMAT.RGB888'
        )

    param_buf[0] = pixel_format
    self.set_params(_COLMOD, param_mv[:1])

    param_buf[:4] = bytearray([0x00, 0x00, 0x00, 0x00])
    self.set_params(_CASET, param_mv[:4])

    param_buf[:4] = bytearray([0x00, 0x00, 0x00, 0x00])
    self.set_params(_PASET, param_mv[:4])

    param_buf[0] = 0x02
    self.set_params(0xDE, param_mv[:1])

    param_buf[:3] = bytearray([0x00, 0x02, 0x00])
    self.set_params(0xE5, param_mv[:3])

    param_buf[0] = 0x00
    self.set_params(0xDE, param_mv[:1])

    self.set_params(_DISPON)
    time.sleep_ms(120)  # NOQA


