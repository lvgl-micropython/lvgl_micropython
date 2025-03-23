from micropython import const  # NOQA
import time
import lvgl as lv
import lcd_bus


_H_BACK_PORCH = const(140)
_H_FRONT_PORCH = const(160)
_H_PULSE_WIDTH = const(20)

_V_BACK_PORCH = const(20)
_V_FRONT_PORCH = const(12)
_V_PULSE_WIDTH = const(3)

# Software Reset Register
# bits 7: Reconfigure PLL frequency
#         1: to activate reconfigure
# bits 6: Reset
#         1: to perform reset
_SRR = const(0x00)  # default 0x00

_SSR_PLL = const(0x80)
_SSR_RST = const(0x40)

# Chip Configuration Register
# bits 7:     Check PLL Ready
# bits 6:     Mask WAIT# on CS# De-assert
#             0: No Mask, WAIT# keep assert if internal state keep busy and
#                cannot accept next R/W cycle, no matter CS# assert/de-
#                assert. If Host cycle cannot be extended while WAIT# keep
#                low, Host program should poll WAIT# and wait it goes high
#                then start next access.
#             1: Mask, WAIT# de-assert when CS# de-assert. Use in Host cycle
#                can be extended by WAIT# automatically.
# bits 6:     Keypad-scan Enable/Disable
# bits 4 - 3: TFT Panel I/F Setting
#             11: no output
#             10: 16 bit
#             01: 18 bit
#             00: 24 bit
# bits 2:     I2C Master Interface Enable/Disable
# bits 1:     Serial Flash or SPI Interface Enable/Disable
# bite 0:     Host Data Bus Width Selection
#             0: 8 lanes
#             1: 16 lanes
_CCR = const(0x01)  # default 0x48

_CCR_LANE8 = const(0x00)
_CCR_LANE16 = const(0x01)
_CCR_WAIT_MASK_ON = const(0x40)
_CCR_COLOR_MASK = const(0x18)
_CCR_16BIT_COLOR = const(0x10)
_CCR_18BIT_COLOR = const(0x08)  # default
_CCR_24BIT_COLOR = const(0x00)
_CCR_SFLASH = const(0x02)  # default is off
_CCR_KEY_SCAN = const(0x40)  # default is on

# Memory Access Control Register
# bits 7 - 6: Host Read/Write Image Data Format
#             0xb: Direct Write, for below I/F format:
#                  1. 8bits MCU I/F.
#                  2. 16bits MCU I/F with 8bpp data mode 1 & 2.
#                  3. 16bits MCU I/F with 16/24bpp data mode 1.
#                  4. Serial SPI/I2C I/F.
#             10b: Mask high byte of each data
#                  (ex. 16 bit MPU I/F with 8-bpp data mode 1)
#             11b: Mask high byte of even data
#                  (ex. 16 bit MPU I/F with 24-bpp data mode 2)
# bits 5 - 4: Host Read Memory Direction (Only for Graphic Mode)
#             00b: Left Right then Top Bottom.
#             01b: Right Left then Top Bottom.
#             10b: Top Bottom then Left Right.
#             11b: Bottom Top then Left Right.
# bits 3:     always 0x00
# bits 2 - 1: Host Write Memory Direction (Only for Graphic Mode)
#             00b: Left Right then Top Bottom (Original)  0°
#             01b: Right Left then Top Bottom (Hor Flip)
#             10b: Top Bottom then Left Right (Rotate right 90° & Hor flip)
#             11b: Bottom Top then Left Right (Rotate left 90°)
_MACR = const(0x02)  # default 0x00


# rotate 180°
# VDIR (REG[12h] bit3) = 1
# MACR_W_RLTB


# rotate 270°
# VDIR (REG[12h] bit3) = 1
# MACR_W_TBLR

_MACR_W_MASK = const(0x06)
_MACR_W_LRTB = const(0x00)
_MACR_W_RLTB = const(0x02)
_MACR_W_TBLR = const(0x04)
_MACR_W_BTLR = const(0x06)

# Input Control Register
# bits 7:     Interrupt Pin Active Level
# bits 6:     External Interrupt Signal - PSM[0] Pin De-bounce
# bits 5 - 4: External Interrupt Signal - PSM[0] Pin Trigger Type
#             00b: low level trigger
#             01b: falling edge trigger
#             10b: high level trigger
#             11b: rising edge trigger
# bits 2:     Text Mode Enable
#             0: Graphic mode
#             1: Text mode
# bits 1 - 0: Memory Port Read/Write Destination Selection
#             00b: Image buffer (Display RAM) for image data, pattern, user-
#                  characters. Support Read-modify-Write.
#             01b: Gamma table for Color Red/Green/Blue. Each color‟s
#                  gamma table has 256 bytes. User need specify desired
#                  gamma table and continuous write 256 bytes.
#             10b: Graphic Cursor RAM (only accept low 8-bits MPU data,
#                  similar normal register data r/w.), not support Graphic
#                  Cursor RAM read. It contains 4 graphic cursor sets. Each
#                  set has 128x16 bits. User need specify target graphic
#                  cursor set and continue write 256 bytes.
#             11b: Color palette RAM. It is 64x12 bits SRAM, so even address‟
#                  data only low 4 bits are valid. Not support Color palette
#                  RAM read. User need continue write 128 bytes.
_ICR = const(0x03)  # default 0x00

# Memory Data Read/Write Port
# bits 7 - 0: Write Function: Memory Write Data
#             Data to write in memory corresponding to the setting of
#             REG[03h][1:0]. Continuous data write cycle can be accepted in bulk
#             data write case.
#             Note:
#                  A. Image data in Display RAM: according MPU I/F bit width
#                     setting (8/16-bits), Host R/W image data format, canvas
#                     color depth and set canvas in block mode.
#                  B. Pattern data for BTE operation in Display RAM: according
#                     MPU I/F bit width setting (8/16-bits), Host R/W image data
#                     format, canvas color depth and set canvas in block mode.
#                     Active window‟s width and height should set as 8x8 or
#                     16x16 depend on user required.
#                  C. User-characters in Display RAM: according MPU I/F bit
#                     width setting (8/16-bits), Host R/W image data format and
#                     set canvas in linear mode.
#                  D. Character code: only accept low 8-bits MPU data, similar
#                     to normal register R/W. For two bytes character code,
#                     input high byte first. To user defined Character,
#                     code < 8000h is half size, code >= 8000h is full size.
#                  E. Gamma table data: only accept low 8-bits MPU data. User
#                     must set “Select Gamma table sets([3Ch] Bit6-5)” to clear
#                     internal Gamma table‟s address counter then start to write
#                     data. User should program 256 bytes data to memory data
#                     port.
#                  F. Graphic Cursor RAM data: only accept low 8-bits MPU data.
#                     User must set “Select Graphic Cursor sets” bits to clear
#                     internal Graphic Cursor RAM address counter then start t
#                     write data.
#                  G. Color palette RAM data: only accept low 8-bits MPU data.
#                     User must program full Color palette RAM in a continuous
#                     128 byte data write to memory data port and cannot change
#                     register address.
#             Read Function: Memory Read Data
#                            Data to read from memory corresponding to the
#                            setting of REG[03h][1:0]. Continuous data read
#                            cycle can be accepted in bulk data read case.
#            Note1: if you set this port address from different port address,
#                   must issue a dummy read, the first data read cycle is dummy
#                   read and data should be ignored. Graphic Cursor RAM & Color
#                   palette RAM data are not support data read function.
#            Note2: read memory data is 4 bytes alignment no matter color depth
#                   setting.
#            Note3: If user write data to Display RAM user must make sure write
#                   FIFO is empty before he change register number or core task
#                   busy status bit becomes idle
_MRWDP = const(0x04)

#  PCLK PLL Control Register 1
# bits 7 - 6: PCLK Output Divider Ratio, OD[1:0]
#             00b: Divided by 1.
#             01b: Divided by 2.
#             10b: Divided by 3.
#             11b: Divided by 4
# bits 5 - 1: PCLK Input Divider Ratio, R[4:0]
#             The value should be 2~31.
# bits 0:     PCLK Feedback Divider Ratio of Loop, N[8]
#             Total 9 bits, the value should be 2~511..
_PPLLC1 = const(0x05)  # default 0x14

_PLLC_DIV1 = const(0x00)
_PLLC_DIV2 = const(0x40)
_PLLC_DIV3 = const(0x80)
_PLLC_DIV4 = const(0x60)

# PCLK PLL Control Register 2
# bits 7 - 0: PCLK PLLDIVN[7:0]
#             Total 9 bits, the value should be 2~511..
_PPLLC2 = const(0x06)  # default 0x3C

# MCLK PLL Control Register 1
# bits 7 - 6: MCLK output divider Ratio, OD[1:0]
#             00b: Divided by 1.
#             01b: Divided by 2.
#             10b: Divided by 3.
#             11b: Divided by 4
# bits 5 - 1: MCLK Input Divider Ratio, R[4:0]
#             The value should be 2~31.
# bits 0:     MCLK Feedback Divider Ratio of Loop, N[8]
#             Total 9 bits, the value should be 2~511..
_MPLLC1 = const(0x07)  # default 0x14


# MCLK PLL Control Register 2
# bits 7 - 0: MCLK PLLDIVN[7:0]
#             Total 9 bits, the value should be 2~511..
_MPLLC2 = const(0x08)  # default 0x85

# CCLK PLL Control Register 1
# bits 7 - 6: CCLK output divider Ratio, OD[1:0]
#             00b: Divided by 1.
#             01b: Divided by 2.
#             10b: Divided by 3.
#             11b: Divided by 4
# bits 5 - 1: CCLK Input Divider Ratio, R[4:0]
#             The value should be 2~31.
# bits 0:     CCLK Feedback Divider Ratio of Loop, N[8]
#             Total 9 bits, the value should be 2~511..
_CPLLC1 = const(0x09)  # default 0x14

# CCLK PLL Control Register 2
# bits 7 - 0: CCLK PLLDIVN[7:0]
#             Total 9 bits, the value should be 2~511..
_CPLLC2 = const(0x0A)  # default 0x85

# Interrupt Enable Register
# bits 7: Wakeup/Resume Interrupt Enable
# bits 6: External Interrupt Input - PSM[0] Enable)
# bits 5: I2C Master Interrupt Enable
# bits 4: VSYNC Time Base Interrupt Enable
# bits 3: Keypad-scan Interrupt Enable
# bits 2: Serial Flash DMA Complete / Draw Task Finished / BTE Process
#         Complete etc. Interrupt Enable
# bits 1: PWM Timer-1 Interrupt Enable
# bits 0: PWM Timer-0 Interrupt Enable
_INTEN = const(0x0B)  # default 0x00

_INTEN_WAKE_RES_EN = const(0x80)
_INTEN_KEYPAD_EN = const(0x10)

# Interrupt Event Flag Register
# bits 7: Wakeup/Resume Interrupt Flag
# bits 6: External Interrupt Input - PSM[0] Flag
# bits 5: I2C Master Interrupt Flag
# bits 4: VSYNC Time Base Interrupt Flag
# bits 3: Keypad-scan Interrupt Flag
# bits 2: Serial Flash DMA Complete | Draw Task Finished | BTE Process
# bits 1: PWM1 Timer Interrupt Flag
# bits 0: PPWM0 Timer Interrupt Flag
#
# all of the above flags are handled in this manner.
#         Write: Clear Interrupt Flag
#                0: No Operation
#                1: Clears Interrupt Flag
#         Read: Read Interrupt Flag
#                0: No Interrupt has happened
#                1: Interrupt has Happened
_INTF = const(0x0C)  # default 0x00

# Mask Interrupt Flag Register
# bits 7: Wakeup/Resume Interrupt Flag
# bits 6: External Interrupt Input - PSM[0] Flag
# bits 5: I2C Master Interrupt Flag
# bits 4: VSYNC Time Base Interrupt Flag
# bits 3: Keypad-scan Interrupt Flag
# bits 2: Serial Flash DMA Complete | Draw Task Finished | BTE Process
# bits 1: PWM1 Timer Interrupt Flag
# bits 0: PPWM0 Timer Interrupt Flag
#
# all of the above flags are handled in this manner.
#         0: Unmask
#         1: Mask
_MINTFR = const(0x0D)  # default 0x00

# Pull-High Control Register
# bits 5: GPIO-F[7:0] Pull-High Enable
# bits 4: GPIO-E[7:0] Pull-High Enable
# bits 3: GPIO-D[7:0] Pull-High Enable
# bits 2: GPIO-C[4:0] Pull-High Enable
# bits 1: DB[15:8] Pull- High Enable
# bits 0: DB[7:0] Pull- High Enable
#
# all of the above flags are handled in this manner.
#         0: disable pullup
#         1: enable pullup
_PUENR = const(0x0E)  # default 0x00

# PD for GPIO/Key Function Select Register
# bits 7: PD[18] – Function Select
#         0: GPIO-D7
#         1: KO[4]
# bits 6: PD[17] – Function Select
#         0: GPIO-D5
#         1: KO[2]
# bits 5: PD[16] – Function Select
#         0: GPIO-D4
#         1: KO[1]
# bits 4: PD[9] – Function Select
#         0: GPIO-D3
#         1: KO[3]
# bits 3: PD[8] – Function Select
#         0: GPIO-D2
#         1: KI[3]
# bits 2: PD[2] – Function Select
#         0: GPIO-D6
#         1: KI[4]
# bits 1: PD[1] – Function Select
#         0: GPIO-D1
#         1: KI[2]
# bits 0: PD[0] – Function Select
#         0: GPIO-D0
#         1: KI[1]
_PSFSR = const(0x0F)  # default 0x00

# Main/PIP Window Control Register
# bits 7:     PIP-1 Window Enable/Disable
#             0: PIP-1 Window Disable
#             1: PIP-1 Window Enable
#             PIP-1window always on top of PIP-2 window.
# bits 6:     PIP-2 Window Enable/Disable
#             0: PIP-2 Window Disable
#             1: PIP-2 Window Enable
#             PIP-1 window always on top of PIP-2 window
# bits 5:     N/A
# bits 4:     Select Configure PIP 1 or 2 Window’s parameters
#             PIP window‟s parameter including Color Depth, Starting Address,
#              Image Width, Display Coordinates, Window Coordinates, Window
#              Width and Window Height.
#             0: To configure PIP 1‟s parameters.
#             1: To configure PIP 2‟s parameters.
# bits 3 - 2: Main Image Color Depth Setting
#             00b: 8bpp Generic TFT (256 color)
#             01b: 16bpp Generic TFT (65K color)
#             1xb: 24bpp Generic TFT (1.67M color)
# bits 1:     N/A
# bits 0:     To Control Panel’s Synchronous Signals
#             0: Sync Mode
#                Enable VSYNC, HSYNC, DE
#             1: DE Mode
#                Only DE enable, VSYNC & HSYNC in idle state.
_MPWCTR = const(0x10)  # default 0x04

_MPWCTR_MAIN_COLOR_MASK = const(0x0C)
_MPWCTR_MAIN_COLOR_8BPP = const(0x00)
_MPWCTR_MAIN_COLOR_16BPP = const(0x04)
_MPWCTR_MAIN_COLOR_24BPP = const(0x08)

# PIP Window Color Depth Setting
# bits 7 - 4: N/A
# bits 3 - 2: PIP-1 Window Color Depth Setting
#             00b: 8bpp Generic TFT (256 color)
#             01b: 16bpp Generic TFT (65K color)
#             1xb: 24bpp Generic TFT (1.67M color)
# bits 1 - 0: PIP-2 Window Color Depth Setting
#             00b: 8bpp Generic TFT (256 color)
#             01b: 16bpp Generic TFT (65K color)
#             1xb: 24bpp Generic TFT (1.67M color)
_PIPCDEP = const(0x11)  # default 0x05

# Display Configuration Register
# bits 7:     PCLK Inversion
#             0: TFT Panel fetches PD at PCLK rising edge.
#             1: TFT Panel fetches PD at PCLK falling edge.
# bits 6:     Display ON/OFF
#             0b: Display Off
#             1b: Display On
# bits 5:     Display Test Color Bar
#             0b: Disable
#             1b: Enable
# bits 4:     MUST BE 0
# bits 3:     VDIR: Vertical Scan Direction
#             0: From Top to Bottom
#             1: From bottom to Top
# bits 2 - 0: Parallel PD[23:0] Output Sequence
#             000b: RGB
#             001b: RBG
#             010b: GRB
#             011b: GBR
#             100b: BRG
#             101b: BGR
#             110b: Gray
#             111b: Send out Idle State. All data are 0 (Black) or 1(White).
#                   This option has to setup with REG[13h].
_DPCR = const(0x12)  # default 0x00

_DPCR_PCLK_INV = const(0x80)
_DPCR_DISP_PWR = const(0x40)

_DPCR_VDIR_MASK = const(0x20)
_DPCR_VDIR_TOP_BOT = const(0x00)
_DPCR_VDIR_BOT_TOP = const(0x20)

_DPCR_COLOR_MASK = const(0x07)
_DPCR_COLOR_RGB = const(0x00)
_DPCR_COLOR_RBG = const(0x01)
_DPCR_COLOR_GRB = const(0x02)
_DPCR_COLOR_GBR = const(0x03)
_DPCR_COLOR_BRG = const(0x04)
_DPCR_COLOR_BGR = const(0x05)
_DPCR_COLOR_GRAY = const(0x06)

# Panel Scan Clock and Data Setting Register
# bits 7: HSYNC Polarity
#         0: Low Active
#         1: High Active
# bits 6: VSYNC Polarity
#         0: Low Active
#         1: High Active
# bits 5: PDE Polarity
#         0: High Active
#         1: Low Active
# bits 4: PDE Idle State
#         This is used to setup the PDE output status in
#         Power Saving mode or Display Off.
#         0: Pin “PDE” output Low
#         1: Pin “PDE” output High
# bits 3: PCLK Idle State
#         This is used to setup the PCLK output status in
#         Power Saving mode or Display Off.
#         0: Pin “PCLK” output Low
#         1: Pin “PCLK” output High
# bits 2: PD Idle State
#         This is used to setup the PD output status in Vertical/Horizontal
#         Non-Display Period, Power Saving mode or Display Off.
#         0: Pins “PD[23:0]” output Low
#         1: Pins “PD[23:0]” output High
# bits 1: HSYNC Idle State
#         This is used to setup the HSYNC output status in Power Saving mode
#         or Display Off.
#         0: Pin “HSYNC” output Low
#         1: Pin “HSYNC” output High
# bits 0: VSYNC Idle State
#         This is used to setup the VSYNC output status in Power Saving mode
#         or Display Off.
#         0: Pin “VSYNC” output Low
#         1: Pin “VSYNC” output High
_PCSR = const(0x13)  # default 0x03

_PCSR_HSYNC_HIGH = const(0x80)
_PCSR_HSYNC_IDLE_HIGH = const(0x02)
_PCSR_VSYNC_HIGH = const(0x40)
_PCSR_VSYNC_IDLE_HIGH = const(0x01)
_PCSR_PCLK_IDLE_HIGH = const(0x08)
_PCSR_DE_LOW = const(0x10)
_PCSR_DE_IDLE_HIGH = const(0x08)

# Horizontal Display Width Register
# bits 7 - 0: Horizontal Display Width Setting
#             This register is set to a horizontal display width. Its specified
#             LCD screen resolution is 8 pixels in one unit resolution.
#
#                  Horizontal Display Width (pixels) = (HDWR + 1) x 8 + HDWFTR
#
#             HDWFTR (REG[15h]) is the fine-tuning value for Horizontal Display
#             Width. Each fine-tuning resolution is 1 pixels, and the maximum
#             horizontal width is 2,048 pixels.
_HDWR = const(0x14)  # default 0x4F

# Horizontal Display Width Fine Tune Register
# bits 7 - 4: N/A
# bits 3 - 0: Horizontal Display Width Fine Tuning
#             This Register is the fine-tuning value for Horizontal Display
#             Width, and each fine-tuning resolution is 1 pixels.
_HDWFTR = const(0x15)  # default 0x00

# Horizontal Non-Display Period Register
# bits 7 - 5: N/A
# bits 4 - 0: Horizontal Non-Display Period
#             This register assign the Period of Horizontal Non-Display.
#             It‟s also called “Back Porch”.
#
#             Horizontal Non-Display Period (Pixels) = (HNDR + 1) x 8 + HNDFTR
#
#             HNDFTR (REG[17h]) is the fine-tuning value for Horizontal Non-
#             display Period. Each fine-tuning resolution is 1 pixels, and the
#             maximum horizontal width is 2,048 pixels.
_HNDR = const(0x16)  # default 0x03

# Horizontal Non-Display Period Fine Tune Register
# bits 7 - 4: N/A
# bits 3 - 0: Horizontal Non-Display Period Fine Tuning
#             This Register is the fine-tuning value for Horizontal Non-display
#             Period, and each fine-tuning resolution is 1 pixels. it is used to
#             support the SYNC mode panel.
_HNDFTR = const(0x17)  # default 0x06

# HSYNC Start Position Register
# bits 7 - 5: N/A
# bits 4 - 0: HSYNC Start Position
#             This register specifies the starting address of the HSYNC. The
#             starting point for the calculation is the point at which the end
#             of the display area is to start producing HSYNC. The basic unit
#             of each adjustment is 8pixel. It‟s also called “Front Porch”.
#
#                 HSYNC Start Position = (HSTR + 1) x 8
#
_HSTR = const(0x18)  # default 0x1F

# HSYNC Pulse Width Register
# bits 7 - 5: N/A
# bits 4 - 0: HSYNC Pulse Width
#             HSYNC Pulse Width (Pixels) = (HPW + 1) x 8
_HPWR = const(0x19)  # default 0x00

# Vertical Display Height Register
# bits 10 - 0: Vertical Display Height
#              REG[1Ah] mapping to VDHR [7:0]
#              REG[1Bh] bit[2:0] mapping to VDHR [10:8], bit[7:3] are not used.
#              The height of the vertical display is in line, and the formula
#              is as follows:
#
#                  Vertical Display Height (Line) = VDHR + 1
#
_VDHR_L = const(0x1A)  # default 0xDF
_VDHR_H = const(0x1B)

# Vertical Non-Display Period Register
# bits 9 - 0: Vertical Non-Display Period
#             REG[1Ch] mapping to VNDR [7:0]
#             REG[1Dh] bit[1:0] mapping to VNDR [9:8], REG[1Dh] bit[7:2] are
#             not used.
#             The formula is as follows:
#
#                  Vertical Non-Display Period (Line)= (VNDR + 1)
_VNDR_L = const(0x1C)  # default 0x15
_VNDR_H = const(0x1D)

# VSYNC Start Position Register
# bits 7 - 0: The starting position from the end of display area to
#             the beginning of VSYNC.
_VSTR = const(0x1E)  # default 0x0B

# VSYNC Pulse Width Register
# bits 7 - 6: N/A
# bits 5 - 0: VSYNC Pulse Width
#             VSYNC Pulse Width (Line)= (VPWR + 1)
_VPWR = const(0x1F)  # default 0x00

#  Main Image Start Address
# bits 31 - 0: Main Image Start Address
#             REG[20h] mapping to MISA[7:0], **bit[1:0] must be 0.
#             REG[21h] mapping to MISA[15:8]
#             REG[22h] mapping to MISA[23:16]
#             REG[23h] mapping to MISA[31:24]
_MISA_1 = const(0x20)  # default 0x00
_MISA_2 = const(0x21)
_MISA_3 = const(0x22)
_MISA_4 = const(0x23)

# Main Image Width
# bits 12 - 0: Main Image Width
#              The unit is pixel, which is the pixel that represents the
#              horizontal width of the actual LCD. The maximum setting is
#              8,192 pixels.
#              REG[24h] mapping to MIW[7:0], bit[1:0] must be 0.
#              REG[25h] bit[4:0] mapping to MIW[12:8], bit[7:5] are not used..
_MIW_L = const(0x24)  # default 0x00
_MIW_H = const(0x25)

# Main Window Upper-Left Corner X-Coordinates
# bits 12 - 0: Main Window Upper-Left Corner X-Coordinates
#              The Unit is pixel. The sum of X-Coordinates plus Horizontal
#              Display Width cannot be greater than 8,191.
#              REG[26h] mapping to MWULX[7:0], bit[1:0] must be 0.
#              REG[27h] bit[4:0] mapping to MWULX [12:8], bit[7:5] are not used.
_MWULX_L = const(0x26)  # default 0x00
_MWULX_H = const(0x27)

# Main Window Upper-Left Corner Y-Coordinates
# bits 12 - 0: Main Window Upper-Left Corner Y-Coordinates
#              Unit: Pixel. Range is between 0 and 8,191.
#              REG[28h] mapping to MWULY[7:0]
#              REG[29h] bit[4:0] mapping to MWULY [12:8], bit[7:5] are not used.
_MWULY_L = const(0x28)  # default 0x00
_MWULY_H = const(0x29)

# PIP Window 1 or 2 Display Upper-Left Corner X-Coordinates
# bits 12 - 0: PIP Window Display Upper-Left Corner X-Coordinates
#              Unit: Pixel. Y-axis coordinates should less than vertical display
#              height. According to bit of Select Configure PIP 1 or 2 Window‟s
#              parameters(REG[10h]), value will be configured for relative
#              PIP window.
#              REG[2Ah] mapping to PWDULX[7:0], bit[1:0] must be 0.
#              REG[2Bh] bit[4:0] mapping to PWDULX[12:8], bit[7:5] are not used.
_PWDULX_L = const(0x2A)  # default 0x00
_PWDULX_H = const(0x2B)

# PIP Window 1 or 2 Display Upper-Left corner Y-Coordinates
# bits 12 - 0: PIP Window Display Upper-Left Corner Y-Coordinates
#              Unit: Pixel. Y-axis coordinates should less than vertical display
#              height. According to bit of Select Configure PIP 1 or 2 Window‟s
#              parameters(REG[10h]), value will be configured for relative
#              PIP window.
#              REG[2Ah] mapping to PWDULX[7:0], bit[1:0] must be 0.
#              REG[2Bh] bit[4:0] mapping to PWDULX[12:8], bit[7:5] are not used.
_PWDULY_L = const(0x2C)  # default 0x00
_PWDULY_H = const(0x2D)

# PIP Image 1 or 2 Start Address
# bits 31 - 0: PIP Image Start Address
#              According to bit of Select Configure PIP 1 or 2 Window‟s
#              parameters.
#              Function bit will be configured for relative PIP window.
#              REG[2Eh] mapping to PISA[7:0], bit[1:0] must be 0.
#              REG[2Fh] mapping to PISA [15:8]
#              REG[30h] mapping to PISA [23:16]
#              REG[31h] mapping to PISA [31:24]
_PISA_1 = const(0x2E)  # default 0x00
_PISA_2 = const(0x2F)
_PISA_3 = const(0x30)
_PISA_4 = const(0x31)

# PIP Image 1 or 2 Width
# bits 13 - 0: PIP Image Width
#              Unit: Pixel. The value is physical width, and maximum value is
#                    8192 pixels.
#              This width should less than horizontal display width. According
#              to bit of Select Configure PIP 1 or 2 Window‟s parameters, value
#              will be configured for relative PIP window.
#
#              REG[32h] mapping to PIW[7:0], bit[1:0] must be 0.
#              REG[33h] bit[5:0] mapping to PIW[13:8], bit[7:6] are not used.
_PIW_L = const(0x32)  # default 0x00
_PIW_H = const(0x33)

# PIP Window Image 1 or 2 Upper-Left Corner X-Coordinates
# bits 12 - 0: PIP Window Image 1 or 2 Upper-Left Corner X-Coordinates
#              Unit: Pixel. The sum of X-axis coordinates plus PIP image width
#              must less or equal to 8,191.
#              REG[34h] mapping to PWIULX[7:0], bit[1:0] must be 0.
#              REG[35h] bit[4:0] mapping to PWIULX[12:8], bit[7:5] are not used.
_PWIULX_L = const(0x34)  # default 0x00
_PWIULX_H = const(0x35)

# PIP Window Image 1 or 2 Upper-Left Corner Y-Coordinates
# bits 12 - 0: PIP Window Image 1 or 2 Upper-Left Corner Y-Coordinates
#              Unit: Pixel. The sum of Y-axis coordinates plus PIP image height
#              must less or equal to 8,191.
#              REG[36h] mapping to PWIULY[7:0], bit[1:0] must be 0.
#              REG[37h] bit[4:0] mapping to PWIULY[12:8], bit[7:5] are not used.
_PWIULY_L = const(0x36)  # default 0x00
_PWIULY_H = const(0x37)

# PIP Window 1 or 2 Width
# bits 13 - 0: PIP Window 1 or 2 Width
#              Unit: Pixel. Maximum value is 8,192 pixels
#              REG[38h] mapping to PWW[7:0], bit[1:0] must be 0.
#              REG[39h] bit[5:0] mapping to PWW[13:8], bit[7:6] are not used.
_PWW_L = const(0x38)  # default 0x00
_PWW_H = const(0x39)

# PIP Window 1 or 2 Height
# bits 13 - 0: PIP Window 1 or 2 Height
#              Unit: Pixel. Maximum value is 8,192 pixels
#              REG[3Ah] mapping to PWH[7:0], bit[1:0] must be 0.
#              REG[3Bh] bit[5:0] mapping to PWH[13:8], bit[7:6] are not used.

# Note 1: Pip Window Size and Starting Position in the horizontal direction is
#         8 pixels, the vertical resolution is 1 line.
# Note 2: The above registers REG[20h] ~ REG[3Bh] need to be written in turn by
#         LSB to MSB. Suppose we need to set the Main Image Start Address, this
#         relative register are REG[20h] to REG[23h]. Then Host must write
#         Address data in turn from LSB (REG[20h]) to MSB (REG[23h]). When
#         REG[23h] was written, ER-TFTMC050-3 will write complete Main Image
#         Start Address to the internal register in actually
_PWH_L = const(0x3A)  # default 0x00
_PWH_H = const(0x3B)

# Graphic / Text Cursor Control Register
# bits 7:     Gamma Correction Enable
#             Gamma correction is the last output stage.
#             0: Disable
#             1: Enable
# bits 6 - 5: Gamma Table Select for Host Write Gamma Data
#             00b: Gamma table for Blue
#             01b: Gamma table for Green
#             10b: Gamma table for Red
#             11b: NA
# bits 4:     Graphic Cursor Enable
#             Graphic cursor will auto disable if VDIR (REG[12h] bit3)
#             set as “1”.
#             0: Disable
#             1: Enable
# bits 3 - 2: Graphic Cursor Selection
#             Select one from four graphic cursor types. (00b to 11b)
#             00b: Graphic Cursor Set 1
#             01b: Graphic Cursor Set 2
#             10b: Graphic Cursor Set 3
#             11b: Graphic Cursor Set 4
# bits 1:     Text Cursor Enable
#             Note: Text cursor & Graphic cursor cannot enable simultaneously.
#                   Graphic cursor has higher priority than Text cursor if
#                   enabled simultaneously.
#             0: Disable
#             1: Enable
# bits 0:     Text Cursor Blinking Enable
#             0: Disable
#             1: Enable
_GTCCR = const(0x3C)  # default 0x00

# Blink Time Control Register
# bits 7 - 0: Text Cursor Blink Time Setting
#             00h: 1 Frame Cycle Time
#             01h: 2 Frames Cycle Time
#             02h: 3 Frames Cycle Time
#             ...
#             FFh: 256 frames Cycle Time
_BTCR = const(0x3D)  # default 0x00

# Text Cursor Horizontal Size Register
# bits 7 - 5: N/A
# bits 4 - 0: Text Cursor Horizontal Size Setting
#             Note: When character is enlarged, the cursor setting will multiply
#                   the same times as the character enlargement.
#             00000b: 1 Pixel
#             00001b: 2 Pixels
#             ...
#             11111b: 32 Pixels
_CURHS = const(0x3E)  # default 0x07

# Text Cursor Vertical Size Register
# bits 7 - 5: N/A
# bits 4 - 0: Text Cursor Vertical Size Setting
#             Unit: Pixel, Zero-based number. Value “0” means 1 pixel.
#             Note: When character is enlarged, the cursor setting will multiply
#                   the same times as the character enlargement.
_CURVS = const(0x3F)  # default 0x00

# Graphic Cursor Horizontal Position Register
# bits 12 - 0: Graphic Cursor Horizontal Position
#              REG[40h] mapping to GCHP[7:0]
#              REG[41h] bit[4:0] mapping to GCHP[12:8], bit[7:5] are not used
_GCHP_L = const(0x40)  # default 0x00
_GCHP_H = const(0x41)

# Graphic Cursor Vertical Position Register
# bits 12 - 0: Graphic Cursor Vertical Position
#              REG[42h] mapping to GCVP[7:0]
#              REG[43h] bit[4:0] mapping to GCVP[12:8], bit[7:5] are not used.
_GCVP_L = const(0x42)  # default 0x00
_GCVP_H = const(0x43)

# Graphic Cursor Color 0
# bits 7 - 0: Graphic Cursor Color 0 with 256 Colors
#             RGB Format [7:0] = RRRGGGBB
_GCC0 = const(0x44)  # default 0x00

# Graphic Cursor Color 1
# bits 7 - 0: Graphic Cursor Color 1 with 256 Colors
#             RGB Format [7:0] = RRRGGGBB
_GCC1 = const(0x45)  # default 0x00

# Canvas Start Address
# bits 31 - 0: Start Address of Canvas
#              These Registers will be ignored if canvas in linear Addressing
#              mode.
#              REG[50h] mapping to CVSSA[7:0], bit[1:0] must be 0.
#              REG[51h] mapping to CVSSA[15:8]
#              REG[52h] mapping to CVSSA[23:16]
#              REG[53h] mapping to CVSSA[31:24]
_CVSSA_1 = const(0x50)  # default 0x00
_CVSSA_2 = const(0x51)
_CVSSA_3 = const(0x52)
_CVSSA_4 = const(0x53)

# Canvas Image Width
# bits 13 - 0: Canvas Image Width
#              Width = Real Image Width
#              These Registers will be ignored if canvas in linear
#              Addressing mode.
#                  REG[54h] mapping to CVS_IMWTH[7:0], bit[1:0] must be 0.
#                  REG[55h] bit[5:0] mapping to CVS_IMWTH[13:8], bit[7:6]
#                      are not used.
_CVS_IMWTH_L = const(0x54)  # default 0x00
_CVS_IMWTH_H = const(0x55)  # default 0x00

# Active Window Upper-Left Corner X-Coordinates
# bits 12 - 0: Active Window Upper-Left Corner X-Coordinates
#              Unit: Pixel. The sum of X-axis coordinates plus Active Window width
#              cannot large than 8,191.
#              These Registers will be ignored if canvas in linear Addressing mode.
#                  REG[56h] mapping to AWUL_X[7:0]
#                  REG[57h] bit[4:0] mapping to AWUL_X[12:8], bit[7:5] are not used.
_AWUL_X_L = const(0x56)  # default 0x00
_AWUL_X_H = const(0x57)  # default 0x00

# Active Window Upper-Left Corner Y-Coordinates
# bits 12 - 0: Active Window Upper-Left Corner Y-Coordinates
#              Unit: Pixel. The sum of y-axis coordinates plus Active Window width
#              cannot large than 8,191.
#              These Registers will be ignored if canvas in linear Addressing mode.
#                  REG[58h] mapping to AWUL_Y[7:0]
#                  REG[59h] bit[4:0] mapping to AWUL_Y[12:8], bit[7:5] are not used.
_AWUL_Y_L = const(0x58)  # default 0x00
_AWUL_Y_H = const(0x59)  # default 0x00

# Active Window Width
# bits 13 - 0: Active Window Width
#              Unit: Pixel. The value is width.
#              cannot large than 8,191.
#              These Registers will be ignored if canvas in linear
#              Addressing mode.
#                  REG[5Ah] mapping to AW_WTH[7:0]
#                  REG[5Bh] bit[5:0] mapping to AW_WTH[13:8], bit[7:6]
#                      are not used.
_AW_WTH_L = const(0x5A)  # default 0x00
_AW_WTH_H = const(0x5B)  # default 0x00

# Active Window Height
# bits 13 - 0: Active Window Height
#              Unit: Pixel. The value is height.
#              cannot large than 8,191.
#              These Registers will be ignored if canvas in linear
#              Addressing mode.
#                  REG[5Ah] mapping to AW_WTH[7:0]
#                  REG[5Bh] bit[5:0] mapping to AW_WTH[13:8], bit[7:6]
#                      are not used.
_AW_HT_L = const(0x5C)  # default 0x00
_AW_HT_H = const(0x5D)  # default 0x00

# Graphic Read/Write X-Coordinate Register
# bits 12/15 - 0: Set Graphic Read/Write X-Coordinate position
#              When DPRAM in Linear mode: Write address [15:0], Unit: Byte.
#              When DPRAM in Block mode: Write X-Coordinate [12:0], Unit: Pixel.
#                  REG[5Fh] mapping to CURH[7:0]
#                  REG[60h] bit[4:0] mapping to CURH[12:8], bit[7:5
#                      are not used.
_CURH_L = const(0x5F)  # default 0x00
_CURH_H = const(0x60)  # default 0x00

# Graphic Read/Write Y-Coordinate Register
# bits 12/31 - 0: Set Graphic Read/Write Y-Coordinate position
#              When DPRAM in Linear mode: Write address [31:16], Unit: Byte.
#              When DPRAM in Block mode: Write Y-Coordinate [12:0], Unit: Pixel.
#                  REG[61h] mapping to CURV[7:0]
#                  REG[62h] bit[4:0] mapping to CURV[12:8], bit[7:5
#                      are not used.
_CURV_L = const(0x61)  # default 0x00
_CURV_H = const(0x62)  # default 0x00

# Color Depth of Canvas & Active Window
# bits 7 - 4: N/A
# bits 3:     Select What will Read Back from Graphic Read/Write
#             Position Register
#             0: Read back Graphic Write position
#             1: Read back Graphic Read position (Pre-fetch Address)
# bits 2:     Canvas Addressing Mode
#             0: Block mode (X-Y coordinates addressing)
#             1: Linear mode
# bits 1 - 0: Canvas Image’s Color Depth & Memory R/W Data Width
#             In Block Mode:
#                 00b: 8bpp
#                 01b: 16bpp
#                 1xb: 24bpp
#             Note: Monochrome data can input with any one color depth depends
#                   on proper image width.
#             In Linear Mode:
#                 00: 8-bits memory data read/write.
#                 01: 16-bits memory data read/write
_AW_COLOR = const(0x5E)  # default 0x00

_AW_COLOR_CANVAS_MASK = const(0x03)
_AW_COLOR_CANVAS_8BPP = const(0x00)
_AW_COLOR_CANVAS_16BPP = const(0x01)
_AW_COLOR_CANVAS_24BPP = const(0x02)
_AW_COLOR_LINEAR_ADDR = const(0x04)
_AW_COLOR_BLOCK_ADDR = const(0x00)

# PWM Prescaler Registe
# bits 7 - 0: PWM Prescaler Registe
#             This register determine the pre-scaler value for Timer 0 and 1.
#             The Base Frequency is:
#                 Core_Freq / (Prescaler + 1)
_PSCLR = const(0x84)  # default 0x00

# PWM Clock Mux Register
# bits 7 - 6: Setup PWM Timer-1 Divisor
#             (Select 2nd Clock Divider‟s MUX Input for PWM Timer-1)
#             00b: 1
#             01b: 1/2
#             10b: 1/4
#             11b: 1/8
# bits 5 - 4: Setup PWM Timer-0 Divisor
#             (Select 2nd Clock Divider‟s MUX Input for PWM Timer-0)
#             00b: 1
#             01b: 1/2
#             10b: 1/4
#             11b: 1/8
# bits 3 - 2: PWM[1] Function Control
#             0xb: PWM[1] output system error flag
#                  (Scan FIFO POP error or Memory access out of range)
#             10b: PWM[1] output PWM timer 1 event or invert of PWM timer 0
#             11b: PWM[1] output Oscillator Clock
# bits 1 - 0: PWM[0] Function Control
#             0xb: PWM[0] becomes GPIOC[7]
#             10b: PWM[0] output PWM Timer 0
#             11b: PWM[0] output Core Clock (CCLK).
_PMUXR = const(0x85)  # default 0x00

_PMUXR_TIMER1_DIV_MASK = const(0x60)
_PMUXR_TIMER1_DIV1 = const(0x00)
_PMUXR_TIMER1_DIV2 = const(0x40)
_PMUXR_TIMER1_DIV4 = const(0x80)
_PMUXR_TIMER1_DIV8 = const(0x60)
_PMUXR_TIMER0_DIV_MASK = const(0x30)
_PMUXR_TIMER0_DIV1 = const(0x00)
_PMUXR_TIMER0_DIV2 = const(0x10)
_PMUXR_TIMER0_DIV4 = const(0x20)
_PMUXR_TIMER0_DIV8 = const(0x30)
_PMUXR_PWM1_MASK = const(0x0C)
_PMUXR_PWM1_SYS_ERR = const(0x04)
_PMUXR_PWM1_TIMER1 = const(0x08)
_PMUXR_PWM1_OSC_CLOCK = const(0x0C)
_PMUXR_PWM0_MASK = const(0x03)
_PMUXR_PWM0_TIMER0 = const(0x02)
_PMUXR_PWM0_CORE_CLOCK = const(0x03)

# PWM Configuration Register
# bits 7: N/A
# bits 6: PWM Timer-1 Output Inverter On/Off
#         0 = Inverter off
#         1 = Inverter on for PWM1
# bits 5: PWM Timer-1 Auto Reload On/Off
#         0: One-Shot Mode
#         1: Interval Mode (Auto Reload)
# bits 4: PWM Timer-1 Start/Stop
#         0: Stop
#         1: Start
# bits 3: PWM Timer-0 Dead Zone Enable
#         0: Disable
#         1: Enable
# bits 2: PWM Timer-0 Output Inverter On/Off
#         0 = Inverter off
#         1 = Inverter on for PWM0
# bits 1: PWM Timer-0 Auto Reload On/Off
#         0: One-Shot Mode
#         1: Interval Mode (Auto Reload)
# bits 0: PWM Timer-0 Start/Stop
#         In Interval Mode, Host need program it as 0 to stop PWM timer. In
#         One-shot Mode, this bit will auto clear.
#         The Host may read this bit to know current PWMx is running or stopped.
#         0: Stop
#         1: Start

_PCFGR = const(0x86)  # default 0x02

_PCFGR_TIMER1_INVERT = const(0x40)
_PCFGR_TIMER1_RESTART_AUTO = const(0x20)
_PCFGR_TIMER1_START = const(0x10)
_PCFGR_TIMER0_DEAD_ZONE = const(0x08)
_PCFGR_TIMER0_INVERT = const(0x04)
_PCFGR_TIMER0_RESTART_AUTO = const(0x02)
_PCFGR_TIMER0_START = const(0x01)


# Timer-0 Dead Zone Length Register
# bits 7 - 0: Timer-0 Dead Zone Length Register
#             These 8 bits determine the dead zone length. The 1 unit time of
#             the dead zone length is equal to Timer 0.
_DZ_LENGTH = const(0x87)  # default 0x00

# Timer-0 Compare Buffer Register
# bits 15 - 0: Timer-0 compare Buffer Register
#              The Timer-0 Compare Buffer Register have a total of 16bits. When
#              the counter is equal to or less than the value of this register,
#              and if INV_ON bit is Off, then PWM1 output is high level.
#
#                  REG[88h] mapping to TCMPB0 [7:0]
#                  REG[89h] mapping to TCMPB0 [15:8]
_TCMPB0_L = const(0x88)  # default 0x00
_TCMPB0_H = const(0x89)

# Timer-0 Count Buffer Register
# bits 15 - 0: Timer-0 Count Buffer Register [15:0]
#              The Timer-0 count registers have a total of 16bit. When the
#              counter down to 0 and the Reload_EN is enabled, the PWM will
#              overloads the value of this register to the counter. When the
#              PWM begins to count, the current count value can be read back
#              through this register
#
#                  REG[8Ah] mapping to TCNTB0 [7:0]
#                  REG[8Bh] mapping to TCNTB0 [15:8]
_TCNTB0_L = const(0x8A)  # default 0x00
_TCNTB0_H = const(0x8B)

# Timer-1 Compare Buffer Register
# bits 15 - 0: Timer-1 compare Buffer Register
#              The Timer-1 Compare Buffer Register have a total of 16bits. When
#              the counter is equal to or less than the value of this register,
#              and if INV_ON bit is Off, then PWM1 output is high level.
#
#                  REG[8Ch] mapping to TCMPB1 [7:0]
#                  REG[8Dh] mapping to TCMPB1 [15:8]
_TCMPB1_L = const(0x8C)  # default 0x00
_TCMPB1_H = const(0x8D)

# Timer-1 Count Buffer Register
# bits 15 - 0: Timer-1 Count Buffer Register [15:0]
#              The Timer-1 count registers have a total of 16bit. When the
#              counter down to 0 and the Reload_EN is enabled, the PWM will
#              overloads the value of this register to the counter. When the
#              PWM begins to count, the current count value can be read back
#              through this register
#
#                  REG[8Eh] mapping to TCNTB1 [7:0]
#                  REG[8Fh] mapping to TCNTB1 [15:8]
_TCNTB1_L = const(0x8E)  # default 0x00
_TCNTB1_H = const(0x8F)

# Power Management Register
# bits 7:     Enter Power Saving State
#             Note: There are 3 ways to wake up from power saving state:
#                   External interrupt event, Key Scan wakeup and Software
#                   wakeup. Write this bit to 0 will cause Software wakeup.
#                   It will be cleared until chip resume. MPU must wait until
#                   system quit from power saving state than allow to write
#                   other registers. User may check this bit or check status
#                   bit1 (power saving
#             0: Normal state or wake up from power saving state
#             1: Enter power saving state.
# bits 6 - 2: N/A
# bits 1 - 0: Power Saving Mode Definition
#             00b: NA
#             01b: Standby Mode, CCLK & PCLK will Stop. MCLK is
#                  provided by MPLL.
#             10b: Suspend Mode, CCLK & PCLK will Stop. MCLK is
#                  provided by OSC.
#             11b: Sleep Mode, All of Clocks and PLL will Stop.
_PMU = const(0xDF)  # default 0x03

# SDRAM Attribute Register
# bits 7:     SDRAM Power Saving
#             0: Execute power down command to enter power saving mode
#             1: Execute self refresh command to enter power saving mode
# bits 6:     MUST BE 0
# bits 5:     SDRAM Bank Number, SDR_BANK
#             Must set to 1 after reset.
#             1: 4 Banks
# bits 4 - 3: SDRAM Row Addressing, SDR_ROW
#             Must set to 00 after reset.
#             00b: 2K (A0-A10)
# bits 2 - 0: SDRAM Column Addressing, SDR_COL
#             Must set to 000 after reset.
#             000b: 256 (A0-A7)
#
# NOTE: 32Mb(4MB, 2Mx16)
#      value: 0x20
#      description:
#        Bank no：4
#        Row Size：2048
#        Column Size：256
#
# Note: The value of register REG[E0h] must be set according to above table.
#       Otherwise, the display of TFT panel will abnormal and the image
#       is garbled.
_SDRAR = const(0xE0)  # default 0x14

_SDRAR_4BANKS = const(0x20)


# SDRAM Mode Register & Extended Mode Register
# bits 7 - 3: Must keep 0.
# bits 2 - 0: SDRAM CAS latency, SDR_CASLAT
#             Note: The suggest setting value of this register is 03h.
#                   This register was locked after SDR_INITDONE (REG[E4h] bit0)
#                   was set as 1.
#             010b: 2 SDRAM clock
#             011b: 3 SDRAM clock
#             Others: Reserved
_SDRMD = const(0xE1)  # default 0x03

_SDRMD_CASLAT_MASK = const(0x03)
_SDRMD_CASLAT_2 = const(0x02)
_SDRMD_CASLAT_3 = const(0x03)


# SDRAM Mode Register & Extended Mode Register
# bits 15 - 0: SDRAM Auto Refresh Interval
#              The internal refresh time is determined according to the period
#              specification of the SDRAM's refresh and the row size. For
#              example, if the SDRAM frequency is 100MHz, SDRAM's refresh period
#              Tref is  64ms, and the row size is 4,096, then the internal
#              refresh time should be less than
#              64 x 10 - 3 / 4096 x 100 x 106 ~= 1562 = 61Ah..
#              Therefore the REG[E3h][ E2h] is set 030Dh.
#
#              Note: If this register is set to 0000h, SDRAM automatic refresh
#                    will be prohibited.
#              REG[E2h] mapping to SDR_REF [7:0].
#              REG[E3h] mapping to SDR_REF [15:8]
#
# NOTE: The Reference Setting of REG[E3h-E2h]
#       model: ER-TFTMC0 50-3
#       REG[E3h]: 06h
#       REG[E2h]: 1Ah
_SDR_REF_L = const(0xE2)  # default 0x00
_SDR_REF_H = const(0xE3)

# SDRAM Control Register
# bits 7 - 4: Must keep 0.
# bits 3:     Report Warning Condition
#             Warning: condition are memory read cycle close to SDRAM maximum
#                      address boundary (may over maximum address minus 512
#                      bytes) or out of range or SDRAM bandwidth insufficient to
#                      fulfill panel‟s frame rate, then this warning event will
#                      be latched, user could check this bit to do some
#                      judgments. That warning flag could be cleared by set this
#                      bit as 0.
#             0: Disable or Clear warning flag
#             1: Disable or Clear warning flag
# bits 2:     SDRAM Timing Parameter Register Enable, SDR_PARAMEN
#             0: Disable Display RAM timing parameter registers
#             1: Enable Display RAM timing parameter registers
# bits 1:     Enter Power Saving Mode, SDR_PSAVING
#             0 to 1 transition will enter power saving mode
#             1 to 0 transition will exit power saving mode
# bits 0:     Start SDRAM Initialization Procedure, SDR_INITDONE
#             0 to 1 transition will execute Display RAM initialization
#             procedure. Read value „1‟ means Display RAM is initialized and
#             ready for access. Once it was written as 1, it cannot be
#             rewrite as 0.
#             1 to 0 transition without have any operation.
#             NOTE: “Write 1” will execute Display RAM initialization procedure.
_SDRCR = const(0xE4)  # default 0x00

_SDRCR_INITDONE = const(0x01)

# ************ NOT USED ***************

# NOTE: The following Display RAM Timing Registers (REG[E0h-E3h]) are
#        available only when SDR_PARAMEN (REG[E4] bit2) is set to 1.

# SDRAM Timing Parameter 1
# bits 7 - 4: Must keep 0.
# bits 3 - 0: Time from Load Mode Command to Active/Refresh Command (TMRD)
#             0000b: 1 SDRAM Clock
#             0001b: 2 SDRAM Clock
#             0010b: 3 SDRAM Clock
#             ...
#             1111b: 16 SDRAM Clock
_SDRCTP1 = const(0xE0)  # default 0x02

# SDRAM Timing Parameter 2
# bits 7 - 4: Auto Refresh Period, TRFC
#             0h – Fh: 1 ~ 16 SDRAM Clock (As REG[E0h] bit[3:0])
# bits 3 - 0: Time of Exit SELF Refresh-to-ACTIVE Command (TXSR)
#             0h – Fh: 1 ~ 16 SDRAM Clock
_SDRCTP2 = const(0xE1)    # default 0x87

# SDRAM Timing Parameter 3
# bits 7 - 4: Time of Pre-charge Command Period (TRP, 15/20ns)
#             0h – Fh: 1 ~ 16 SDRAM Clock
# bits 3 - 0: Time of WRITE Recovery Time (TWR)
#             0h – Fh: 1 ~ 16 SDRAM Clock
_SDRCTP3 = const(0xE2)  # default 0x20

# SDRAM Timing Parameter 4
# bits 7 - 4: Delay Time of Active-to-Read/Write (TRCD)
#             0h – Fh: 1 ~ 16 SDRAM Clock
# bits 3 - 0: Time of Active-to-Precharge (TRAS)
#             0h – Fh: 1 ~ 16 SDRAM Clock
_SDRCTP4 = const(0xE3)  # default 0x26
# *******************************


def set_backlight(self, value):
    self._write_reg(_PMUXR, self._pmuxr)
    self._write_reg(_PSCLR, 19)

    self._pmuxr &= ~_PMUXR_TIMER1_DIV_MASK

    self._write_reg(_PMUXR, self._pmuxr)

    self._write_reg(_TCNTB1_L, 0x64)
    self._write_reg(_TCNTB1_H, 0x00)

    value = min(100, max(0, int(value)))
    self._backlight = value

    self._write_reg(_TCMPB1_L, value)
    self._write_reg(_TCMPB1_H, 0x00)

    self._pcfgr |= _PCFGR_TIMER1_START
    self._write_reg(_PCFGR, self._pcfgr)


def get_backlight(self):
    return self._backlight


def init(self):
    self._set_reg(_PPLLC1, 0x8A)
    self._set_reg(_PPLLC2, 0x3C)

    self._set_reg(_MPLLC1, 0x8A)
    self._set_reg(_MPLLC2, 0x84)

    self._set_reg(_CPLLC1, 0x8A)
    self._set_reg(_CPLLC2, 0x84)

    self._set_reg(_SRR, _SSR_PLL)
    time.sleep_ms(1)  # NOQA

    self._pmuxr = _PMUXR_PWM1_TIMER1 | _PMUXR_PWM0_TIMER0
    self._set_reg(_PMUXR, self._pmuxr)

    self._pcfgr = _PCFGR_TIMER1_RESTART_AUTO | _PCFGR_TIMER0_RESTART_AUTO
    self._set_reg(_PCFGR, self._pcfgr)

    # SDRAM
    self._set_reg(_SDRAR, _SDRAR_4BANKS | 0x09)
    self._set_reg(_SDRMD, _SDRMD_CASLAT_3)

    self._set_reg(_SDR_REF_L, 0xE6)
    self._set_reg(_SDR_REF_H, 0x01)
    self._set_reg(_SDRCR, _SDRCR_INITDONE)

    while self._wait_pin.value():
        time.sleep_ms(1)  # NOQA

    time.sleep_ms(1)  # NOQA

    color_size = lv.color_format_get_size(self._color_space)
    if color_size not in (2, 3):  # NOQA
        raise RuntimeError(
            'LT7281 driver only supports 16bit and 24 bit color depths'
        )

    ccr_flag = _CCR_WAIT_MASK_ON

    if color_size == 2:
        ccr_flag |= _CCR_16BIT_COLOR
    else:
        ccr_flag |= _CCR_24BIT_COLOR

    lanes = self._data_bus.get_lane_count()
    if isinstance(self._data_bus, lcd_bus.I80Bus):
        if lanes == 8:
            ccr_flag |= _CCR_LANE8
        elif lanes == 16:
            ccr_flag |= _CCR_LANE16
        else:
            raise RuntimeError(
                'only 8 or 16 lanes is supported when using the I80Bus'
            )

    self._set_reg(_CCR, ccr_flag)

    self._macr = 0x00
    self._set_reg(_MACR, self._macr)
    self._set_reg(_ICR, 0x00)

    self._dpcr = _DPCR_PCLK_INV | _DPCR_DISP_PWR | self._color_byte_order

    self._set_reg(_DPCR, self._dpcr)

    self._set_reg(_PCSR, _PCSR_HSYNC_IDLE_HIGH | _PCSR_HSYNC_IDLE_HIGH)

    if self.display_width < 8:
        self._set_reg(_HDWR, 0x00)
        self._set_reg(_HDWFTR, self.display_width)
    else:
        self._set_reg(_HDWR, int(self.display_width / 8) - 1)
        self._set_reg(_HDWFTR, (self.display_width % 8) & 0xFF)

    self._set_reg(_VDHR_L, (self.display_height - 1) & 0xFF)
    self._set_reg(_VDHR_H, ((self.display_height - 1) >> 8) & 0x07)

    if _H_BACK_PORCH < 8:
        self._set_reg(_HNDR, 0x00)
        self._set_reg(_HNDFTR, _H_BACK_PORCH & 0xFF)
    else:
        self._set_reg(_HNDR, int(_H_BACK_PORCH / 8) - 1)
        self._set_reg(_HNDFTR, (_H_BACK_PORCH % 8) & 0xFF)

    if _H_FRONT_PORCH < 8:
        self._set_reg(_HSTR, 0x00)
    else:
        self._set_reg(_HSTR, int(_H_FRONT_PORCH / 8) - 1)

    if _H_PULSE_WIDTH < 8:
        self._set_reg(_HPWR, 0x00)
    else:
        self._set_reg(_HPWR, int(_H_PULSE_WIDTH / 8) - 1)

    self._set_reg(_VNDR_L, (_V_BACK_PORCH - 1) & 0xFF)
    self._set_reg(0x1D, ((_V_BACK_PORCH - 1) >> 8) & 0x03)

    self._set_reg(_VSTR, (_V_FRONT_PORCH - 1) & 0xFF)
    self._set_reg(_VPWR, (_V_PULSE_WIDTH - 1) & 0xFF)

    if color_size == 2:
        self._set_reg(_MPWCTR, _MPWCTR_MAIN_COLOR_16BPP)
        self._set_reg(_AW_COLOR, _AW_COLOR_CANVAS_16BPP)
        self._set_reg(_MPWCTR, _MPWCTR_MAIN_COLOR_16BPP)
    else:
        self._set_reg(_MPWCTR, _MPWCTR_MAIN_COLOR_24BPP)
        self._set_reg(_AW_COLOR, _AW_COLOR_CANVAS_24BPP)
        self._set_reg(_MPWCTR, _MPWCTR_MAIN_COLOR_24BPP)
