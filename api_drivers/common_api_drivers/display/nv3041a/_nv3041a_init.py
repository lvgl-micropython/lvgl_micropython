import time
from micropython import const  # NOQA
import lvgl as lv  # NOQA

AXS_LCD_NOP      = 0x00  # No operation (C)
AXS_LCD_SWRESET  = 0x01  # Software reset (C)
AXS_LCD_RDDID    = 0x04  # Read display (R)
AXS_LCD_RDNUMED  = 0x05  # Read Number of the Errors on DSI (R)
AXS_LCD_RDDST    = 0x09  # Read display status (R)
AXS_LCD_RDDPM    = 0x0A  # Read display power (R)
AXS_LCD_RDDMADC  = 0x0B  # Read memory data access control (R)
AXS_LCD_RDDIPF   = 0x0C  # Read Interface Pixel Format (R)
AXS_LCD_RDDIM    = 0x0D  # Read display image (R)
AXS_LCD_RDDSM    = 0x0E  # Read display signal (R)
AXS_LCD_RDDSDR   = 0x0F  # Read display self-diagnostic result (R)
AXS_LCD_SLPIN    = 0x10  # Sleep in (C)
AXS_LCD_SLPOUT   = 0x11  # Sleep out (C)
AXS_LCD_PTLON    = 0x12  # Partial mode on (C)
AXS_LCD_NORON    = 0x13  # Partial mode off(Normal) (C)
AXS_LCD_INVOFF   = 0x20  # Display inversion off (C)
AXS_LCD_INVON    = 0x21  # Display inversion on (C)
AXS_LCD_ALLPOFF  = 0x22  # All pixel off (C)
AXS_LCD_ALLPON   = 0x23  # All pixel on (C)
AXS_LCD_ALLPFILL = 0x24  # All pixel fill given color (W)
AXS_LCD_GAMSET   = 0x26  # Gamma curve set (W)
AXS_LCD_DISPOFF  = 0x28  # Display off (C)
AXS_LCD_DISPON   = 0x29  # Display on (C)
AXS_LCD_CASET    = 0x2A  # Column address set (W)
AXS_LCD_RASET    = 0x2B  # Row address set (W)
AXS_LCD_RAMWR    = 0x2C  # Memory write any length MIPI/SPI/QSPI/DBI (W)
AXS_LCD_RAMRD    = 0x2E  # Memory read any length SPI/QSPI/DBI (R)
AXS_LCD_RAWFILL  = 0x2F  # Memory fill given color at window (W)
AXS_LCD_PTLAR    = 0x30  # Partial start/end address set (W)
AXS_LCD_PTLARC   = 0x31  # set_partial_columns (W)
AXS_LCD_VSCRDEF  = 0x33  # Vertical scrolling definition (W)
AXS_LCD_TEOFF    = 0x34  # Tearing effect line off (C)
AXS_LCD_TEON     = 0x35  # Tearing effect line on (W)
AXS_LCD_MADCTL   = 0x36  # Memory data access control (W)
AXS_LCD_VSCRSADD = 0x37  # Vertical scrolling start address (W)
AXS_LCD_IDMOFF   = 0x38  # Idle mode off (C)
AXS_LCD_IDMON    = 0x39  # Idle mode on (C)
AXS_LCD_IPF      = 0x3A  # Interface pixel format (W)
AXS_LCD_RAMWRC   = 0x3C  # Memory write continue any length MIPI/SPI/QSPI/DBI (W)
AXS_LCD_RAMRDC   = 0x3E  # Memory read continue any length SPI/QSPI/DBI (R)
AXS_LCD_TESCAN   = 0x44  # Set tear scanline (W)
AXS_LCD_RDTESCAN = 0x45  # Get tear scanline (R)
AXS_LCD_WRDISBV  = 0x51  # Write display brightness value (W)
AXS_LCD_RDDISBV  = 0x52  # Read display brightness value (R)
AXS_LCD_WRCTRLD  = 0x53  # Write CTRL display (W)
AXS_LCD_RDCTRLD  = 0x54  # Read CTRL dsiplay (R)
AXS_LCD_RDFCHKSU = 0xAA  # Read First Checksum (R)
AXS_LCD_RDCCHKSU = 0xAA  # Read Continue Checksum (R)
AXS_LCD_RDID1    = 0xDA  # Read ID1 (R)
AXS_LCD_RDID2    = 0xDB  # Read ID2 (R)
AXS_LCD_RDID3    = 0xDC  # Read ID3 (R)
AXS_LCD_DSTB     = 0x90  # Enter Deep-Standby (W)


_NV3041A_INIT_CMDS = const((
    (0xff,0xa5,0),
    (0xE7,0x10,0),
    (0x35,0x00,0),
    (0x36,0xc0,0), # MACTL 0xc0 11000000 -> 90° 0x96 01100000
    (0x3A,0x01,0), # 01---565，00---666
    (0x40,0x01,0),
    (0x41,0x03,0), # 01--8bit, 03-16bit
    (0x44,0x15,0),
    (0x45,0x15,0),
    (0x7d,0x03,0),
    (0xc1,0xbb,0), 
    (0xc2,0x05,0),
    (0xc3,0x10,0),
    (0xc6,0x3e,0),
    (0xc7,0x25,0),
    (0xc8,0x11,0),
    (0x7a,0x5f,0),
    (0x6f,0x44,0),
    (0x78,0x70,0),
    (0xc9,0x00,0),
    (0x67,0x21,0),

    (0x51,0x0a,0),
    (0x52,0x76,0),
    (0x53,0x0a,0),
    (0x54,0x76,0),

    (0x46,0x0a,0),
    (0x47,0x2a,0),
    (0x48,0x0a,0),
    (0x49,0x1a,0),
    (0x56,0x43,0),
    (0x57,0x42,0),
    (0x58,0x3c,0),
    (0x59,0x64,0),
    (0x5a,0x41,0),
    (0x5b,0x3c,0),
    (0x5c,0x02,0),
    (0x5d,0x3c,0),
    (0x5e,0x1f,0),
    (0x60,0x80,0),
    (0x61,0x3f,0),
    (0x62,0x21,0),
    (0x63,0x07,0),
    (0x64,0xe0,0),
    (0x65,0x02,0),
    (0xca,0x20,0),
    (0xcb,0x52,0),
    (0xcc,0x10,0),
    (0xcD,0x42,0),

    (0xD0,0x20,0),
    (0xD1,0x52,0),
    (0xD2,0x10,0),
    (0xD3,0x42,0),
    (0xD4,0x0a,0),
    (0xD5,0x32,0),

    (0xf8,0x03,0),
    (0xf9,0x20,0),

    (0x80,0x00,0),
    (0xA0,0x00,0),

    (0x81,0x07,0),
    (0xA1,0x06,0),

    (0x82,0x02,0),
    (0xA2,0x01,0),

    (0x86,0x11,0),
    (0xA6,0x10,0),

    (0x87,0x27,0),
    (0xA7,0x27,0),

    (0x83,0x37,0),
    (0xA3,0x37,0),

    (0x84,0x35,0),
    (0xA4,0x35,0),

    (0x85,0x3f,0),
    (0xA5,0x3f,0),

    (0x88,0x0b,0),
    (0xA8,0x0b,0),

    (0x89,0x14,0),
    (0xA9,0x14,0),

    (0x8a,0x1a,0),
    (0xAa,0x1a,0),

    (0x8b,0x0a,0),
    (0xAb,0x0a,0),

    (0x8c,0x14,0),
    (0xAc,0x08,0),

    (0x8d,0x17,0),
    (0xAd,0x07,0),

    (0x8e,0x16,0),
    (0xAe,0x06,0),

    (0x8f,0x1B,0),
    (0xAf,0x07,0),

    (0x90,0x04,0),
    (0xB0,0x04,0),

    (0x91,0x0A,0),
    (0xB1,0x0A,0),

    (0x92,0x16,0),
    (0xB2,0x15,0),

    (0xff,0x00,0),
    (0x11,0x00,700),
    (0x29,0x00,100)
))


def init(self):
    param_buf = bytearray(15)
    param_mv = memoryview(param_buf)

    # Custom CMD sequense   
    for cmd,p,pause in _NV3041A_INIT_CMDS:
        param_buf[0] = p
        self.set_params(cmd, param_mv[:1])
        time.sleep_ms(pause)

