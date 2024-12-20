
from micropython import const  # NOQA
import time

#  Software Reset Register (SRR)
_SRR = const(0x00)
#  Chip Configuration Register (CRR)
_CCR = const(0x01)
#  Memory Access Control Register (MACR)
_MACR = const(0x02)
#  Input Control Register (ICR)
_ICR = const(0x03)
#  Memory Data Read/Write Port (MRWDP)
_MRWDP = const(0x04)
#  SCLK PLL Control Register 1 (PPLLC1)
_PPLLC1 = const(0x05)
#  SCLK PLL Control Register 2 (PPLLC2)
_PPLLC2 = const(0x06)
#  MCLK PLL Control Register 1 (MPLLC1)
_MPLLC1 = const(0x07)
#  MCLK PLL Control Register 2 (MPLLC2)
_MPLLC2 = const(0x08)
#  CCLK PLL Control Register 1 (SPLLC1)
_SPLLC1 = const(0x09)
#  CCLK PLL Control Register 2 (SPLLC2)
_SPLLC2 = const(0x0A)
#  Main/PIP Window Control Register (MPWCTR)
_MPWCTR = const(0x10)
#  Display Configuration Register (DPCR)
_DPCR = const(0x12)
#  Panel scan Clock and Data Setting Register (PCSR)
_PCSR = const(0x13)
#  Horizontal Display Width Register (HDWR)
_HDWR = const(0x14)
#  Horizontal Display Width Fine Tune Register (HDWFTR)
_HDWFTR = const(0x15)
#  Horizontal Non-Display Period Register (HNDR)
_HNDR = const(0x16)
#  Horizontal Non-Display Period Fine Tune Register (HNDFTR)
_HNDFTR = const(0x17)
#  HSYNC Start Position Register (HSTR)
_HSTR = const(0x18)
#  HSYNC Pulse Width Register (HPWR)
_HPWR = const(0x19)
#  Vertical Display Height Register 0 (VDHR0)
_VDHR0 = const(0x1A)
#  Vertical Display Height Register 1 (VDHR1)
_VDHR1 = const(0x1B)
#  Vertical Non-Display Period Register 0 (VNDR0)
_VNDR0 = const(0x1C)
#  Vertical Non-Display Period Register 1 (VNDR1)
_VNDR1 = const(0x1D)
#  VSYNC Start Position Register (VSTR)
_VSTR = const(0x1E)
#  VSYNC Pulse Width Register (VPWR)
_VPWR = const(0x1F)
#  Main Image Start Address 0 (MISA0)
_MISA0 = const(0x20)
#  Main Image Start Address 1 (MISA1)
_MISA1 = const(0x21)
#  Main Image Start Address 2 (MISA2)
_MISA2 = const(0x22)
#  Main Image Start Address 3 (MISA3)
_MISA3 = const(0x23)
#  Main Image Width 0 (MIW0)
_MIW0 = const(0x24)
#  Main Image Width 1 (MIW1)
_MIW1 = const(0x25)
#  Main Window Upper-Left corner X-coordinates 0 (MWULX0)
_MWULX0 = const(0x26)
#  Main Window Upper-Left corner X-coordinates 1 (MWULX1)
_MWULX1 = const(0x27)
#  Main Window Upper-Left corner Y-coordinates 0 (MWULY0)
_MWULY0 = const(0x28)
#  Main Window Upper-Left corner Y-coordinates 1 (MWULY1)
_MWULY1 = const(0x29)
#  Canvas Start address 0 (CVSSA0)
_CVSSA0 = const(0x50)
#  Canvas Start address 1 (CVSSA1)
_CVSSA1 = const(0x51)
#  Canvas Start address 2 (CVSSA2)
_CVSSA2 = const(0x52)
#  Canvas Start address 3 (CVSSA3)
_CVSSA3 = const(0x53)
#  Canvas image width 0 (CVS_IMWTH0)
_CVS_IMWTH0 = const(0x54)
#  Canvas image width 1 (CVS_IMWTH1)
_CVS_IMWTH1 = const(0x55)
#  Active Window Upper-Left corner X-coordinates 0 (AWUL_X0)
_AWUL_X0 = const(0x56)
#  Active Window Upper-Left corner X-coordinates 1 (AWUL_X1)
_AWUL_X1 = const(0x57)
#  Active Window Upper-Left corner Y-coordinates 0 (AWUL_Y0)
_AWUL_Y0 = const(0x58)
#  Active Window Upper-Left corner Y-coordinates 1 (AWUL_Y1)
_AWUL_Y1 = const(0x59)
#  Active Window Width 0 (AW_WTH0)
_AW_WTH0 = const(0x5A)
#  Active Window Width 1 (AW_WTH1)
_AW_WTH1 = const(0x5B)
#  Active Window Height 0 (AW_HT0)
_AW_HT0 = const(0x5C)
#  Active Window Height 1 (AW_HT1)
_AW_HT1 = const(0x5D)
#  Color Depth of Canvas & Active Window (AW_COLOR)
_AW_COLOR = const(0x5E)
#  Graphic Read/Write position Horizontal Position Register 0 (CURH0)
_CURH0 = const(0x5F)
#  Graphic Read/Write position Horizontal Position Register 1 (CURH1)
_CURH1 = const(0x60)
#  Graphic Read/Write position Vertical Position Register 0 (CURV0)
_CURV0 = const(0x61)
#  Graphic Read/Write position Vertical Position Register 1 (CURV1)
_CURV1 = const(0x62)
#  SDRAM attribute register (SDRAR)
_SDRAR = const(0xE0)
#  SDRAM mode register & extended mode register (SDRMD)
_SDRMD = const(0xE1)
#  SDRAM auto refresh interval (SDR_REG_ITVL0)
_SDR_REF_ITVL0 = const(0xE2)
#  SDRAM auto refresh interval (SDR_REG_ITVL1)
_SDR_REF_ITVL1 = const(0xE3)
#  SDRAM Control register (SDRCR)
_SDRCR = const(0xE4)


_PLL_DIV_2 = const(0x02)  # PLL divided by 2
_PLL_DIV_4 = const(0x04)  # PLL divided by 4
_PLL_DIV_8 = const(0x06)  # PLL divided by 8
_PLL_DIV_16 = const(0x16)  # PLL divided by 16
_PLL_DIV_32 = const(0x26)  # PLL divided by 32
_PLL_DIV_64 = const(0x36)  # PLL divided by 64

# IS42SM16160D  -  integrated silicon solution dram IC
# _SDRAR_VAL = const(0xF9)
# _SDRMD_VAL = const(0x02)
# _SDRCR_VAL = const(0x09)
# _SDRAM_ITV_DIV = const(8192)


# IS42S16320B  -  integrated silicon solution dram IC
# _SDRAR_VAL = const(0x32)
# _SDRMD_VAL = const(0x02)
# _SDRCR_VAL = const(0x01)
# _SDRAM_ITV_DIV = const(8192)


# IS42S16400F  -  integrated silicon solution dram IC
# _SDRAR_VAL = const(0x28)
# _SDRMD_VAL = const(0x02)
# _SDRCR_VAL = const(0x01)
# _SDRAM_ITV_DIV = const(4096)


# M12L32162A  -  elite semiconductor dram IC
# _SDRAR_VAL = const(0x08)
# _SDRMD_VAL = const(0x03)
# _SDRCR_VAL = const(0x09)
# _SDRAM_ITV_DIV = const(4096)


# M12L2561616A  -  elite semiconductor dram IC
# _SDRAR_VAL = const(0x31)
# _SDRMD_VAL = const(0x03)
# _SDRCR_VAL = const(0x01)
# _SDRAM_ITV_DIV = const(8192)


# W9825G6JH  -  winbond dram IC
_SDRAR_VAL = const(0x31)
_SDRMD_VAL = const(0x03)
_SDRCR_VAL = const(0x01)
_SDRAM_ITV_DIV = const(8192)


# W9812G6JH  -  winbond dram IC
# _SDRAR_VAL = const(0x29)
# _SDRMD_VAL = const(0x03)
# _SDRCR_VAL = const(0x01)
# _SDRAM_ITV_DIV = const(4096)


# MT48LC4M16A  -  micron dram IC
# _SDRAR_VAL = const(0x28)
# _SDRMD_VAL = const(0x03)
# _SDRCR_VAL = const(0x01)
# _SDRAM_ITV_DIV = const(4096)


# K4S641632N  -  samsung dram IC
# _SDRAR_VAL = const(0x28)
# _SDRMD_VAL = const(0x03)
# _SDRCR_VAL = const(0x01)
# _SDRAM_ITV_DIV = const(4096)


# K4S281632K  -  samsung dram IC
# _SDRAR_VAL = const(0x29)
# _SDRMD_VAL = const(0x03)
# _SDRCR_VAL = const(0x01)
# _SDRAM_ITV_DIV = const(4096)


_OSC_FREQ = const(10)  # crystal clock (MHz)
_DRAM_FREQ = const(100)  # SDRAM clock frequency (MHz)
_CORE_FREQ = const(100)  # core (system) clock frequency (MHz)
_SCAN_FREQ = const(50)  # pixel scan clock frequency (MHz)

_HSYNC_BACK_PORCH = const(160)  # hndr
_HSYNC_FRONT_PORCH = const(160)  # hstr
_HSYNC_PULSE_WIDTH = const(70)  # hpwr

_VSYNC_BACK_PORCH = const(23)  # vndr
_VSYNC_FRONT_PORCH = const(12)  # vstr
_VSYNC_PULSE_WIDTH = const(10)  # vpwr


def init(self):
    time.sleep_ms(100)  # NOQA
    self.reset()
    time.sleep_ms(100)  # NOQA
    self._wait()

    buf = bytearray(1)
    mv = memoryview(buf)

    #  perform soft reset
    buf[0] = 0x01
    self.set_params(_SRR, mv)
    self._wait()

    # set pixel clock
    if _SCAN_FREQ >= 63:
        buf[0] = _PLL_DIV_4
        self.set_params(_PPLLC1, mv)
        buf[0] = int(_SCAN_FREQ * 4 / _OSC_FREQ) - 1
        self.set_params(_PPLLC2, mv)
    elif 32 <= _SCAN_FREQ <= 62:
        buf[0] = _PLL_DIV_8
        self.set_params(_PPLLC1, mv)
        buf[0] = int(_SCAN_FREQ * 8 / _OSC_FREQ) - 1
        self.set_params(_PPLLC2, mv)
    elif 16 <= _SCAN_FREQ <= 31:
        buf[0] = _PLL_DIV_16
        self.set_params(_PPLLC1, mv)
        buf[0] = int(_SCAN_FREQ * 16 / _OSC_FREQ) - 1
        self.set_params(_PPLLC2, mv)
    elif 8 <= _SCAN_FREQ <= 15:
        buf[0] = _PLL_DIV_32
        self.set_params(_PPLLC1, mv)
        buf[0] = int(_SCAN_FREQ * 32 / _OSC_FREQ) - 1
        self.set_params(_PPLLC2, mv)
    elif 0 < _SCAN_FREQ <= 7:
        buf[0] = _PLL_DIV_64
        self.set_params(_PPLLC1, mv)
        buf[0] = int(_SCAN_FREQ * 64 / _OSC_FREQ) - 1
        self.set_params(_PPLLC2, mv)
    else:
        raise RuntimeError(
            'unsupported scan frequency'
        )

    if _DRAM_FREQ >= 125:
        buf[0] = _PLL_DIV_2
        self.set_params(_MPLLC1, mv)
        buf[0] = int(_DRAM_FREQ * 2 / _OSC_FREQ) - 1
        self.set_params(_MPLLC2, mv)
    elif 63 <= _DRAM_FREQ <= 124:
        buf[0] = _PLL_DIV_4
        self.set_params(_MPLLC1, mv)
        buf[0] = int(_DRAM_FREQ * 4 / _OSC_FREQ) - 1
        self.set_params(_MPLLC2, mv)
    elif 31 <= _DRAM_FREQ <= 62:
        buf[0] = _PLL_DIV_8
        self.set_params(_MPLLC1, mv)
        buf[0] = int(_DRAM_FREQ * 8 / _OSC_FREQ) - 1
        self.set_params(_MPLLC2, mv)
    elif _DRAM_FREQ <= 30:
        buf[0] = _PLL_DIV_8
        self.set_params(_MPLLC1, mv)
        buf[0] = int(30 * 8 / _OSC_FREQ) - 1
        self.set_params(_MPLLC2, mv)
    else:
        raise RuntimeError('unsupported dram frequency')

    # set core clock
    if _CORE_FREQ >= 125:
        buf[0] = _PLL_DIV_2
        self.set_params(_SPLLC1, mv)
        buf[0] = int(_CORE_FREQ * 2 / _OSC_FREQ) - 1
        self.set_params(_SPLLC2, mv)
    elif 63 <= _CORE_FREQ <= 124:
        buf[0] = _PLL_DIV_4
        self.set_params(_SPLLC1, mv)
        buf[0] = int(_CORE_FREQ * 4 / _OSC_FREQ) - 1
        self.set_params(_SPLLC2, mv)
    elif 31 <= _CORE_FREQ <= 62:
        buf[0] = _PLL_DIV_8
        self.set_params(_SPLLC1, mv)
        buf[0] = int(_CORE_FREQ * 8 / _OSC_FREQ) - 1
        self.set_params(_SPLLC2, mv)
    elif _CORE_FREQ <= 30:
        buf[0] = _PLL_DIV_8
        self.set_params(_SPLLC1, mv)
        buf[0] = int(30 * 8 / _OSC_FREQ) - 1
        self.set_params(_SPLLC2, mv)
    else:
        raise RuntimeError('unsupported core frequency')

    # reconfigure PLL generator
    buf[0] = 0x00
    self.set_params(_CCR, mv)
    time.sleep_us(10)  # NOQA
    buf[0] = 0x80
    self.set_params(_CCR, mv)
    time.sleep_us(10)  # NOQA

    buf[0] = _SDRAR_VAL
    self.set_params(_SDRAR, mv)

    buf[0] = _SDRMD_VAL
    self.set_params(_SDRMD, mv)

    buf[0] = int(int(64000000 / _SDRAM_ITV_DIV) / int(1000 / _DRAM_FREQ)) - 2
    self.set_params(_SDR_REF_ITVL0, mv)

    buf[0] >>= 8
    self.set_params(_SDR_REF_ITVL1, mv)

    buf[0] = _SDRCR_VAL
    self.set_params(_SDRCR, mv)

    self._wait()
    time.sleep_ms(10)  # NOQA

    # set chip config register
    lane_count = self._data_bus.get_lane_count()
    if lane_count == 8:
        buf[0] = 0x00
    elif lane_count == 16:
        buf[0] = 0x01  # 16 lanes, 8 lanes = 0
    else:
        raise RuntimeError(
            f'Unsupported number of lanes, 8 or 16 is allowed ({lane_count})'
            )
    self.set_params(_CCR, mv)

    # configure memory address control register
    buf[0] = 0x40
    self.set_params(_MACR, mv)

    # set graphic mode
    buf[0] = 0x00
    self.set_params(_ICR, mv)

    # set display configuration register
    buf[0] = 0x00  # display off: 0x00, display on: 0x40
    self.set_params(_DPCR, mv)

    # set HSYNC+VSYNC+DE high active
    buf[0] = 0xC0
    self.set_params(_PCSR, mv)

    # set LCD width
    buf[0] = int(self.display_width / 8) - 1
    self.set_params(_HDWR, mv)
    buf[0] = int(self.display_width % 8)
    self.set_params(_HDWFTR, mv)

    # set LCD height
    buf[0] = int(self.display_height - 1) & 0xFF
    self.set_params(_VDHR0, mv)
    buf[0] = int(self.display_height - 1) >> 8
    self.set_params(_VDHR1, mv)

    # set horizontal non-display period / back porch
    buf[0] = int(_HSYNC_BACK_PORCH / 8) - 1
    self.set_params(_HNDR, mv)
    buf[0] = int(_HSYNC_BACK_PORCH % 8)
    self.set_params(_HNDFTR, mv)

    # set horizontal start position / front porch
    buf[0] = int(_HSYNC_FRONT_PORCH / 8) - 1
    self.set_params(_HSTR, mv)

    # set HSYNC pulse width
    buf[0] = int(_HSYNC_PULSE_WIDTH / 8) - 1
    self.set_params(_HPWR, mv)

    # set vertical non-display period
    buf[0] = (_VSYNC_BACK_PORCH - 1) & 0xFF
    self.set_params(_VNDR0, mv)
    buf[0] = (_VSYNC_BACK_PORCH - 1) >> 8
    self.set_params(_VNDR1, mv)

    # set vertical start position
    buf[0] = _VSYNC_FRONT_PORCH - 1
    self.set_params(_VSTR, mv)

    # set VSYNC pulse width
    buf[0] = _VSYNC_PULSE_WIDTH - 1
    self.set_params(_VPWR, mv)

    # set panel to PIP disabled, 16bpp TFT (65k colours)
    buf[0] = 0x04
    self.set_params(_MPWCTR, mv)

    # set main image start address to start of sdram
    buf[0] = 0x00 & 0xFF
    self.set_params(_MISA0, mv)
    buf[0] = 0x00 >> 8
    self.set_params(_MISA1, mv)
    buf[0] = 0x00 >> 16
    self.set_params(_MISA2, mv)
    buf[0] = 0x00 >> 24
    self.set_params(_MISA3, mv)

    # set main image width to panel width
    buf[0] = self.display_width & 0xFF
    self.set_params(_MIW0, mv)
    buf[0] = self.display_width >> 8
    self.set_params(_MIW1, mv)

    # set main window start coordinates to 0x0
    buf[0] = 0x00 & 0xFF
    self.set_params(_MWULX0, mv)
    buf[0] = 0x00 >> 8
    self.set_params(_MWULX1, mv)
    buf[0] = 0x00 & 0xFF
    self.set_params(_MWULY0, mv)
    buf[0] = 0x00 >> 8
    self.set_params(_MWULY1, mv)

    # set canvas image start address to start of sdram
    buf[0] = 0x00 & 0xFF
    self.set_params(_CVSSA0, mv)
    buf[0] = 0x00 >> 8
    self.set_params(_CVSSA1, mv)
    buf[0] = 0x00 >> 16
    self.set_params(_CVSSA2, mv)
    buf[0] = 0x00 >> 24
    self.set_params(_CVSSA3, mv)

    # set canvas image width to panel width
    buf[0] = self.display_width & 0xFF
    self.set_params(_CVS_IMWTH0, mv)
    buf[0] = self.display_width >> 8
    self.set_params(_CVS_IMWTH1, mv)

    # set top left corner of active window to 0,0
    buf[0] = 0x00 & 0xFF
    self.set_params(_AWUL_X0, mv)
    buf[0] = 0x00 >> 8
    self.set_params(_AWUL_X1, mv)
    buf[0] = 0x00 & 0xFF
    self.set_params(_AWUL_Y0, mv)
    buf[0] = 0x00 >> 8
    self.set_params(_AWUL_Y1, mv)

    # width active window to full panel width and height
    buf[0] = self.display_width & 0xFF
    self.set_params(_AW_WTH0, mv)
    buf[0] = self.display_width >> 8
    self.set_params(_AW_WTH1, mv)
    buf[0] = self.display_height & 0xFF
    self.set_params(_AW_HT0, mv)
    buf[0] = self.display_height >> 8
    self.set_params(_AW_HT1, mv)

    # configure block mode, and 16bpp memory mode
    buf[0] = 0x01
    self.set_params(_AW_COLOR, mv)

    # set panel to PIP disabled, 16bpp TFT (65k colours)
    buf[0] = 0x04
    self.set_params(_MPWCTR, mv)

    # set display configuration register
    buf[0] = 0x40  # display off: 0x00, display on: 0x40
    self.set_params(_DPCR, mv)
