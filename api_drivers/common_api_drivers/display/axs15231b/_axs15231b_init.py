# Based on the work by straga (https://github.com/straga)
# https://github.com/straga/micropython_lcd/blob/master/device/JC3248W535/driver/axs15231b/_axs15231b_init_type1.py
# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import time
from micropython import const  # NOQA
import lvgl as lv  # NOQA


_AXS_LCD_NOP = const(0x02000000)  # No operation (C)
_AXS_LCD_SWRESET = const(0x02000100)  # Software reset (C)
_AXS_LCD_RDDID = const(0x02000400)  # Read display (R)
_AXS_LCD_RDNUMED = const(0x02000500)  # Read Number of the Errors on DSI (R)
_AXS_LCD_RDDST = const(0x02000900)  # Read display status (R)
_AXS_LCD_RDDPM = const(0x02000A00)  # Read display power (R)
_AXS_LCD_RDDMADC = const(0x02000B00)  # Read memory data access control (R)
_AXS_LCD_RDDIPF = const(0x02000C00)  # Read Interface Pixel Format (R)
_AXS_LCD_RDDIM = const(0x02000D00)  # Read display image (R)
_AXS_LCD_RDDSM = const(0x02000E00)  # Read display signal (R)
_AXS_LCD_RDDSDR = const(0x02000F00)  # Read display self-diagnostic result (R)
_AXS_LCD_SLPIN = const(0x02001000)  # Sleep in (C)
_AXS_LCD_SLPOUT = const(0x02001100)  # Sleep out (C)
_AXS_LCD_PTLON = const(0x02001200)  # Partial mode on (C)
_AXS_LCD_NORON = const(0x02001300)  # Partial mode off(Normal) (C)
_AXS_LCD_INVOFF = const(0x02002000)  # Display inversion off (C)
_AXS_LCD_INVON = const(0x02002100)  # Display inversion on (C)
_AXS_LCD_ALLPOFF = const(0x02002200)  # All pixel off (C)
_AXS_LCD_ALLPON = const(0x02002300)  # All pixel on (C)
_AXS_LCD_ALLPFILL = const(0x02002400)  # All pixel fill given color (W)
_AXS_LCD_GAMSET = const(0x02002600)  # Gamma curve set (W)
_AXS_LCD_DISPOFF = const(0x02002800)  # Display off (C)
_AXS_LCD_DISPON = const(0x02002900)  # Display on (C)
_AXS_LCD_CASET = const(0x02002A00)  # Column address set (W)
_AXS_LCD_RASET = const(0x02002B00)  # Row address set (W)
_AXS_LCD_RAMWR = const(0x02002C00)  # Memory write any length MIPI/SPI/QSPI/DBI (W)
_AXS_LCD_RAMRD = const(0x02002E00)  # Memory read any length SPI/QSPI/DBI (R)
_AXS_LCD_RAWFILL = const(0x02002F00)  # Memory fill given color at window (W)
_AXS_LCD_PTLAR = const(0x02003000)  # Partial start/end address set (W)
_AXS_LCD_PTLARC = const(0x02003100)  # set_partial_columns (W)
_AXS_LCD_VSCRDEF = const(0x02003300)  # Vertical scrolling definition (W)
_AXS_LCD_TEOFF = const(0x02003400)  # Tearing effect line off (C)
_AXS_LCD_TEON = const(0x02003500)  # Tearing effect line on (W)
_AXS_LCD_MADCTL = const(0x02003600)  # Memory data access control (W)
_AXS_LCD_VSCRSADD = const(0x02003700)  # Vertical scrolling start address (W)
_AXS_LCD_IDMOFF = const(0x02003800)  # Idle mode off (C)
_AXS_LCD_IDMON = const(0x02003900)  # Idle mode on (C)
_AXS_LCD_IPF = const(0x02003A00)  # Interface pixel format (W)
_AXS_LCD_RAMWRC = const(0x02003C00)  # Memory write continue any length MIPI/SPI/QSPI/DBI (W)
_AXS_LCD_RAMRDC = const(0x02003E00)  # Memory read continue any length SPI/QSPI/DBI (R)
_AXS_LCD_TESCAN = const(0x02004400)  # Set tear scanline (W)
_AXS_LCD_RDTESCAN = const(0x02004500)  # Get tear scanline (R)
_AXS_LCD_WRDISBV = const(0x02005100)  # Write display brightness value (W)
_AXS_LCD_RDDISBV = const(0x02005200)  # Read display brightness value (R)
_AXS_LCD_WRCTRLD = const(0x02005300)  # Write CTRL display (W)
_AXS_LCD_RDCTRLD = const(0x02005400)  # Read CTRL dsiplay (R)
_AXS_LCD_RDFCHKSU = const(0x0200AA00)  # Read First Checksum (R)
_AXS_LCD_RDCCHKSU = const(0x0200AA00)  # Read Continue Checksum (R)
_AXS_LCD_RDID1 = const(0x0200DA00)  # Read ID1 (R)
_AXS_LCD_RDID2 = const(0x0200DB00)  # Read ID2 (R)
_AXS_LCD_RDID3 = const(0x0200DC00)  # Read ID3 (R)
_AXS_LCD_DSTB = const(0x02009000)  # Enter Deep-Standby (W)


def init(self):
    param_buf = self._param_buf
    param_mv = self._param_mv

    # Pixel size
    color_size = lv.color_format_get_size(self._color_space)
    if color_size == 2:
        pixel_format = 0x55
    else:
        pixel_format = 0x66

    param_buf[0] = pixel_format
    self.set_params(_AXS_LCD_IPF, param_mv[:1])

    self.set_params(_AXS_LCD_TEOFF)

    # 0xD0 BRIGHTNESS
    param_buf[0] = 0xD0
    self.set_params(_AXS_LCD_WRDISBV, param_mv[:1])

    # # Disable Partial Display Mode (return to Normal Display Mode)
    self.set_params(_AXS_LCD_NORON)
    time.sleep_ms(10)  # NOQA

    #  AXS_LCD_SLPOUT
    self.set_params(_AXS_LCD_SLPOUT)
    time.sleep_ms(150)  # NOQA

    #  AXS_LCD_DISPON
    self.set_params(_AXS_LCD_DISPON)
    time.sleep_ms(150)  # NOQA
