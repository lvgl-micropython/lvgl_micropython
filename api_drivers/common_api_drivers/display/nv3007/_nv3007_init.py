# Copyright (c) 2024 - 2025 Kevin G. Schlosser
# NV3007 version (c) 2025 Elliot Williams who just ported init code taken from the internets

import time
from micropython import const  # NOQA
import lvgl as lv

_CMD_RAMCTRL  = const(0xB0)
_CMD_PORCTRL  = const(0xB2)      # Porch control
_CMD_GCTRL    = const(0xB7)      # Gate control
_CMD_VCOMS    = const(0xBB)      # VCOMS setting
_CMD_LCMCTRL  = const(0xC0)      # LCM control
_CMD_VDVVRHEN = const(0xC2)      # VDV and VRH command enable
_CMD_VRHS     = const(0xC3)      # VRH set
_CMD_VDVSET   = const(0xC4)      # VDV setting
_CMD_FRCTR2   = const(0xC6)      # FR Control 2
_CMD_PWCTRL1  = const(0xD0)      # Power control 1
_CMD_PVGAMCTRL= const(0xE0)      # Positive voltage gamma control
_CMD_NVGAMCTRL= const(0xE1)      # Negative voltage gamma control

_CMD_NOP     = const(0x00)
_CMD_SWRESET = const(0x01)
_CMD_RDDID   = const(0x04)
_CMD_RDDST   = const(0x09)
_CMD_SLPIN   = const(0x10)
_CMD_SLPOUT  = const(0x11)
_CMD_PTLON   = const(0x12)
_CMD_NORON   = const(0x13)
_CMD_INVOFF  = const(0x20)
_CMD_INVON   = const(0x21)
_CMD_GAMMASET= const(0x26)
_CMD_DISPOFF = const(0x28)
_CMD_DISPON  = const(0x29)
_CMD_CASET   = const(0x2A)
_CMD_RASET   = const(0x2B)
_CMD_PASET   = const(0x2B)
_CMD_RAMWR   = const(0x2C)
_CMD_RAMRD   = const(0x2E)
_CMD_TEON    = const(0x35)
_CMD_MADCTL  = const(0x36)
_CMD_IDMOFF  = const(0x38)
_CMD_IDMON   = const(0x39)
_CMD_COLMOD  = const(0x3A)
_CMD_PIXSET  = const(0x3A)
_CMD_GETSCANLINE= const(0x45)

def init(self):
    param_buf = bytearray(12)
    param_mv = memoryview(param_buf)

    param_buf[0] = 0xa5
    self.set_params(0xFF, param_mv[:1])

    param_buf[0] = 0x08 #/ PWRCTRL12
    self.set_params(0x9a, param_mv[:1])

    param_buf[0] = 0x08 #/ PWRCTRL13
    self.set_params(0x9b, param_mv[:1])

    param_buf[0] = 0xb0 #/ PWRCTRL14
    self.set_params(0x9c, param_mv[:1])

    param_buf[0] = 0x16 #/ PWRCTRL15
    self.set_params(0x9d, param_mv[:1])

    param_buf[0] = 0xc4 #/ PWRCTRL16
    self.set_params(0x9e, param_mv[:1])

    param_buf[:2] = bytearray([0x55, 0x04]) #/ PWRCTRL1
    self.set_params(0x8f, param_mv[:2])

    param_buf[0] = 0x90 #/ GLDOCTRL2
    self.set_params(0x84, param_mv[:1])

    param_buf[0] = 0x7b #/ GLDOCTRL1
    self.set_params(0x83, param_mv[:1])

    param_buf[0] = 0x33 #/ GLDOCTRL3
    self.set_params(0x85, param_mv[:1])

    param_buf[0] = 0x00 #/ GAMCTRL1
    self.set_params(0x60, param_mv[:1])

    param_buf[0] = 0x00 #/ GAMCTRL17
    self.set_params(0x70, param_mv[:1])

    param_buf[0] = 0x02 #/ GAMCTRL2
    self.set_params(0x61, param_mv[:1])

    param_buf[0] = 0x02 #/ GAMCTRL18
    self.set_params(0x71, param_mv[:1])

    param_buf[0] = 0x04 #/ GAMCTRL3
    self.set_params(0x62, param_mv[:1])

    param_buf[0] = 0x04 #/ GAMCTRL19
    self.set_params(0x72, param_mv[:1])

    param_buf[0] = 0x29 #/ GAMCTRL13
    self.set_params(0x6c, param_mv[:1])

    param_buf[0] = 0x29 #/ GAMCTRL29
    self.set_params(0x7c, param_mv[:1])

    param_buf[0] = 0x31 #/ GAMCTRL14
    self.set_params(0x6d, param_mv[:1])

    param_buf[0] = 0x31 #/ GAMCTRL30
    self.set_params(0x7d, param_mv[:1])

    param_buf[0] = 0x0f #/ GAMCTRL15
    self.set_params(0x6e, param_mv[:1])

    param_buf[0] = 0x0f #/ GAMCTRL31
    self.set_params(0x7e, param_mv[:1])

    param_buf[0] = 0x21 #/ GAMCTRL7
    self.set_params(0x66, param_mv[:1])

    param_buf[0] = 0x21 #/ GAMCTRL23
    self.set_params(0x76, param_mv[:1])

    param_buf[0] = 0x3A #/ GAMCTRL9
    self.set_params(0x68, param_mv[:1])

    param_buf[0] = 0x3A #/ GAMCTRL25
    self.set_params(0x78, param_mv[:1])

    param_buf[0] = 0x07 #/ GAMCTRL4
    self.set_params(0x63, param_mv[:1])

    param_buf[0] = 0x07 #/ GAMCTRL20
    self.set_params(0x73, param_mv[:1])

    param_buf[0] = 0x05 #/ GAMCTRL5
    self.set_params(0x64, param_mv[:1])

    param_buf[0] = 0x05 #/ GAMCTRL21
    self.set_params(0x74, param_mv[:1])

    param_buf[0] = 0x02 #/ GAMCTRL6
    self.set_params(0x65, param_mv[:1])

    param_buf[0] = 0x02 #/ GAMCTRL22
    self.set_params(0x75, param_mv[:1])

    param_buf[0] = 0x23 #/ GAMCTRL8
    self.set_params(0x67, param_mv[:1])

    param_buf[0] = 0x23 #/ GAMCTRL24
    self.set_params(0x77, param_mv[:1])

    param_buf[0] = 0x08 #/ GAMCTRL10
    self.set_params(0x69, param_mv[:1])

    param_buf[0] = 0x08 #/ GAMCTRL26
    self.set_params(0x79, param_mv[:1])

    param_buf[0] = 0x13 #/ GAMCTRL11
    self.set_params(0x6a, param_mv[:1])

    param_buf[0] = 0x13 #/ GAMCTRL27
    self.set_params(0x7a, param_mv[:1])

    param_buf[0] = 0x13 #/ GAMCTRL12
    self.set_params(0x6b, param_mv[:1])

    param_buf[0] = 0x13 #/ GAMCTRL28
    self.set_params(0x7b, param_mv[:1])

    param_buf[0] = 0x00 #/ GAMCTRL16
    self.set_params(0x6f, param_mv[:1])

    param_buf[0] = 0x00 #/ GAMCTRL32
    self.set_params(0x7f, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0x50, param_mv[:1])

    param_buf[0] = 0xd6
    self.set_params(0x52, param_mv[:1])

    param_buf[0] = 0x08 #/ ITCTRL1
    self.set_params(0x53, param_mv[:1])

    param_buf[0] = 0x08 #/ ITCTRL2
    self.set_params(0x54, param_mv[:1])

    param_buf[0] = 0x1e #/ ITCTRL3
    self.set_params(0x55, param_mv[:1])

    param_buf[0] = 0x1c #/ ITCTRL4
    self.set_params(0x56, param_mv[:1])

    param_buf[:3] = bytearray([0x2b, 0x24, 0x00]) #/ GOACTRL
    self.set_params(0xa0, param_mv[:3])

    param_buf[0] = 0x87 #/ VSTCTRL1
    self.set_params(0xa1, param_mv[:1])

    param_buf[0] = 0x86 #/ VSTCTRL2
    self.set_params(0xa2, param_mv[:1])

    param_buf[0] = 0x00 #/ VSTCTRL5
    self.set_params(0xa5, param_mv[:1])

    param_buf[0] = 0x00 #/ VSTCTRL6
    self.set_params(0xa6, param_mv[:1])

    param_buf[0] = 0x00 #/ VSTCTRL7
    self.set_params(0xa7, param_mv[:1])

    param_buf[0] = 0x36 #/ VSTCTRL8
    self.set_params(0xa8, param_mv[:1])

    param_buf[0] = 0x7e #/ VSTCTRL9
    self.set_params(0xa9, param_mv[:1])

    param_buf[0] = 0x7e #/ VSTCTRL10
    self.set_params(0xaa, param_mv[:1])

    param_buf[0] = 0x85 #/ CLKCTRL1
    self.set_params(0xB9, param_mv[:1])

    param_buf[0] = 0x84 #/ CLKCTRL2
    self.set_params(0xBA, param_mv[:1])

    param_buf[0] = 0x83 #/ CLKCTRL3
    self.set_params(0xBB, param_mv[:1])

    param_buf[0] = 0x82 #/ CLKCTRL4
    self.set_params(0xBC, param_mv[:1])

    param_buf[0] = 0x81 #/ CLKCTRL5
    self.set_params(0xBD, param_mv[:1])

    param_buf[0] = 0x80 #/ CLKCTRL6
    self.set_params(0xBE, param_mv[:1])

    param_buf[0] = 0x01 #/ CLKCTRL7
    self.set_params(0xBF, param_mv[:1])

    param_buf[0] = 0x02 #/ CLKCTRL8
    self.set_params(0xC0, param_mv[:1])

    param_buf[0] = 0x00 #/ CLKCTRL9
    self.set_params(0xc1, param_mv[:1])

    param_buf[0] = 0x00 #/ CLKCTRL10
    self.set_params(0xc2, param_mv[:1])

    param_buf[0] = 0x00 #/ CLKCTRL11
    self.set_params(0xc3, param_mv[:1])

    param_buf[0] = 0x33 #/ CLKCTRL12
    self.set_params(0xc4, param_mv[:1])

    param_buf[0] = 0x7e #/ CLKCTRL13
    self.set_params(0xc5, param_mv[:1])

    param_buf[0] = 0x7e #/ CLKCTRL14
    self.set_params(0xc6, param_mv[:1])

    param_buf[:2] = bytearray([0x33, 0x33]) #/ CLKCTRL16
    self.set_params(0xC8, param_mv[:2])

    param_buf[0] = 0x68 #/ CLKCTRL17
    self.set_params(0xC9, param_mv[:1])

    param_buf[0] = 0x69 #/ CLKCTRL18
    self.set_params(0xCA, param_mv[:1])

    param_buf[0] = 0x6a #/ CLKCTRL19
    self.set_params(0xCB, param_mv[:1])

    param_buf[0] = 0x6b #/ CLKCTRL20
    self.set_params(0xCC, param_mv[:1])
    
    param_buf[:2] = bytearray([0x33, 0x33]) #/ CLKCTRL21
    self.set_params(0xCD, param_mv[:2])

    param_buf[0] = 0x6c #/ CLKCTRL22
    self.set_params(0xCE, param_mv[:1])

    param_buf[0] = 0x6d #/ CLKCTRL23
    self.set_params(0xCF, param_mv[:1])

    param_buf[0] = 0x6e #/ CLKCTRL24
    self.set_params(0xD0, param_mv[:1])

    param_buf[0] = 0x6f #/ CLKCTRL25
    self.set_params(0xD1, param_mv[:1])
   
    param_buf[:2] = bytearray([0x03, 0x67])
    self.set_params(0xAB, param_mv[:2]) # VENDCTRL1

    param_buf[:2] = bytearray([0x03, 0x6b])
    self.set_params(0xAC, param_mv[:2]) # VENDCTRL2

    param_buf[:2] = bytearray([0x03,0x68])
    self.set_params(0xAD, param_mv[:2]) # VENDCTRL3

    param_buf[:2] = bytearray([0x03, 0x6c])
    self.set_params(0xAE, param_mv[:2]) # VENDCTRL4

    param_buf[0] = 0x00 #/ VENDCTRL9
    self.set_params(0xb3, param_mv[:1])

    param_buf[0] = 0x00 #/ VENDCTRL10
    self.set_params(0xb4, param_mv[:1])

    param_buf[0] = 0x00 #/ VENDCTRL11
    self.set_params(0xb5, param_mv[:1])

    param_buf[0] = 0x32 #/ VENDCTRL12
    self.set_params(0xB6, param_mv[:1])

    param_buf[0] = 0x7e #/ VENDCTRL13
    self.set_params(0xB7, param_mv[:1])

    param_buf[0] = 0x7e #/ VENDCTRL14
    self.set_params(0xB8, param_mv[:1])

    param_buf[0] = 0x00 #/ SOUCTRL1
    self.set_params(0xe0, param_mv[:1])
    
    param_buf[:2] = bytearray([0x03, 0x0f])
    self.set_params(0xE1, param_mv[:2]) # SOUCTRL2

    param_buf[0] = 0x04 #/ SOUCTRL3
    self.set_params(0xe2, param_mv[:1])

    param_buf[0] = 0x01 #/ SOUCTRL4
    self.set_params(0xe3, param_mv[:1])

    param_buf[0] = 0x0e #/ SOUCTRL5
    self.set_params(0xe4, param_mv[:1])

    param_buf[0] = 0x01 #/ SOUCTRL6
    self.set_params(0xe5, param_mv[:1])

    param_buf[0] = 0x19 #/ SOUCTRL7
    self.set_params(0xe6, param_mv[:1])

    param_buf[0] = 0x10 #/ SOUCTRL8
    self.set_params(0xe7, param_mv[:1])

    param_buf[0] = 0x10 #/ SOUCTRL9
    self.set_params(0xe8, param_mv[:1])

    param_buf[0] = 0x12 #/ SOUCTRL11
    self.set_params(0xea, param_mv[:1])

    param_buf[0] = 0xd0 #/ SOUCTRL12
    self.set_params(0xeb, param_mv[:1])

    param_buf[0] = 0x04 #/ SOUCTRL13
    self.set_params(0xec, param_mv[:1])

    param_buf[0] = 0x07 #/ SOUCTRL14
    self.set_params(0xed, param_mv[:1])

    param_buf[0] = 0x07 #/ SOUCTRL15
    self.set_params(0xee, param_mv[:1])

    param_buf[0] = 0x09 #/ SOUCTRL16
    self.set_params(0xef, param_mv[:1])

    param_buf[0] = 0xd0 #/ SOUCTRL17
    self.set_params(0xf0, param_mv[:1])

    param_buf[0] = 0x0e #/ SOUCTRL18
    self.set_params(0xf1, param_mv[:1])

    param_buf[0] = 0x17 #/ FSMCTRL
    self.set_params(0xF9, param_mv[:1])

    param_buf[:4] = bytearray([0x2c, 0x1b, 0x0b, 0x20])
    self.set_params(0xF2, param_mv[:4]) # SOUCTRL19

    param_buf[0] = 0x29 #/ SOUCTRL10
    self.set_params(0xe9, param_mv[:1])

    param_buf[0] = 0x04 #/ SOUCTRL13
    self.set_params(0xec, param_mv[:1])

    self.set_params(_CMD_TEON)

    param_buf[:2] = bytearray([0x00, 0x10])
    self.set_params(0x44, param_mv[:2]) # TECTRL1

    param_buf[0] = 0x10 #/ TECTRL3
    self.set_params(0x46, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xff, param_mv[:1])

    param_buf[0] = 0x05
    self.set_params(_CMD_COLMOD, param_mv[:1])

    self.set_params(_CMD_SLPOUT)
    time.sleep_ms(100)

    self.set_params(_CMD_NORON)
    time.sleep_ms(50)

    self.set_params(_CMD_DISPON)
    time.sleep_ms(50)


