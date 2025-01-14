# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA
import machine  # NOQA

import lvgl as lv
import lcd_bus  # NOQA
import display_driver_framework


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


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR

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


class RA8876(display_driver_framework.DisplayDriver):
    display_name = 'RA8876'
    WAIT_TIMEOUT = 100

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
        rgb565_byte_swap=False,
        wait_pin=None,
        wait_state=STATE_HIGH
    ):

        if not isinstance(data_bus, lcd_bus.I80Bus):
            raise RuntimeError('Only the I8080 bus is supported by the driver')

        if wait_pin is None:
            self._wait_pin = None
        else:
            self._wait_pin = machine.Pin(wait_pin, machine.Pin.IN)
        self._wait_state = wait_state

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
            color_space=lv.COLOR_FORMAT.RGB565,  # NOQA
            rgb565_byte_swap=rgb565_byte_swap
        )

    def set_invert_colors(self, value):
        raise NotImplementedError

    @property
    def orientation(self):
        raise NotImplementedError

    @orientation.setter
    def orientation(self, value):
        raise NotImplementedError

    def _wait(self):
        if self._wait_pin is not None:
            start_time = time.ticks_ms()  # NOQA

            while (
                self._wait_pin.value() != self._wait_state and
                time.ticks_diff(time.ticks_ms(), start_time) < self.WAIT_TIMEOUT  # NOQA
            ):
                time.sleep_ms(1)  # NOQA

    def reset(self):
        if self._reset_pin is None:
            self.set_params(_SRR)
            time.sleep_ms(20)  # NOQA
        else:
            display_driver_framework.DisplayDriver.reset(self)
            time.sleep_ms(50)  # NOQA

    def init( # NOQA
        self,
        dram_ic=W9812G6JH,
        osc_freq=10,  # crystal clock (MHz)
        dram_freq=100,  # SDRAM clock frequency (MHz)
        core_freq=100,  # core (system) clock frequency (MHz)
        scan_freq=50,  # pixel scan clock frequency (MHz)
        hndr=160,  # horizontal non-display period or back porch
        hstr=160,  # horizontal start position or front porch
        hpwr=70,  # HSYNC pulse width
        vndr=23,  # vertical non-display period
        vstr=12,  # vertical start position
        vpwr=10,  # VSYNC pulse width
    ): 
        time.sleep_ms(100)  # NOQA
        self.reset()
        time.sleep_ms(100)  # NOQA
        self._wait()

        buf = self._param_buf
        mv = self._param_mv[:1]
    
        #  perform soft reset
        buf[0] = 0x01
        self.set_params(_SRR, mv)
        self._wait()
    
        # set pixel clock
        if scan_freq >= 63:
            buf[0] = _PLL_DIV_4
            self.set_params(_PPLLC1, mv)
            buf[0] = int(scan_freq * 4 / osc_freq) - 1
            self.set_params(_PPLLC2, mv)
        elif 32 <= scan_freq <= 62:
            buf[0] = _PLL_DIV_8
            self.set_params(_PPLLC1, mv)
            buf[0] = int(scan_freq * 8 / osc_freq) - 1
            self.set_params(_PPLLC2, mv)
        elif 16 <= scan_freq <= 31:
            buf[0] = _PLL_DIV_16
            self.set_params(_PPLLC1, mv)
            buf[0] = int(scan_freq * 16 / osc_freq) - 1
            self.set_params(_PPLLC2, mv)
        elif 8 <= scan_freq <= 15:
            buf[0] = _PLL_DIV_32
            self.set_params(_PPLLC1, mv)
            buf[0] = int(scan_freq * 32 / osc_freq) - 1
            self.set_params(_PPLLC2, mv)
        elif 0 < scan_freq <= 7:
            buf[0] = _PLL_DIV_64
            self.set_params(_PPLLC1, mv)
            buf[0] = int(scan_freq * 64 / osc_freq) - 1
            self.set_params(_PPLLC2, mv)
        else:
            raise RuntimeError(
                'unsupported scan frequency'
            )

        if dram_freq >= 125:
            buf[0] = _PLL_DIV_2
            self.set_params(_MPLLC1, mv)
            buf[0] = int(dram_freq * 2 / osc_freq) - 1
            self.set_params(_MPLLC2, mv)
        elif 63 <= dram_freq <= 124:
            buf[0] = _PLL_DIV_4
            self.set_params(_MPLLC1, mv)
            buf[0] = int(dram_freq * 4 / osc_freq) - 1
            self.set_params(_MPLLC2, mv)
        elif 31 <= dram_freq <= 62:
            buf[0] = _PLL_DIV_8
            self.set_params(_MPLLC1, mv)
            buf[0] = int(dram_freq * 8 / osc_freq) - 1
            self.set_params(_MPLLC2, mv)
        elif dram_freq <= 30:
            buf[0] = _PLL_DIV_8
            self.set_params(_MPLLC1, mv)
            buf[0] = int(30 * 8 / osc_freq) - 1
            self.set_params(_MPLLC2, mv)
        else:
            raise RuntimeError('unsupported dram frequency')

        # set core clock
        if core_freq >= 125:
            buf[0] = _PLL_DIV_2
            self.set_params(_SPLLC1, mv)
            buf[0] = int(core_freq * 2 / osc_freq) - 1
            self.set_params(_SPLLC2, mv)
        elif 63 <= core_freq <= 124:
            buf[0] = _PLL_DIV_4
            self.set_params(_SPLLC1, mv)
            buf[0] = int(core_freq * 4 / osc_freq) - 1
            self.set_params(_SPLLC2, mv)
        elif 31 <= core_freq <= 62:
            buf[0] = _PLL_DIV_8
            self.set_params(_SPLLC1, mv)
            buf[0] = int(core_freq * 8 / osc_freq) - 1
            self.set_params(_SPLLC2, mv)
        elif core_freq <= 30:
            buf[0] = _PLL_DIV_8
            self.set_params(_SPLLC1, mv)
            buf[0] = int(30 * 8 / osc_freq) - 1
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

        # configure SDRAM
        if dram_ic == IS42SM16160D:
            sdrar = 0xF9
        elif dram_ic == IS42S16320B:
            sdrar = 0x32
        elif dram_ic == M12L32162A:
            sdrar = 0x08
        elif dram_ic in (W9825G6JH, M12L2561616A):
            sdrar = 0x31
        elif dram_ic in (W9812G6JH, K4S281632K):
            sdrar = 0x29
        elif dram_ic in (IS42S16400F, MT48LC4M16A, K4S641632N):
            sdrar = 0x28
        else:
            raise RuntimeError('unsupported display')

        if dram_ic in (IS42S16400F, IS42S16320B, IS42SM16160D):
            sdrmd = 0x02
        else:
            sdrmd = 0x03

        if dram_ic in (IS42S16320B, IS42SM16160D, M12L2561616A, W9825G6JH):
            sdram_itv = int(int(64000000 / 8192) / int(1000 / dram_freq)) - 2
        else:
            sdram_itv = int(int(64000000 / 4096) / int(1000 / dram_freq)) - 2

        if dram_ic in (M12L32162A, IS42SM16160D):
            sdrcr = 0x09
        else:
            sdrcr = 0x01

        buf[0] = sdrar
        self.set_params(_SDRAR, mv)

        buf[0] = sdrmd
        self.set_params(_SDRMD, mv)

        buf[0] = sdram_itv
        self.set_params(_SDR_REF_ITVL0, mv)

        buf[0] >>= 8
        self.set_params(_SDR_REF_ITVL1, mv)

        buf[0] = sdrcr
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
            raise RuntimeError(f'Unsupported number of lanes, 8 or 16 is allowed ({lane_count})')
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
        buf[0] = int(hndr / 8) - 1
        self.set_params(_HNDR, mv)
        buf[0] = int(hndr % 8)
        self.set_params(_HNDFTR, mv)
    
        # set horizontal start position / front porch
        buf[0] = int(hstr / 8) - 1
        self.set_params(_HSTR, mv)
    
        # set HSYNC pulse width
        buf[0] = int(hpwr / 8) - 1
        self.set_params(_HPWR, mv)
    
        # set vertical non-display period
        buf[0] = (vndr - 1) & 0xFF
        self.set_params(_VNDR0, mv)
        buf[0] = (vndr - 1) >> 8
        self.set_params(_VNDR1, mv)
    
        # set vertical start position
        buf[0] = vstr - 1
        self.set_params(_VSTR, mv)
    
        # set VSYNC pulse width
        buf[0] = vpwr - 1
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

        display_driver_framework.DisplayDriver.init(self)

    def _set_memory_location(self, x_start, y_start, x_end, y_end):

        print(x_start, y_start, x_end, y_end)
        buf = self._param_buf
        mv = self._param_mv[:1]

        # set active window start X/Y
        buf[0] = x_start & 0xFF
        self.set_params(_AWUL_X0, mv)
        buf[0] = (x_start >> 8) & 0xFF
        self.set_params(_AWUL_X1, mv)
        buf[0] = y_start & 0xFF
        self.set_params(_AWUL_Y0, mv)
        buf[0] = (y_start >> 8) & 0xFF
        self.set_params(_AWUL_Y1, mv)

        # set active window width and height
        buf[0] = (x_end - x_start) & 0xFF
        self.set_params(_AW_WTH0, mv)
        buf[0] = ((x_end - x_start) >> 8) & 0xFF
        self.set_params(_AW_WTH1, mv)
        buf[0] = (y_end - y_start) & 0xFF
        self.set_params(_AW_HT0, mv)
        buf[0] = ((y_end - y_start) >> 8) & 0xFF
        self.set_params(_AW_HT1, mv)

        # set cursor
        buf[0] = x_start & 0xff
        self.set_params(_CURH0, mv)
        buf[0] = (x_start >> 8) & 0xFF
        self.set_params(_CURH1, mv)
        buf[0] = y_start & 0xFF
        self.set_params(_CURV0, mv)
        buf[0] = (y_start >> 8) & 0xFF
        self.set_params(_CURV1, mv)

        return _MRWDP
