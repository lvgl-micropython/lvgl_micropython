from micropython import const

import time
import lvgl as lv  # NOQA


_CSC1 = const(0xF0)
_CSC2 = const(0xF1)
_CSC3 = const(0xF2)
_CSC4 = const(0xF3)
_SPIOR = const(0xF4)

# command table 1
_SLPOUT = const(0x11)
_TEOFF = const(0x34)
_INVON = const(0x21)
_RASET = const(0x2B)
_CASET = const(0x2A)
_RAMCLACT = const(0x4C)
_RAMCLSETR = const(0x4D)
_RAMCLSETG = const(0x4E)
_RAMCLSETB = const(0x4F)
_DISPON = const(0x29)
_MADCTL = const(0x36)
_COLMOD = const(0x3A)

# command table 2
_VRHPS = const(0xB0)
_VRHNS = const(0xB1)
_VCOMS = const(0xB2)
_GAMOPPS = const(0xB4)
_STEP14S = const(0xB5)
_STEP23S = const(0xB6)
_SBSTS = const(0xB7)
_TCONS = const(0xBA)
_RGBVBP = const(0xBB)
_RGBHBP = const(0xBC)
_RGBSET = const(0xBD)
_FRCTRA1 = const(0xC0)
_FRCTRA2 = const(0xC1)
_FRCTRA3 = const(0xC2)
_FRCTRB1 = const(0xC3)
_FRCTRB2 = const(0xC4)
_FRCTRB3 = const(0xC5)
_PWRCTRA1 = const(0xC6)
_PWRCTRA2 = const(0xC7)
_PWRCTRA3 = const(0xC8)
_PWRCTRB1 = const(0xC9)
_PWRCTRB2 = const(0xCA)
_PWRCTRB3 = const(0xCB)
_RESSET1 = const(0xD0)
_RESSET2 = const(0xD1)
_RESSET3 = const(0xD2)
_VCMOFSET = const(0xDD)
_VCMOFNSET = const(0xDE)
_GAMCTRP1 = const(0xE0)
_GAMCTRN1 = const(0xE1)


def init(self):
    param_buf = bytearray(14)
    param_mv = memoryview(param_buf)

    param_buf[0] = 0x08
    self.set_params(_CSC1, param_mv[:1])

    param_buf[0] = 0x08
    self.set_params(_CSC3, param_mv[:1])

    param_buf[0] = 0x51
    self.set_params(0x9B, param_mv[:1])

    param_buf[0] = 0x53
    self.set_params(0x86, param_mv[:1])

    param_buf[0] = 0x80
    self.set_params(_CSC3, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(_CSC1, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(_CSC1, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(_CSC2, param_mv[:1])

    param_buf[0] = 0x54
    self.set_params(_VRHPS, param_mv[:1])

    param_buf[0] = 0x3F
    self.set_params(_VRHNS, param_mv[:1])

    param_buf[0] = 0x2A
    self.set_params(_VCOMS, param_mv[:1])

    param_buf[0] = 0x46
    self.set_params(_GAMOPPS, param_mv[:1])

    param_buf[0] = 0x34
    self.set_params(_STEP14S, param_mv[:1])

    param_buf[0] = 0xD5
    self.set_params(_STEP23S, param_mv[:1])

    param_buf[0] = 0x30
    self.set_params(_SBSTS, param_mv[:1])

    param_buf[0] = 0x04
    self.set_params(0xB8, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(_TCONS, param_mv[:1])

    param_buf[0] = 0x08
    self.set_params(_RGBVBP, param_mv[:1])

    param_buf[0] = 0x08
    self.set_params(_RGBHBP, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(_RGBSET, param_mv[:1])

    param_buf[0] = 0x80
    self.set_params(_FRCTRA1, param_mv[:1])

    param_buf[0] = 0x10
    self.set_params(_FRCTRA2, param_mv[:1])

    param_buf[0] = 0x37
    self.set_params(_FRCTRA3, param_mv[:1])

    param_buf[0] = 0x80
    self.set_params(_FRCTRB1, param_mv[:1])

    param_buf[0] = 0x10
    self.set_params(_FRCTRB2, param_mv[:1])

    param_buf[0] = 0x37
    self.set_params(_FRCTRB3, param_mv[:1])

    param_buf[0] = 0xA9
    self.set_params(_PWRCTRA1, param_mv[:1])

    param_buf[0] = 0x41
    self.set_params(_PWRCTRA2, param_mv[:1])

    param_buf[0] = 0x51
    self.set_params(_PWRCTRA3, param_mv[:1])

    param_buf[0] = 0xA9
    self.set_params(_PWRCTRB1, param_mv[:1])

    param_buf[0] = 0x41
    self.set_params(_PWRCTRB2, param_mv[:1])

    param_buf[0] = 0x51
    self.set_params(_PWRCTRB3, param_mv[:1])

    param_buf[0] = 0x91
    self.set_params(_RESSET1, param_mv[:1])

    param_buf[0] = 0x68
    self.set_params(_RESSET2, param_mv[:1])

    param_buf[0] = 0x69
    self.set_params(_RESSET3, param_mv[:1])

    param_buf[:2] = bytearray([0x00, 0xA5])
    self.set_params(0xF5, param_mv[:2])

    param_buf[0] = 0x35
    self.set_params(_VCMOFSET, param_mv[:1])

    param_buf[0] = 0x35
    self.set_params(_VCMOFNSET, param_mv[:1])

    param_buf[0] = 0x10
    self.set_params(_CSC2, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(_CSC1, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(_CSC1, param_mv[:1])

    param_buf[:14] = bytearray([0x70, 0x09, 0x12, 0x0C, 0x0B, 0x27, 0x38, 0x54, 0x4E, 0x19, 0x15, 0x15, 0x2C, 0x2F])
    self.set_params(0xE0, param_mv[:14])

    param_buf[:14] = bytearray([0x70, 0x08, 0x11, 0x0C, 0x0B, 0x27, 0x38, 0x43, 0x4C, 0x18, 0x14, 0x14, 0x2B, 0x2D])
    self.set_params(0xE1, param_mv[:14])

    param_buf[0] = 0x10
    self.set_params(_CSC1, param_mv[:1])

    param_buf[0] = 0x10
    self.set_params(_CSC4, param_mv[:1])

    param_buf[0] = 0x0A
    self.set_params(0xE0, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xE1, param_mv[:1])

    param_buf[0] = 0x0B
    self.set_params(0xE2, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xE3, param_mv[:1])

    param_buf[0] = 0xE0
    self.set_params(0xE4, param_mv[:1])

    param_buf[0] = 0x06
    self.set_params(0xE5, param_mv[:1])

    param_buf[0] = 0x21
    self.set_params(0xE6, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xE7, param_mv[:1])

    param_buf[0] = 0x05
    self.set_params(0xE8, param_mv[:1])

    param_buf[0] = 0x82
    self.set_params(0xE9, param_mv[:1])

    param_buf[0] = 0xDF
    self.set_params(0xEA, param_mv[:1])

    param_buf[0] = 0x89
    self.set_params(0xEB, param_mv[:1])

    param_buf[0] = 0x20
    self.set_params(0xEC, param_mv[:1])

    param_buf[0] = 0x14
    self.set_params(0xED, param_mv[:1])

    param_buf[0] = 0xFF
    self.set_params(0xEE, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xEF, param_mv[:1])

    param_buf[0] = 0xFF
    self.set_params(0xF8, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xF9, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xFA, param_mv[:1])

    param_buf[0] = 0x30
    self.set_params(0xFB, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xFC, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xFD, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xFE, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xFF, param_mv[:1])

    param_buf[0] = 0x42
    self.set_params(0x60, param_mv[:1])

    param_buf[0] = 0xE0
    self.set_params(0x61, param_mv[:1])

    param_buf[0] = 0x40
    self.set_params(0x62, param_mv[:1])

    param_buf[0] = 0x40
    self.set_params(0x63, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x64, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x65, param_mv[:1])

    param_buf[0] = 0x40
    self.set_params(0x66, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x67, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x68, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x69, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x6A, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x6B, param_mv[:1])

    param_buf[0] = 0x42
    self.set_params(0x70, param_mv[:1])

    param_buf[0] = 0xE0
    self.set_params(0x71, param_mv[:1])

    param_buf[0] = 0x40
    self.set_params(0x72, param_mv[:1])

    param_buf[0] = 0x40
    self.set_params(0x73, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x74, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x75, param_mv[:1])

    param_buf[0] = 0x40
    self.set_params(0x76, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0x77, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x78, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x79, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x7A, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x7B, param_mv[:1])

    param_buf[0] = 0x38
    self.set_params(0x80, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x81, param_mv[:1])

    param_buf[0] = 0x04
    self.set_params(0x82, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x83, param_mv[:1])

    param_buf[0] = 0xDC
    self.set_params(0x84, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x85, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x86, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x87, param_mv[:1])

    param_buf[0] = 0x38
    self.set_params(0x88, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x89, param_mv[:1])

    param_buf[0] = 0x06
    self.set_params(0x8A, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x8B, param_mv[:1])

    param_buf[0] = 0xDE
    self.set_params(0x8C, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x8D, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x8E, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x8F, param_mv[:1])

    param_buf[0] = 0x38
    self.set_params(0x90, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x91, param_mv[:1])

    param_buf[0] = 0x08
    self.set_params(0x92, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x93, param_mv[:1])

    param_buf[0] = 0xE0
    self.set_params(0x94, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x95, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x96, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x97, param_mv[:1])

    param_buf[0] = 0x38
    self.set_params(0x98, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x99, param_mv[:1])

    param_buf[0] = 0x0A
    self.set_params(0x9A, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0x9B, param_mv[:1])

    param_buf[0] = 0xE2
    self.set_params(0x9C, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x9D, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x9E, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x9F, param_mv[:1])

    param_buf[0] = 0x38
    self.set_params(0xA0, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xA1, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(0xA2, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0xA3, param_mv[:1])

    param_buf[0] = 0xDB
    self.set_params(0xA4, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xA5, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xA6, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xA7, param_mv[:1])

    param_buf[0] = 0x38
    self.set_params(0xA8, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xA9, param_mv[:1])

    param_buf[0] = 0x05
    self.set_params(0xAA, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0xAB, param_mv[:1])

    param_buf[0] = 0xDD
    self.set_params(0xAC, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xAD, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xAE, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xAF, param_mv[:1])

    param_buf[0] = 0x38
    self.set_params(0xB0, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xB1, param_mv[:1])

    param_buf[0] = 0x07
    self.set_params(0xB2, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0xB3, param_mv[:1])

    param_buf[0] = 0xDF
    self.set_params(0xB4, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xB5, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xB6, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xB7, param_mv[:1])

    param_buf[0] = 0x38
    self.set_params(0xB8, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xB9, param_mv[:1])

    param_buf[0] = 0x09
    self.set_params(0xBA, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(0xBB, param_mv[:1])

    param_buf[0] = 0xE1
    self.set_params(0xBC, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xBD, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xBE, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xBF, param_mv[:1])

    param_buf[0] = 0x22
    self.set_params(0xC0, param_mv[:1])

    param_buf[0] = 0xAA
    self.set_params(0xC1, param_mv[:1])

    param_buf[0] = 0x65
    self.set_params(0xC2, param_mv[:1])

    param_buf[0] = 0x74
    self.set_params(0xC3, param_mv[:1])

    param_buf[0] = 0x47
    self.set_params(0xC4, param_mv[:1])

    param_buf[0] = 0x56
    self.set_params(0xC5, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xC6, param_mv[:1])

    param_buf[0] = 0x88
    self.set_params(0xC7, param_mv[:1])

    param_buf[0] = 0x99
    self.set_params(0xC8, param_mv[:1])

    param_buf[0] = 0x33
    self.set_params(0xC9, param_mv[:1])

    param_buf[0] = 0x11
    self.set_params(0xD0, param_mv[:1])

    param_buf[0] = 0xAA
    self.set_params(0xD1, param_mv[:1])

    param_buf[0] = 0x65
    self.set_params(0xD2, param_mv[:1])

    param_buf[0] = 0x74
    self.set_params(0xD3, param_mv[:1])

    param_buf[0] = 0x47
    self.set_params(0xD4, param_mv[:1])

    param_buf[0] = 0x56
    self.set_params(0xD5, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xD6, param_mv[:1])

    param_buf[0] = 0x88
    self.set_params(0xD7, param_mv[:1])

    param_buf[0] = 0x99
    self.set_params(0xD8, param_mv[:1])

    param_buf[0] = 0x33
    self.set_params(0xD9, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(_CSC4, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(_CSC1, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(_CSC1, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(_CSC2, param_mv[:1])

    param_buf[0] = 0x0B
    self.set_params(0xA0, param_mv[:1])

    param_buf[0] = 0x2A
    self.set_params(0xA3, param_mv[:1])

    param_buf[0] = 0xC3
    self.set_params(0xA5, param_mv[:1])
    time.sleep_ms(1)  # NOQA

    param_buf[0] = 0x2B
    self.set_params(0xA3, param_mv[:1])

    param_buf[0] = 0xC3
    self.set_params(0xA5, param_mv[:1])
    time.sleep_ms(1)  # NOQA

    param_buf[0] = 0x2C
    self.set_params(0xA3, param_mv[:1])

    param_buf[0] = 0xC3
    self.set_params(0xA5, param_mv[:1])
    time.sleep_ms(1)  # NOQA

    param_buf[0] = 0x2D
    self.set_params(0xA3, param_mv[:1])

    param_buf[0] = 0xC3
    self.set_params(0xA5, param_mv[:1])
    time.sleep_ms(1)  # NOQA

    param_buf[0] = 0x2E
    self.set_params(0xA3, param_mv[:1])

    param_buf[0] = 0xC3
    self.set_params(0xA5, param_mv[:1])
    time.sleep_ms(1)  # NOQA

    param_buf[0] = 0x2F
    self.set_params(0xA3, param_mv[:1])

    param_buf[0] = 0xC3
    self.set_params(0xA5, param_mv[:1])
    time.sleep_ms(1)  # NOQA

    param_buf[0] = 0x30
    self.set_params(0xA3, param_mv[:1])

    param_buf[0] = 0xC3
    self.set_params(0xA5, param_mv[:1])
    time.sleep_ms(1)  # NOQA

    param_buf[0] = 0x31
    self.set_params(0xA3, param_mv[:1])

    param_buf[0] = 0xC3
    self.set_params(0xA5, param_mv[:1])
    time.sleep_ms(1)  # NOQA

    param_buf[0] = 0x32
    self.set_params(0xA3, param_mv[:1])

    param_buf[0] = 0xC3
    self.set_params(0xA5, param_mv[:1])
    time.sleep_ms(1)  # NOQA

    param_buf[0] = 0x33
    self.set_params(0xA3, param_mv[:1])

    param_buf[0] = 0xC3
    self.set_params(0xA5, param_mv[:1])
    time.sleep_ms(1)  # NOQA

    param_buf[0] = 0x09
    self.set_params(0xA0, param_mv[:1])

    param_buf[0] = 0x10
    self.set_params(_CSC2, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(_CSC1, param_mv[:1])

    param_buf[:4] = bytearray([0x00, 0x00, 0x01, 0x67])
    self.set_params(_CASET, param_mv[:4])

    param_buf[:4] = bytearray([0x01, 0x68, 0x01, 0x68])
    self.set_params(_RASET, param_mv[:4])

    param_buf[0] = 0x00
    self.set_params(_RAMCLSETR, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(_RAMCLSETG, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(_RAMCLSETB, param_mv[:1])

    param_buf[0] = 0x01
    self.set_params(_RAMCLACT, param_mv[:1])
    time.sleep_ms(10)  # NOQA

    param_buf[0] = 0x00
    self.set_params(_RAMCLACT, param_mv[:1])

    param_buf[:4] = bytearray([0x00, 0x00, 0x01, 0x67])
    self.set_params(_CASET, param_mv[:4])

    param_buf[:4] = bytearray([0x00, 0x00, 0x01, 0x67])
    self.set_params(_RASET, param_mv[:4])

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

    self.set_params(_INVON)
    self.set_params(_TEOFF)

    param_buf[0] = 0x00
    self.set_params(_SLPOUT, param_mv[:1])
    time.sleep_ms(120)  # NOQA
