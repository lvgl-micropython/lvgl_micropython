from micropython import const  # NOQA


_PLL_DIV_2 = const(0x02)  # PLL divided by 2
_PLL_DIV_4 = const(0x04)  # PLL divided by 4
_PLL_DIV_8 = const(0x06)  # PLL divided by 8
_PLL_DIV_16 = const(0x16)  # PLL divided by 16
_PLL_DIV_32 = const(0x26)  # PLL divided by 32
_PLL_DIV_64 = const(0x36)  # PLL divided by 64


# Horizontal Display Width Register (HDWR)
_HDWR = const(0x14)
# Horizontal Display Width Fine Tune Register (HDWFTR)
_HDWFTR = const(0x15)

# Horizontal Non-Display Period Register (HNDR)
_HNDR = const(0x16)
# Horizontal Non-Display Period Fine Tune Register (HNDFTR)
_HNDFTR = const(0x17)

# HSYNC Start Position Register (HSTR)
_HSTR = const(0x18)

# HSYNC Pulse Width Register (HPWR)
_HPWR = const(0x19)

# Vertical Display Height Register 0 (VDHR0)
_VDHR0 = const(0x1A)
# Vertical Display Height Register 1 (VDHR1)
_VDHR1 = const(0x1B)

# Vertical Non-Display Period Register 0 (VNDR0)
_VNDR0 = const(0x1C)
# Vertical Non-Display Period Register 1 (VNDR1)
_VNDR1 = const(0x1D)

# VSYNC Start Position Register (VSTR)
_VSTR = const(0x1E)

# VSYNC Pulse Width Register (VPWR)
_VPWR = const(0x1F)

# Main Image Start Address 0 (MISA0)
_MISA0 = const(0x20)
# Main Image Start Address 1 (MISA1)
_MISA1 = const(0x21)
# Main Image Start Address 2 (MISA2)
_MISA2 = const(0x22)
# Main Image Start Address 3 (MISA3)
_MISA3 = const(0x23)

# Main Image Width 0 (MIW0)
_MIW0 = const(0x24)
# Main Image Width 1 (MIW1)
_MIW1 = const(0x25)

# Main Window Upper-Left corner X-coordinates 0 (MWULX0)
_MWULX0 = const(0x26)
# Main Window Upper-Left corner X-coordinates 1 (MWULX1)
_MWULX1 = const(0x27)
# Main Window Upper-Left corner Y-coordinates 0 (MWULY0)
_MWULY0 = const(0x28)
# Main Window Upper-Left corner Y-coordinates 1 (MWULY1)
_MWULY1 = const(0x29)

# Canvas Start address 0 (CVSSA0)
_CVSSA0 = const(0x50)
# Canvas Start address 1 (CVSSA1)
_CVSSA1 = const(0x51)
# Canvas Start address 2 (CVSSA2)
_CVSSA2 = const(0x52)
# Canvas Start address 3 (CVSSA3)
_CVSSA3 = const(0x53)

# Canvas image width 0 (CVS_IMWTH0)
_CVS_IMWTH0 = const(0x54)
# Canvas image width 1 (CVS_IMWTH1)
_CVS_IMWTH1 = const(0x55)

# Active Window Upper-Left corner X-coordinates 0 (AWUL_X0)
_AWUL_X0 = const(0x56)
# Active Window Upper-Left corner X-coordinates 1 (AWUL_X1)
_AWUL_X1 = const(0x57)
# Active Window Upper-Left corner Y-coordinates 0 (AWUL_Y0)
_AWUL_Y0 = const(0x58)
# Active Window Upper-Left corner Y-coordinates 1 (AWUL_Y1)
_AWUL_Y1 = const(0x59)

# Active Window Width 0 (AW_WTH0)
_AW_WTH0 = const(0x5A)
# Active Window Width 1 (AW_WTH1)
_AW_WTH1 = const(0x5B)

# Active Window Height 0 (AW_HT0)
_AW_HT0 = const(0x5C)
# Active Window Height 1 (AW_HT1)
_AW_HT1 = const(0x5D)


# pixel clock
# SCLK PLL Control Register 1 (PPLLC1)
_PPLLC1 = const(0x05)
# SCLK PLL Control Register 2 (PPLLC2)
_PPLLC2 = const(0x06)

# memory clock
# MCLK PLL Control Register 1 (MPLLC1)
_MPLLC1 = const(0x07)
# MCLK PLL Control Register 2 (MPLLC2)
_MPLLC2 = const(0x08)

# core clock
# CCLK PLL Control Register 1 (SPLLC1)
_SPLLC1 = const(0x09)
# CCLK PLL Control Register 2 (SPLLC2)
_SPLLC2 = const(0x0A)

# ram type
# SDRAM attribute register (SDRAR)
_SDRAR = const(0xE0)
# SDRAM mode register & extended mode register (SDRMD)
_SDRMD = const(0xE1)
# SDRAM auto refresh interval (SDR_REG_ITVL0)
_SDR_REF_ITVL0 = const(0xE2)
# SDRAM auto refresh interval (SDR_REG_ITVL1)
_SDR_REF_ITVL1 = const(0xE3)
# SDRAM Control register (SDRCR)
_SDRCR = const(0xE4)


IS42SM16160D = 0x00  # integrated silicon solution dram IC
IS42S16320B = 0x01  # integrated silicon solution dram IC
IS42S16400F = 0x02  # integrated silicon solution dram IC
M12L32162A = 0x03  # elite semiconductor dram IC
M12L2561616A = 0x04  # elite semiconductor dram IC
W9825G6JH = 0x05  # winbond dram IC
W9812G6JH = 0x06  # winbond dram IC
MT48LC4M16A = 0x07  # micron dram IC
K4S641632N = 0x08  # samsung dram IC
K4S281632K = 0x09  # samsung dram IC
M12L64164A = 0x0A


def set_core_clock(self, core_freq, osc_freq):
    #  Set Core clock
    if core_freq >= 158:
        self.set_params(_SPLLC1, _PLL_DIV_2)  # PLL Divided by 2
        self.set_params(_SPLLC2, int(core_freq * 2 / osc_freq) - 1)
    elif 125 <= core_freq <= 157:
        self.set_params(_SPLLC1, 0x03)  # PLL Divided by 4
        self.set_params(_SPLLC2, int(core_freq * 4 / osc_freq) - 1)
    elif 79 <= core_freq <= 124:
        self.set_params(_SPLLC1, _PLL_DIV_4)  # PLL Divided by 4
        self.set_params(_SPLLC2, int(core_freq * 4 / osc_freq) - 1)
    elif 63 <= core_freq <= 78:
        self.set_params(_SPLLC1, 0x05)  # PLL Divided by 8
        self.set_params(_SPLLC2, int(core_freq * 8 / osc_freq) - 1)
    elif 40 <= core_freq <= 62:
        self.set_params(_SPLLC1, _PLL_DIV_8)  # PLL Divided by 8
        self.set_params(_SPLLC2, int(core_freq * 8 / osc_freq) - 1)
    elif 32 <= core_freq <= 39:
        self.set_params(_SPLLC1, _PLL_DIV_8)  # PLL Divided by 8
        self.set_params(0x0A, int(core_freq * 8 / osc_freq) - 1)
    elif core_freq <= 31:
        # set to 30MHz if out off range
        self.set_params(_SPLLC1, _PLL_DIV_8)  # PLL Divided by 8
        self.set_params(_SPLLC2, int(30 * 8 / osc_freq) - 1)


def set_pixel_clock(self, scan_freq, osc_freq):
    # Set tft output pixel clock
    if scan_freq >= 79:
        self.set_params(_PPLLC1, 0x04)  # PLL Divided by 4
        self.set_params(_PPLLC2, int(scan_freq * 4 / osc_freq) - 1)
    elif 63 <= scan_freq <= 78:
        self.set_params(_PPLLC1, 0x05)  # PLL Divided by 4
        self.set_params(_PPLLC2, int(scan_freq * 8 / osc_freq) - 1)
    elif 40 <= scan_freq <= 62:
        self.set_params(_PPLLC1, _PLL_DIV_8)  # PLL Divided by 8
        self.set_params(_PPLLC2, int(scan_freq * 8 / osc_freq) - 1)
    elif 32 <= scan_freq <= 39:
        self.set_params(_PPLLC1, 0x07)  # PLL Divided by 8
        self.set_params(_PPLLC2, int(scan_freq * 16 / osc_freq) - 1)
    elif 16 <= scan_freq <= 31:
        self.set_params(_PPLLC1, _PLL_DIV_16)  # PLL Divided by 16
        self.set_params(_PPLLC2, int(scan_freq * 16 / osc_freq) - 1)
    elif 8 <= scan_freq <= 15:
        self.set_params(_PPLLC1, _PLL_DIV_32)  # PLL Divided by 32
        self.set_params(_PPLLC2, int(scan_freq * 32 / osc_freq) - 1)
    elif 0 < scan_freq <= 7:
        self.set_params(_PPLLC1, _PLL_DIV_64)  # PLL Divided by 64
        self.set_params(_PPLLC2, int(scan_freq * 64 / osc_freq) - 1)


def set_ram_clock(self, dram_freq, osc_freq):
    # Set internal Buffer Ram clock
    if dram_freq >= 158:
        self.set_params(_MPLLC1, 0x02)  # PLL Divided by 2
        self.set_params(_MPLLC2, int(dram_freq * 2 / osc_freq) - 1)
    elif 125 <= dram_freq <= 157:
        self.set_params(_MPLLC1, 0x03)  # PLL Divided by 3
        self.set_params(_MPLLC2, int(dram_freq * 4 / osc_freq) - 1)
    elif 79 <= dram_freq <= 124:
        self.set_params(_MPLLC1, _PLL_DIV_4)  # PLL Divided by 4
        self.set_params(_MPLLC2, int(dram_freq * 4 / osc_freq) - 1)
    elif 63 <= dram_freq <= 78:
        self.set_params(_MPLLC1, 0x05)  # PLL Divided by 5
        self.set_params(_MPLLC2, int(dram_freq * 8 / osc_freq) - 1)
    elif 40 <= dram_freq <= 62:
        self.set_params(_MPLLC1, _PLL_DIV_8)  # PLL Divided by 8
        self.set_params(_MPLLC2, int(dram_freq * 8 / osc_freq) - 1)
    elif 32 <= dram_freq <= 39:
        self.set_params(_MPLLC1, 0x07)  # PLL Divided by 16
        self.set_params(_MPLLC2, int(dram_freq * 16 / osc_freq) - 1)
    elif dram_freq <= 31:
        # set to 30MHz if out off range
        self.set_params(_MPLLC1, _PLL_DIV_8)  # PLL Divided by 8
        self.set_params(_MPLLC2, int(30 * 8 / osc_freq) - 1)


def set_ram_type(self, type, dram_freq):  # NOQA
    if type in (IS42SM16160D, IS42S16320B, M12L32162A, M12L64164A):
        if type in (IS42SM16160D, IS42S16320B):
            Auto_Refresh = int((64 * dram_freq * 1000) / 8192) - 2

            if type == IS42SM16160D:
                self.set_params(_SDRAR, 0xf9)

            elif type == IS42S16320B:
                self.set_params(_SDRAR, 0x32)

            if dram_freq <= 133:
                self.set_params(_SDRMD, 2)  # CAS:2=0x02，CAS:3=0x03
            else:
                self.set_params(_SDRMD, 3)  # CAS:2=0x02，CAS:3=0x03

        elif type in (M12L32162A, M12L64164A):
            Auto_Refresh = int((64 * dram_freq * 1000) / 4096) - 2

            if type == M12L32162A:
                self.set_params(_SDRAR, 0x08)

            elif type == M12L64164A:
                self.set_params(_SDRAR, 0x28)

            self.set_params(_SDRMD, 3)  # CAS:2=0x02，CAS:3=0x03
        else:
            raise RuntimeError('Invalid memory type')

        self.set_params(_SDR_REF_ITVL0, Auto_Refresh)
        self.set_params(_SDR_REF_ITVL1, Auto_Refresh >> 8)
        self.set_params(_SDRCR, 0x09)

    elif type in (
        IS42S16400F, M12L2561616A, W9825G6JH, W9812G6JH,
        MT48LC4M16A, K4S641632N, K4S281632K
    ):
        if type in (
            IS42S16400F, W9825G6JH, W9812G6JH,
            MT48LC4M16A, K4S641632N, K4S281632K
        ):
            Auto_Refresh = int((64 * dram_freq * 1000) / 4096) - 2

            if type in (IS42S16400F, MT48LC4M16A, K4S641632N):
                self.set_params(_SDRAR, 0x28)
            elif type in (W9812G6JH, K4S281632K):
                self.set_params(_SDRAR, 0x29)
            elif type in (W9825G6JH,):
                self.set_params(_SDRAR, 0x31)

            if type in (
                MT48LC4M16A, K4S641632N, W9825G6JH, W9812G6JH, K4S281632K
            ):
                self.set_params(_SDRMD, 3)  # CAS:2=0x02，CAS:3=0x03
            elif type in (IS42S16400F,):
                if dram_freq < 143:
                    self.set_params(_SDRMD, 2)  # CAS:2=0x02，CAS:3=0x03
                else:
                    self.set_params(_SDRMD, 3)  # CAS:2=0x02，CAS:3=0x03

        elif type in (M12L2561616A,):
            Auto_Refresh = int((64 * dram_freq * 1000) / 8192) - 2
            self.set_params(_SDRAR, 0x31)
            self.set_params(_SDRMD, 3)  # CAS:2=0x02，CAS:3=0x03

        else:
            raise RuntimeError('Invalid memory type')

        self.set_params(_SDR_REF_ITVL0, Auto_Refresh)
        self.set_params(_SDR_REF_ITVL1, Auto_Refresh >> 8)
        self.set_params(_SDRCR, 0x01)


def set_active_window_size(self, width, height):
    self.set_params(_AW_WTH0, width & 0xFF)  # 5ah
    self.set_params(_AW_WTH1, (width >> 8) & 0xFF)  # 5bh
    self.set_params(_AW_HT0, height & 0xFF)  # 5ch
    self.set_params(_AW_HT1, (height >> 8) & 0xFF)  # 5dh


def set_active_window_pos(self, x, y):
    self.set_params(_AWUL_X0, x & 0xFF)  # 56h
    self.set_params(_AWUL_X1, (x >> 8) & 0xFF)  # 57h
    self.set_params(_AWUL_Y0, y & 0xFF)  # 58h
    self.set_params(_AWUL_Y1, (y >> 8) & 0xFF)  # 59h


def set_canvas_image_width(self, width):
    self.set_params(_CVS_IMWTH0, width & 0xFF)  # 54h
    self.set_params(_CVS_IMWTH1, (width >> 8) & 0xFF)  # 55h


def set_canvas_image_start_address(self, addr):
    self.set_params(_CVSSA0, addr & 0xFF)  # 50h
    self.set_params(_CVSSA1, (addr >> 8) & 0xFF)  # 51h
    self.set_params(_CVSSA2, (addr >> 16) & 0xFF)  # 52h
    self.set_params(_CVSSA3, (addr >> 24) & 0xFF)  # 53h


def set_display_window_start_pos(self, x, y):
    self.set_params(_MWULX0, x & 0xFF)  # 26h
    self.set_params(_MWULX1, (x >> 8) & 0xFF)  # 27h
    self.set_params(_MWULY0, y & 0xFF)  # 28h
    self.set_params(_MWULY1, (y >> 8) & 0xFF)  # 29h


def set_display_image_width(self, width):
    self.set_params(_MIW0, width & 0xFF)  # 24h
    self.set_params(_MIW1, (width >> 8) & 0xFF)  # 25h


def set_display_image_start_address(self, addr):
    self.set_params(_MISA0, addr & 0xFF)  # 20h
    self.set_params(_MISA1, (addr >> 8) & 0xFF)  # 21h
    self.set_params(_MISA2, (addr >> 16) & 0xFF)  # 22h
    self.set_params(_MISA3, (addr >> 24) & 0xFF)  # 23h


def set_vsync_pulse_width(self, pulse_width):
    self.set_params(_VPWR, (pulse_width - 1) & 0xFF)


def set_vsync_front_porch(self, front_porch):
    self.set_params(_VSTR, (front_porch - 1) & 0xFF)


def set_vsync_back_porch(self, back_porch):
    self.set_params(_VNDR0, (back_porch - 1) & 0xFF)
    self.set_params(_VNDR1, ((back_porch - 1) >> 8) & 0xFF)


def set_hsync_pulse_width(self, pulse_width):
    if pulse_width < 8:
        self.set_params(_HPWR, 0x00)
    else:
        self.set_params(_HPWR, ((pulse_width / 8) - 1) & 0xFF)


def set_hsync_front_porch(self, front_porch):
    if front_porch < 8:
        self.set_params(_HSTR, 0x00)
    else:
        self.set_params(_HSTR, ((front_porch / 8) - 1) & 0xFF)


def set_hsync_back_porch(self, back_porch):
    if back_porch < 8:
        self.set_params(_HNDR, 0x00)
        self.set_params(_HNDFTR, (back_porch & 0xFF))
    else:
        self.set_params(_HNDR, ((back_porch / 8) - 1) & 0xFF)
        self.set_params(_HNDFTR, (back_porch % 8) & 0xFF)


def set_display_size(self, width, height):
    self.set_params(_HDWR, ((width / 8) - 1) & 0xFF)
    self.set_params(_HDWFTR, (width % 8) & 0xFF)
    self.set_params(_VDHR0, (height - 1) & 0xFF)
    self.set_params(_VDHR1, ((height - 1) >> 8) & 0xFF)
