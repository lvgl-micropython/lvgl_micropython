# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import display_driver_framework
from micropython import const  # NOQA

import lvgl as lv

# No-op
_NOP = const(0x0000)

# Software reset
_SWRESET = const(0x0100)

# Read display ID (0x0400 - 0x0402)
_RDDID = const(0x0400)

# Read number of errors (DSI only)
_RDNUMED = const(0x0500)

# Read Display Power Mode
_RDDPM = const(0x0A00)

# Read Display MADCTL
_RDDMADCTL = const(0x0B00)

# Read Display Pixel Format
_RDDCOLMOD = const(0x0C00)

# Read Display Image Mode
_RDDIM = const(0x0D00)

# Read Display Signal Mode
_RDDSM = const(0x0E00)

# Read Display Self-Diagnostic Result
_RDDSDR = const(0x0F00)

# Enter Sleep Mode
_SLPIN = const(0x1000)

# Sleep Out
_SLPOUT = const(0x1100)

# Partial Mode ON
_PTLON = const(0x1200)

# Normal Display Mode ON
_NORON = const(0x1300)

# Display Inversion OFF
_INVOFF = const(0x2000)

# Display Inversion ON
_INVON = const(0x2100)

# All pixels off
_ALLPOFF = const(0x2200)

# All pixels on
_ALLPON = const(0x2300)

# Gamma Set
_GAMSET = const(0x2600)

# Display OFF
_DISPOFF = const(0x2800)

# Display ON
_DISPON = const(0x2900)

# Column Address Set (0x2A00 - 0x2A03)
_CASET = const(0x2A00)

# Row Address Set (0x2B00 - 0x2B03)
_RASET = const(0x2B00)

# Memory Write
_RAMWR = const(0x2C00)

# Memory Read
_RAMRD = const(0x2E00)

# Partial Area (0x3000 - 0x3003)
_PTLAR = const(0x3000)

# Tearing effect line off
_TEOFF = const(0x3400)

# Tearing effect line on
_TEON = const(0x3500)

# Memory Access Control
_MADCTL = const(0x3600)

# Idle mode off
_IDMOFF = const(0x3800)

# Idle mode on
_IDMON = const(0x3900)

# Interface pixel format
_COLMOD = const(0x3A00)

# Memory write continue
_RAMWRC = const(0x3C00)

# Memory read continue
_RAMRDC = const(0x3E00)

# Set tearing effect line (0x4400-4401)
_STESL = const(0x4400)

# Get scan line (0x4500 - 0x4501)
_GSL = const(0x4500)

# Display clock in RGB interface
_DPCKRGB = const(0x4A00)

# Deep standby mode on
_DSTBON = const(0x4F00)

# Write profile value for display
_WRPFD = const(0x5000)

# Write display brightness
_WRDISBV = const(0x5100)

# Read display brightness
_RDDISBV = const(0x5200)

# Write CTRL display
_WRCTRLD = const(0x5300)

# Read CTRL display
_RDCTRLD = const(0x5400)

# Write content adaptive brightness
_WRCABC = const(0x5500)

# Read content adaptive brightness
_RDCABC = const(0x5600)

# Write hysteresis (0x5700 - 0x573F)
_WRHYSTE = const(0x5700)

# Write gamma setting (0x5800 - 0x5807)
_WRGAMMASET = const(0x5800)

# Read FS value MSBs
_RDFSVM = const(0x5A00)

# Read FS value LSBs
_RDFSVL = const(0x5B00)

# Read median filter FS value MSBs
_RDMFFSVM = const(0x5C00)

# Read median filter FS value LSBs
_RDMFFSVL = const(0x5D00)

# Write CABC minimum brightness
_WRCABCMB = const(0x5E00)

# Read CABC minimum brightness
_RDCABCMB = const(0x5F00)

# Write light sensor comp (0x6500-6501)
_WRLSCC = const(0x6500)

# Read light sensor value MSBs
_RDLSCCM = const(0x6600)

# Read light sensor value LSBs
_RDLSCCL = const(0x6700)

# Read black/white low bits
_RDBWLB = const(0x7000)

# Read Bkx
_RDBkx = const(0x7100)

# Read Bky
_RDBky = const(0x7200)

# Read Wx
_RDWx = const(0x7300)

# Read Wy
_RDWy = const(0x7400)

# Read red/green low bits
_RDRGLB = const(0x7500)

# Read Rx
_RDRx = const(0x7600)

# Read Ry
_RDRy = const(0x7700)

# Read Gx
_RDGx = const(0x7800)

# Read Gy
_RDGy = const(0x7900)

# Read blue/acolor low bits
_RDBALB = const(0x7A00)

# Read Bx
_RDBx = const(0x7B00)

# Read By
_RDBy = const(0x7C00)

# Read Ax
_RDAx = const(0x7D00)

# Read Ay
_RDAy = const(0x7E00)

# Read DDB start (0xA100 - 0xA104)
_RDDDBS = const(0xA100)

# Read DDB continue (0xA800 - 0xA804)
_RDDDBC = const(0xA800)

# Read first checksum
_RDFCS = const(0xAA00)

# Read continue checksum
_RDCCS = const(0xAF00)

# Read ID1 value
_RDID1 = const(0xDA00)

# Read ID2 value
_RDID2 = const(0xDB00)

# Read ID3 value
_RDID3 = const(0xDC00)

TYPE_TN = 1
TYPE_IPS = 2

STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


_MADCTL_MV = const(0x20)  # 0=Normal, 1=Row/column exchange
_MADCTL_MX = const(0x40)  # 0=Left to Right, 1=Right to Left
_MADCTL_MY = const(0x80)  # 0=Top to Bottom, 1=Bottom to Top


class NT35510(display_driver_framework.DisplayDriver):
    _INVON = 0x2100
    _INVOFF = 0x2000

    _ORIENTATION_TABLE = (
        0,
        _MADCTL_MV | _MADCTL_MX,
        _MADCTL_MX | _MADCTL_MY,
        _MADCTL_MV | _MADCTL_MY
    )

    def __init__(
        self,
        data_bus,
        display_width,
        display_height,
        frame_buffer1=None,
        frame_buffer2=None,
        reset_pin=None,
        reset_state=STATE_HIGH,
        power_pin=None,
        power_on_state=STATE_HIGH,
        backlight_pin=None,
        backlight_on_state=STATE_HIGH,
        offset_x=0,
        offset_y=0,
        color_byte_order=BYTE_ORDER_RGB,
        color_space=lv.COLOR_FORMAT.RGB888,  # NOQA
        rgb565_byte_swap=False
    ):

        super().__init__(
            data_bus=data_bus,
            display_width=display_width,
            display_height=display_height,
            frame_buffer1=frame_buffer1,
            frame_buffer2=frame_buffer2,
            reset_pin=reset_pin,
            reset_state=reset_state,
            power_pin=power_pin,
            power_on_state=power_on_state,
            backlight_pin=backlight_pin,
            backlight_on_state=backlight_on_state,
            offset_x=offset_x,
            offset_y=offset_y,
            color_byte_order=color_byte_order,
            color_space=color_space,  # NOQA
            rgb565_byte_swap=rgb565_byte_swap,
            _cmd_bits=16,
            _param_bits=16,
            _init_bus=True
        )

    def _set_memory_location(self, x1, y1, x2, y2):
        # Column addresses
        param_buf = self._param_buf  # NOQA

        param_buf[0] = (x1 >> 8) & 0xFF
        self._data_bus.tx_param(_CASET, self._param_mv[:1])

        param_buf[0] = x1 & 0xFF
        self._data_bus.tx_param(_CASET + 1, self._param_mv[:1])

        param_buf[0] = (x2 >> 8) & 0xFF
        self._data_bus.tx_param(_CASET + 2, self._param_mv[:1])

        param_buf[0] = x2 & 0xFF
        self._data_bus.tx_param(_CASET + 3, self._param_mv[:1])

        # Page addresses
        param_buf[0] = (y1 >> 8) & 0xFF
        self._data_bus.tx_param(_RASET, self._param_mv[:1])

        param_buf[0] = y1 & 0xFF
        self._data_bus.tx_param(_RASET + 1, self._param_mv[:1])

        param_buf[0] = (y2 >> 8) & 0xFF
        self._data_bus.tx_param(_RASET + 2, self._param_mv[:1])

        param_buf[0] = y2 & 0xFF
        self._data_bus.tx_param(_RASET + 3, self._param_mv[:1])

        return _RAMWR
