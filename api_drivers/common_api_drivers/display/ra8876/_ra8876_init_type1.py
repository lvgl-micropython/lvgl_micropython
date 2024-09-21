from micropython import const  # NOQA
import time
import sys


_OSC_CLOCK = const(10)
_RAM_CLOCK = const(100)
_CORE_CLOCK = const(100)
_PIXEL_CLOCK = const(50)
_HSYNC_BACK_PORCH = const(160)
_HSYNC_FRONT_PORCH = const(160)
_HSYNC_PULSE_WIDTH = const(70)
_VSYNC_BACK_PORCH = const(23)
_VSYNC_FRONT_PORCH = const(12)
_VSYNC_PULSE_WIDTH = const(10)


# Chip Configuration Register (CRR)
_CCR = const(0x01)
# =================================

# 0: disable
# 1: enable
# _PLL = const(0)
_PLL = const(1)

# 0: off
# 1: on
_WAIT_MASK = const(0)
# _WAIT_MASK = const(1)

# 0: disable
# 1: enable
_KEY_SCAN = const(0)
# _KEY_SCAN = const(1)

# 0: output 24
# 1: output 18
# 3: output 16
_TFT_OUTPUT = const(0)
# _TFT_OUTPUT = const(1)
# _TFT_OUTPUT = const(3)

# 0: disable
# 1: enable
_I2C_MASTER = const(0)
# _I2C_MASTER = const(1)

# 0: disable
# 1: enable
# _SERIAL_IF = const(0)
_SERIAL_IF = const(1)

# 0: 8bit
# 1: 16bit
_HOST_DATA_BUS = const(0)
# _HOST_DATA_BUS = const(1)
# =================================


# Memory Access Control Register (MACR)
_MACR = const(0x02)
# =================================

# for all
# 8 bits MPU I/F
# 16 bits MPU I/F with 8bpp data mode 1 & 2
# 16 bits MPU I/F with 16/24-bpp data mode 1 & serial host interface
_DIRECT_WRITE = const(0)

# 16 bit MPU I/F with 8-bpp data mode 1
_MASK_HIGH_BYTE = const(2)
# # 16 bit MPU I/F with 24-bpp data mode 2
# _MASK_HIGH_BYTE_EVEN_DATA = const(3)

# 0: left, right, top, bottom
# 1: right, left, top, bottom
# 2: top, bottom, left, right
# 3: bottom, top, left, right
_READ_MEMORY = const(0)
# _READ_MEMORY = const(1)
# _READ_MEMORY = const(2)
# _READ_MEMORY = const(3)

# 0: left, right, top, bottom
# 1: right, left, top, bottom
# 2: top, bottom, left, right
# 3: bottom, top, left, right
_WRITE_MEMORY = const(0)
# _WRITE_MEMORY = const(1)
# _WRITE_MEMORY = const(2)
# _WRITE_MEMORY = const(3)
# =================================


# Input Control Register (ICR)
_ICR = const(0x03)
# =================================

# 0: VESA format
# 1: JEIDA format
_LVDS_FORMAT = const(0)

# 0: low
# 1: high
# _OUTPUT_INT_LEVEL = const(0)
# _OUTPUT_INT_LEVEL = const(1)

# 0: off
# 1: on
# _EXT_INT_DEBOUNCE = const(0)
# _EXT_INT_DEBOUNCE = const(1)

# 0: low level
# 1: falling edge
# 2: high level
# 3: rising edge
# _EXT_INT_TRIGGER = const(0)
# _EXT_INT_TRIGGER = const(1)
# _EXT_INT_TRIGGER = const(2)
# _EXT_INT_TRIGGER = const(3)

# 0: graphic mode
# 1: text mode
_OPP_MODE = const(0)
# _OPP_MODE = const(1)


# 0: image
# 1: gamma
# 2: cursor ram
# 3: pallett ram
_MEMORY_SELECT = const(0)
# _MEMORY_SELECT = const(1)
# _MEMORY_SELECT = const(2)
# _MEMORY_SELECT = const(3)
# =================================


# Main/PIP Window Control Register (MPWCTR)
_MPWCTR = const(0x10)
# =================================

# 0: disabled
# 1: enabled
_PIP1_WINDOW = const(0)
# _PIP1_WINDOW = const(1)

# 0: disabled
# 1: enabled
_PIP2_WINDOW = const(0)
# _PIP2_WINDOW = const(1)

# 0: PIP1
# 1: PIP2
_SELECT_CONFIG_PIP = const(0)
# _SELECT_CONFIG_PIP = const(1)

# 0: 8BPP
# 1: 16BPP
# 2: 24BPP
# _IMAGE_COLOR_DEPTH = const(0)
_IMAGE_COLOR_DEPTH = const(1)
# _IMAGE_COLOR_DEPTH = const(2)

# 0: SYNC_mode(SYNC+DE mode)
# 1: DE mode
# if sync only mode do not connect DE signal or XDE_INV = 1

_TFT_MODE = const(0)
# _TFT_MODE = const(1)
# =================================


_PIPCDEP = const(0x11)
# =================================

# 0: 8bpp
# 1: 16bpp
# 2: 24bpp
# _PIP1_COLOR_DEPTH = const(0)
_PIP1_COLOR_DEPTH = const(1)
# _PIP1_COLOR_DEPTH = const(2)

# 0: 8bpp
# 1: 16bpp
# 2: 24bpp
# _PIP2_COLOR_DEPTH = const(0)
_PIP2_COLOR_DEPTH = const(1)
# _PIP2_COLOR_DEPTH = const(2)
# =================================


# Color Depth of Canvas & Active Window (AW_COLOR)
_AW_COLOR = const(0x5E)
# =================================

# 0: block mode
# 1: linear mode
_CANVAS_MODE = const(0)
# _CANVAS_MODE = const(1)

# 0: 8bpp
# 1: 16bpp
# 2: 24bpp
# _CANVAS_COLOR_DEPTH = const(0)
_CANVAS_COLOR_DEPTH = const(1)
# _CANVAS_COLOR_DEPTH = const(2)
# =================================


_BTE_COLOR = const(0x92)
# =================================

# 0: 8bpp
# 1: 16bpp
# 2: 24bpp
# _S0_COLOR_DEPTH = const(0)
_S0_COLOR_DEPTH = const(1)
# _S0_COLOR_DEPTH = const(2)

# 0: 8bpp
# 1: 16bpp
# 2: 24bpp
# _S1_COLOR_DEPTH = const(0)
_S1_COLOR_DEPTH = const(1)
# _S1_COLOR_DEPTH = const(2)

# 3: constant
# 4: 8bit pixel alpha blending
# 5: 16 bit pixel alpha blending
# _S1_COLOR = const(3)
# _S1_COLOR = const(4)
# _S1_COLOR = const(5)

# 0: 8bpp
# 1: 16bpp
# 2: 24bpp
# _DESTINATION_COLOR_DEPTH = const(0)
# _DESTINATION_COLOR_DEPTH = const(1)
# _DESTINATION_COLOR_DEPTH = const(2)
# =================================


# Display Configuration Register (DPCR)
_DPCR = const(0x12)
# =================================

# 0: off
# 1: on
# _PCLK_INVERSION = const(0)
_PCLK_INVERSION = const(1)

# 0: off
# 1: on
_DISPLAY_STATE = const(0)
# _DISPLAY_STATE = const(1)

# 0: disable
# 1: enable
# _COLOR_BAR = const(0)
# _COLOR_BAR = const(1)

_VDIR_TB = const(0)
_VDIR_BT = const(1)

# 0: RGB
# 1: RBG
# 2: GRB
# 3: GBR
# 4: BRG
# 5: BGR
# 6: GRAY
# 7: IDLE
_OUTPUT_ORDER = const(0)
# _OUTPUT_ORDER = const(1)
# _OUTPUT_ORDER = const(2)
# _OUTPUT_ORDER = const(3)
# _OUTPUT_ORDER = const(4)
# _OUTPUT_ORDER = const(5)
# _OUTPUT_ORDER = const(6)
# _OUTPUT_ORDER = const(7)
# =================================


# Panel scan Clock and Data Setting Register (PCSR)
_PCSR = const(0x13)
# =================================

# 0: low
# 1: high
_XHSYNC_ACTIVE = const(0)
# _XHSYNC_ACTIVE = const(1)

# 0: low
# 1: high
_XVSYNC_ACTIVE = const(0)
# _XVSYNC_ACTIVE = const(1)

# 0: high
# 1: low
_XDE_ACTIVE = const(0)
# _XDE_ACTIVE = const(1)

# 0: low
# 1: high
# _XDE_IDLE = const(0)
# _XDE_IDLE = const(1)

# 0: low
# 1: high
# _XPCLK_IDLE = const(0)
# _XPCLK_IDLE = const(1)

# 0: low
# 1: high
# _XPDAT_IDLE = const(0)
# _XPDAT_IDLE = const(1)

# 0: low
# 1: high
# _XHSYNC_IDLE = const(0)
# _XHSYNC_IDLE = const(1)

# 0: low
# 1: high
# _XVSYNC_IDLE = const(0)
# _XVSYNC_IDLE = const(1)
# =================================


def init(self):
    import ra8876_init

    # disable PLL
    self.set_params(0x01, 0x08)
    time.sleep_ms(1)  # NOQA

    ra8876_init.set_pixel_clock(self, _PIXEL_CLOCK, _OSC_CLOCK)
    ra8876_init.set_ram_clock(self, _RAM_CLOCK, _OSC_CLOCK)
    ra8876_init.set_core_clock(self, _CORE_CLOCK, _OSC_CLOCK)

    time.sleep_ms(1)  # NOQA
    self.set_params(0x01, 0x80)
    # wait for pll stable
    time.sleep_ms(2) # NOQA

    ra8876_init.set_ram_type(self, ra8876_init.W9812G6JH, _RAM_CLOCK)

    self.set_params(
        _CCR,
        (_PLL << 7) | (_WAIT_MASK << 6) | (_KEY_SCAN << 5) |
        (_TFT_OUTPUT << 3) | (_I2C_MASTER << 2) | (_SERIAL_IF << 1) |
        _HOST_DATA_BUS
    )

    self.set_params(
        _MACR,
        (_DIRECT_WRITE << 6) | (_READ_MEMORY << 4) | (_WRITE_MEMORY << 1)
    )

    self.set_params(
        _ICR,
        (_LVDS_FORMAT << 3) | (_OPP_MODE << 2) | _MEMORY_SELECT
    )

    self.set_params(
        _MPWCTR,
        (_PIP1_WINDOW << 7) | (_PIP2_WINDOW << 6) | (_SELECT_CONFIG_PIP << 4) |
        (_IMAGE_COLOR_DEPTH << 2) | _TFT_MODE
    )

    self.set_params(_PIPCDEP, (_PIP1_COLOR_DEPTH << 2) | _PIP2_COLOR_DEPTH)
    self.set_params(_AW_COLOR, (_CANVAS_MODE << 2) | _CANVAS_COLOR_DEPTH)
    self.set_params(
        _BTE_COLOR,
        (_S0_COLOR_DEPTH << 5) | (_S1_COLOR_DEPTH << 2) | _S0_COLOR_DEPTH
    )

    # TFT timing configure
    self.set_params(
        _DPCR,
        (_PCLK_INVERSION << 7) | (_DISPLAY_STATE << 6) | _OUTPUT_ORDER
    )

    self.set_params(
        _PCSR,
        (_XHSYNC_ACTIVE << 7) | (_XVSYNC_ACTIVE << 6) | (_XDE_ACTIVE << 5)
    )

    ra8876_init.set_display_size(self, self.display_width, self.display_height)

    ra8876_init.set_hsync_back_porch(self, _HSYNC_BACK_PORCH)
    ra8876_init.set_hsync_front_porch(self, _HSYNC_FRONT_PORCH)
    ra8876_init.set_hsync_pulse_width(self, _HSYNC_PULSE_WIDTH)

    ra8876_init.set_vsync_back_porch(self, _VSYNC_BACK_PORCH)
    ra8876_init.set_vsync_front_porch(self, _VSYNC_FRONT_PORCH)
    ra8876_init.set_vsync_pulse_width(self, _VSYNC_PULSE_WIDTH)

    # image buffer configure
    ra8876_init.set_display_image_start_address(self, 0)
    ra8876_init.set_display_image_width(self, self.display_width)
    ra8876_init.set_display_window_start_pos(self, 0, 0)
    ra8876_init.set_canvas_image_start_address(self, 0)
    ra8876_init.set_canvas_image_width(self, self.display_width)
    ra8876_init.set_active_window_pos(self, 0, 0)

    ra8876_init.set_active_window_size(
        self,
        self.display_width,
        self.display_height
    )

    del sys.modules['ra8876_init']
