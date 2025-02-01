# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import time


# Software Reset
_SRR = const(0x00)

# Chip Configuration
_CCR = const(0x01)

# Memory Access Control
_MACR = const(0x02)

# Input Control
_ICR = const(0x03)

# SCLK PLL Control Register 1
_PPLLC1 = const(0x05)

# SCLK PLL Control Register 2
_PPLLC2 = const(0x06)

# MCLK PLL Control Register 1
_MPLLC1 = const(0x07)

#  MCLK PLL Control Register 2
_MPLLC2 = const(0x08)

# CCLK PLL Control Register 1
_SPLLC1 = const(0x09)

# CCLK PLL Control Register 2
_SPLLC2 = const(0x0A)

# SDRAM attribute register
_SDRAR = const(0xE0)

# DRAM mode register & extended mode register
_SDRMD = const(0xE1)

# DRAM auto refresh interval 0
_SDR_REF_ITVL0 = const(0xE2)

# DRAM auto refresh interval 1
_SDR_REF_ITVL1 = const(0xE3)

# SDRAM Control register
_SDRCR = const(0xE4)

# Display Configuration
_DPCR = const(0x12)

# Panel scan Clock and Data Setting
_PCSR = const(0x13)

# Horizontal Display Width
_HDWR = const(0x14)

# Horizontal Display Width Fine Tune
_HDWFTR = const(0x15)

# Horizontal Non-Display Period
_HNDR = const(0x16)

# Horizontal Non-Display Period Fine Tune
_HNDFTR = const(0x17)

# HSYNC Start Position
_HSTR = const(0x18)

# HSYNC Pulse Width
_HPWR = const(0x19)

# Vertical Display Height 0
_VDHR0 = const(0x1A)
_VDHR1 = const(0x1B)

# Vertical Non-Display Period
_VNDR0 = const(0x1C)
_VNDR1 = const(0x1D)

# VSYNC Start Position
_VSTR = const(0x1E)

# VSYNC Pulse Width
_VPWR = const(0x1F)

# Main Image Start Address
_MISA0 = const(0x20)
_MISA1 = const(0x21)
_MISA2 = const(0x22)
_MISA3 = const(0x23)

# Main Image Width
_MIW0 = const(0x24)
_MIW1 = const(0x25)

# Main Window Upper-Left corner X-coordinates
_MWULX0 = const(0x26)
_MWULX1 = const(0x27)

# Main Window Upper-Left corner Y-coordinates
_MWULY0 = const(0x28)
_MWULY1 = const(0x29)

# Canvas Start address
_CVSSA0 = const(0x50)
_CVSSA1 = const(0x51)
_CVSSA2 = const(0x52)
_CVSSA3 = const(0x53)

# Canvas image width
_IMWTH0 = const(0x54)
_IMWTH1 = const(0x55)

# Active Window Upper-Left corner X-coordinates
_AWUL_X0 = const(0x56)
_AWUL_X1 = const(0x57)

# Active Window Upper-Left corner Y-coordinates
_AWUL_Y0 = const(0x58)
_AWUL_Y1 = const(0x59)

# Active Window Width
_AW_WTH0 = const(0x5A)
_AW_WTH1 = const(0x5B)

# Active Window Height
_AW_HT0 = const(0x5C)
_AW_HT1 = const(0x5D)

# Color Depth of Canvas & Active Window
_AW_COLOR = const(0x5E)

# Main/PIP Window Control
_MPWCTR = const(0x10)


def PLL_Initial(self, SCAN_FREQ, OSC_FREQ, DRAM_FREQ, CORE_FREQ):
    # Set pixel clock
    if SCAN_FREQ >= 63:
        self._write_reg(_PPLLC1, 0x04)  # PLL Divided by 4
        self._write_reg(_PPLLC2, int(SCAN_FREQ * 4 / OSC_FREQ) - 1)

    elif SCAN_FREQ >= 32:
        self._write_reg(_PPLLC1, 0x06)  # PLL Divided by 8
        self._write_reg(_PPLLC2, int(SCAN_FREQ * 8 / OSC_FREQ) - 1)

    elif SCAN_FREQ >= 16:
        self._write_reg(_PPLLC1, 0x16)  # PLL Divided by 16
        self._write_reg(_PPLLC2, int(SCAN_FREQ * 16 / OSC_FREQ) - 1)

    elif SCAN_FREQ >= 8:
        self._write_reg(_PPLLC1, 0x26)  # PLL Divided by 32
        self._write_reg(_PPLLC2, int(SCAN_FREQ * 32 / OSC_FREQ) - 1)

    elif SCAN_FREQ > 0:
        self._write_reg(_PPLLC1, 0x36)  # PLL Divided by 64
        self._write_reg(_PPLLC2, int(SCAN_FREQ * 64 / OSC_FREQ) - 1)
    else:
        raise RuntimeError

    # Set SDRAM clock
    if DRAM_FREQ >= 125:
        self._write_reg(_MPLLC1, 0x02)  # PLL Divided by 2
        self._write_reg(_MPLLC2, int(DRAM_FREQ * 2 / OSC_FREQ) - 1)

    elif DRAM_FREQ >= 63:
        self._write_reg(_MPLLC1, 0x04)  # PLL Divided by 4
        self._write_reg(_MPLLC2, int(DRAM_FREQ * 4 / OSC_FREQ) - 1)

    elif DRAM_FREQ >= 31:
        self._write_reg(_MPLLC1, 0x06)  # PLL Divided by 8
        self._write_reg(_MPLLC2, int(DRAM_FREQ * 8 / OSC_FREQ) - 1)

    elif DRAM_FREQ > 0:
        self._write_reg(_MPLLC1, 0x06)  # PLL Divided by 8
        self._write_reg(_MPLLC2, int(30 * 8 / OSC_FREQ) - 1)

    else:
        raise RuntimeError

    # Set Core clock
    if CORE_FREQ >= 125:
        self._write_reg(_SPLLC1, 0x02)  # PLL Divided by 2
        self._write_reg(_SPLLC2, int(CORE_FREQ * 2 / OSC_FREQ) - 1)

    elif CORE_FREQ >= 63:
        self._write_reg(_SPLLC1, 0x04)  # PLL Divided by 4
        self._write_reg(_SPLLC2, int(CORE_FREQ * 4 / OSC_FREQ) - 1)

    elif CORE_FREQ >= 31:
        self._write_reg(_SPLLC1, 0x06)  # PLL Divided by 8
        self._write_reg(_SPLLC2, int(CORE_FREQ * 8 / OSC_FREQ) - 1)

    elif CORE_FREQ > 0:
        self._write_reg(_SPLLC1, 0x06)  # PLL Divided by 8
        self._write_reg(_SPLLC2, int(30 * 8 / OSC_FREQ) - 1)
    else:
        raise RuntimeError


_IS42SM16160D = const(0)
_IS42S16320B = const(1)
_IS42S16400F = const(3)
_M12L32162A = const(4)
_M12L2561616A = const(5)
_W9825G6JH = const(6)
_W9812G6JH = const(7)
_MT48LC4M16A = const(8)
_K4S641632N = const(9)
_K4S281632K = const(10)


def DRAM_Initial(self, type, DRAM_FREQ):  # NOQA

    if type == _IS42SM16160D:
        self._write_reg(_SDRAR, 0xf9)
        self._write_reg(_SDRMD, 0x02)  # CAS:2=0x02 ACAS:3=0x03

        sdram_itv = int(int(64000000 / 8192) / int(1000 / DRAM_FREQ)) - 2

        self._write_reg(_SDR_REF_ITVL0, sdram_itv)
        self._write_reg(_SDR_REF_ITVL1, sdram_itv >> 8)
        self._write_reg(_SDRCR, 0x09)

    elif type == _IS42S16320B:
        self._write_reg(_SDRAR, 0x32)
        self._write_reg(_SDRMD, 0x02)  # CAS:2=0x02 ACAS:3=0x03

        sdram_itv = int(int(64000000 / 8192) / int(1000 / DRAM_FREQ)) - 2

        self._write_reg(_SDR_REF_ITVL0, sdram_itv)
        self._write_reg(_SDR_REF_ITVL1, sdram_itv >> 8)

        self._write_reg(_SDRCR, 0x01)

    elif type == _IS42S16400F:
        self._write_reg(_SDRAR, 0x28)
        self._write_reg(_SDRMD, 0x02)  # CAS:2=0x02 ACAS:3=0x03

        sdram_itv = int(int(64000000 / 4096) / int(1000 / DRAM_FREQ)) - 2

        self._write_reg(_SDR_REF_ITVL0, sdram_itv)
        self._write_reg(_SDR_REF_ITVL1, sdram_itv >> 8)

        self._write_reg(_SDRCR, 0x01)

    elif type == _M12L32162A:
        self._write_reg(_SDRAR, 0x08)
        self._write_reg(_SDRMD, 0x03)  # CAS:2=0x02 ACAS:3=0x03

        sdram_itv = int(int(64000000 / 4096) / int(1000 / DRAM_FREQ)) - 2

        self._write_reg(_SDR_REF_ITVL0, sdram_itv)
        self._write_reg(_SDR_REF_ITVL1, sdram_itv >> 8)

        self._write_reg(_SDRCR, 0x09)

    elif type == _M12L2561616A:
        self._write_reg(_SDRAR, 0x31)
        self._write_reg(_SDRMD, 0x03)  # CAS:2=0x02 ACAS:3=0x03

        sdram_itv = int(int(64000000 / 8192) / int(1000 / DRAM_FREQ)) - 2

        self._write_reg(_SDR_REF_ITVL0, sdram_itv)
        self._write_reg(_SDR_REF_ITVL1, sdram_itv >> 8)

        self._write_reg(_SDRCR, 0x01)

    elif type == _W9825G6JH:
        self._write_reg(_SDRAR, 0x31)
        self._write_reg(_SDRMD, 0x03)  # CAS:2=0x02 ACAS:3=0x03

        sdram_itv = int(int(64000000 / 8192) / int(1000 / DRAM_FREQ)) - 2

        self._write_reg(_SDR_REF_ITVL0, sdram_itv)
        self._write_reg(_SDR_REF_ITVL1, sdram_itv >> 8)

        self._write_reg(_SDRCR, 0x01)

    elif type == _W9812G6JH:
        self._write_reg(_SDRAR, 0x29)
        self._write_reg(_SDRMD, 0x03)  # CAS:2=0x02 ACAS:3=0x03

        sdram_itv = int(int(64000000 / 8192) / int(1000 / DRAM_FREQ)) - 2

        self._write_reg(_SDR_REF_ITVL0, sdram_itv)
        self._write_reg(_SDR_REF_ITVL1, sdram_itv >> 8)

        self._write_reg(_SDRCR, 0x01)

    elif type == _MT48LC4M16A:
        self._write_reg(_SDRAR, 0x28)
        self._write_reg(_SDRMD, 0x03)  # CAS:2=0x02 ACAS:3=0x03

        sdram_itv = int(int(64000000 / 4096) / int(1000 / DRAM_FREQ)) - 2

        self._write_reg(_SDR_REF_ITVL0, sdram_itv)
        self._write_reg(_SDR_REF_ITVL1, sdram_itv >> 8)

        self._write_reg(_SDRCR, 0x01)

    elif type == _K4S641632N:
        self._write_reg(_SDRAR, 0x28)
        self._write_reg(_SDRMD, 0x03)  # CAS:2=0x02 ACAS:3=0x03

        sdram_itv = int(int(64000000 / 4096) / int(1000 / DRAM_FREQ)) - 2

        self._write_reg(_SDR_REF_ITVL0, sdram_itv)
        self._write_reg(_SDR_REF_ITVL1, sdram_itv >> 8)

        self._write_reg(_SDRCR, 0x01)

    elif type == _K4S281632K:
        self._write_reg(_SDRAR, 0x29)
        self._write_reg(_SDRMD, 0x03)  # CAS:2=0x02 ACAS:3=0x03

        sdram_itv = int(int(64000000 / 4096) / int(1000 / DRAM_FREQ)) - 2

        self._write_reg(_SDR_REF_ITVL0, sdram_itv)
        self._write_reg(_SDR_REF_ITVL1, sdram_itv >> 8)

        self._write_reg(_SDRCR, 0x01)
    else:
        raise RuntimeError


def init(self):
    time.sleep_ms(100)  # NOQA
    self.reset()
    time.sleep_ms(100)  # NOQA
    self._wait()
        
    self._write_reg(_SRR, 0x01)
    self._wait()

    PLL_Initial(self, self.SCAN_FREQ, self.OSC_FREQ, self.DRAM_FREQ, self.CORE_FREQ)
    
    self._write_reg(_CCR, 0x00)
    time.sleep_ms(1)  # NOQA
    
    self._write_reg(_CCR, 0x80)
    time.sleep_ms(1)  # NOQA
    
    DRAM_Initial(self, self.DRAM_TYPE, self.DRAM_FREQ)

    self._wait()
    time.sleep_ms(1)  # NOQA
    
    lanes = self._data_bus.get_lane_count()

    if lanes == 16:
        self._write_reg(_CCR, 0x01)
    elif lanes == 8:
        self._write_reg(_CCR, 0x00)
    else:
        raise RuntimeError

    self._write_reg(_MACR, 0x40)
    self._write_reg(_ICR, 0x00)

    self._write_reg(_DPCR, (int(self.PCLK_FALLING) << 7) | self._color_byte_order)

    pcsr = (int(self.HSYNC_HIGH_ACTIVE) << 7) | (int(self.VSYNC_HIGH_ACTIVE) << 6)
    pcsr |= (int(self.DE_LOW_ACTIVE) << 5) | (int(self.DE_IDLE_HIGH) << 4)
    pcsr |= (int(self.PCLK_IDLE_HIGH) << 3)
    pcsr |= (int(self.HSYNC_IDLE_HIGH) << 1) | int(self.VSYNC_IDLE_HIGH)
    
    self._write_reg(_PCSR, pcsr)

    self._write_reg(_HDWR, (int(self.display_width / 8) - 1) & 0xFF)
    self._write_reg(_HDWFTR, (self.display_width % 8) & 0xFF)
    
    self._write_reg(_VDHR0, (self.display_height - 1) & 0xFF)
    self._write_reg(_VDHR1, ((self.display_height - 1) >> 8) & 0xFF)

    self._write_reg(_HNDR, int(self.H_BACK_PORCH / 8) - 1)
    self._write_reg(_HNDFTR, self.H_BACK_PORCH % 8)
    self._write_reg(_HSTR, int(self.H_FRONT_PORCH / 8) - 1)
    self._write_reg(_HPWR, int(self.H_PULSE_WIDTH / 8) - 1)

    self._write_reg(_VNDR0, (self.V_BACK_PORCH - 1) & 0xFF)
    self._write_reg(_VNDR1, (self.V_BACK_PORCH - 1) >> 8)
    self._write_reg(_VSTR, self.V_FRONT_PORCH - 1)
    self._write_reg(_VPWR, self.V_PULSE_WIDTH - 1)

    self._write_reg(_MPWCTR, 0x04)

    self._write_reg(_MISA0, 0x00)
    self._write_reg(_MISA1, 0x00)
    self._write_reg(_MISA2, 0x00)
    self._write_reg(_MISA3, 0x00)
    
    self._write_reg(_MIW0, self.display_width & 0xFF)
    self._write_reg(_MIW1, (self.display_width >> 8) & 0xFF)

    self._write_reg(_MWULX0, 0x00)
    self._write_reg(_MWULX1, 0x00)
    self._write_reg(_MWULY0, 0x00)
    self._write_reg(_MWULY1, 0x00)

    self._write_reg(_CVSSA0, 0x00)
    self._write_reg(_CVSSA1, 0x00)
    self._write_reg(_CVSSA2, 0x00)
    self._write_reg(_CVSSA3, 0x00)
    
    self._write_reg(_IMWTH0, self.display_width & 0xFF)
    self._write_reg(_IMWTH1, (self.display_width >> 8) & 0xFF)
        
    self._write_reg(_AWUL_X0, 0x00)
    self._write_reg(_AWUL_X1, 0x00)
    self._write_reg(_AWUL_Y0, 0x00)
    self._write_reg(_AWUL_Y1, 0x00)

    self._write_reg(_AW_WTH0, self.display_width & 0xFF)
    self._write_reg(_AW_WTH1, (self.display_width >> 8) & 0xFF)
    
    self._write_reg(_AW_HT0, self.display_height & 0xFF)
    self._write_reg(_AW_HT1, (self.display_height >> 8) & 0xFF)

    self._write_reg(_AW_COLOR, 0x01)  # check this
    self._write_reg(_MPWCTR, 0x04)  # check this

    self._write_reg(_DPCR, (int(self.PCLK_FALLING) << 7) | self._color_byte_order | 1 << 6)
