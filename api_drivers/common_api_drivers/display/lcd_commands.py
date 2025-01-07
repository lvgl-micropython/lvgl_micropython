# Copyright (c) 2024 - 2025 Kevin G. Schlosser

# These are here as a reference of the most common commands that displays use.
# this module is not meant to be loaded or imported. It is a complete waste
# of memory if that is done. Most of the init commands are a fire once and done
# so there is no need to keep the commands resident in memory at all.

# the driver framework is designed to send the init commands from inside of a
# function. This means dynamic memory allocation is used so the commands do not
# stay resident in memory. I recommend looking at one of the included display
# drivers to see how it is done and add the commands in the same manner as seen
# in the examples.


# No Operation
# params: None
# arch: 1, 2 ,3
NOP = 0x00

# Software Reset
# params: 0
# arch: 1, 2, 3
SWRESET = 0x01

# Read Display ID
RD_DID = 0x04

# Read Red Channel
# Get the red component of the pixel at (0, 0).
# params: 1
# arch: 2, 3
RD_RCH = 0x06

# Read Green Channel
# Get the green component of the pixel at (0, 0).
# params: 1
# arch: 2, 3
RD_GCH = 0x07

# Read Blue Channel
# Get the blue component of the pixel at (0, 0).
# params: 1
# arch: 2, 3
RD_BCH = 0x08

# Read Display Status
RD_DST = 0x09

# Read Display Power Mode
# Get the current power mode.
# params: 1
# arcg 1, 2, 3
RD_DPM = 0x0A

# Read MADCTL
# Get the data order for transfers from the Host to the display module and
# from the frame memory to the display device
# params: 1
# arch: 1, 2, 3
RD_MADCTL = 0x0B

# Read Pixel Format
# Get the current pixel format.
# params: 1
# arch: 1, 2, 3
RD_COLMOD = 0x0C

# Read Display Image Mode
# Get the current display mode from the peripheral.
# params: 1
# arch: 1, 2, 3
RD_DIM = 0x0D

# Read Display Signal Mode
# Get display module signaling mode
# params: 1
# arch: 1, 2, 3
RD_DSM = 0x0E

# Read Display Self Diagnostic
# Get Peripheral Self-Diagnostic Result
# params: 1
# arch: 1, 2, 3
RD_DSR = 0x0F

# Sleep Mode On
# Power for the display panel is off.
# params: None
# arch: 1, 2, 3
SLPIN = 0x10

# Sleep Mode Off
# Power for the display panel is on.
# params: None
# arch: 1, 2, 3
SLPOUT = 0x11

# Partial Mode On
# Part of the display area is used for image display.
# params: None
# arch: 1, 2
PLTON = 0x12

# Normal Mode On
# The whole display area is used for image display.
# params: None
# arch: 1, 2
NORON = 0x13

# Inversion Off
# Displayed image colors are not inverted
# params: None
# arch: 1, 2, 3
INVOFF = 0x20

# Inversion On
# Displayed image colors are inverted.
# params: None
# arch: 1, 2, 3
INVON = 0x21

# Gamma Set
# Selects the gamma curve used by the display device.
# params: 1
# arch: 1, 2, 3
GAMSET = 0x26

# Display Off
# Blanks the display device.
# params: None
# arch: 1, 2, 3
DISPOFF = 0x28

# Display On
# Show the image on the display device.
# params: None
# arch: 1, 2, 3
DISPON = 0x29

# Set Column Address
# Set the column extent.
# params: 4
# arch: 1, 2
CASET = 0x2A

# Set Row Address
# Set the row extent.
# params: 4
# arch: 1, 2
RASET = 0x2B

# Transfer image data from the Host Processor to the peripheral starting at the
# location provided by set_column_address and set_page_address.
# params: variable
# arch: 1, 2
RAMWR = 0x2C  # Write Frame Memory

# Read Frame Memory
# Transfer image data from the peripheral to the Host Processor interface
# starting at the location provided by set_column_address and set_page_address
# params: variable
# arch: 1, 2
RAMRD = 0x2E

# Set Partial Area Row
# Defines the number of rows in the partial display area on the display device.
# prams: 4
# arch: 1, 2
PTLAR = 0x30

# Set Partial Area Column
# Defines the number of columns in the partial display area on the display
# device.
# prams: 4
# arch: 1, 2
PTLAC = 0x31

# Vertical scrolling definition
# Defines the vertical scrolling and fixed area on display device.
# prams: 6
# arch: 1
VSCRDEF = 0x33

# Tear Effect Off
# Synchronization information is not sent from the display module
# to the host processor.
# params: None
# arch: 1
TEOFF = 0x34

# Tear Effect On
# Synchronization information is sent from the display module
# to the host processor.
# params: None
# arch: 1
TEON = 0x35

# Memory data access control
# Set the data order for transfers from the Host to the display module and
# from the frame memory to the display device.
# prams: 1
# arch: 1, 2, 3
WR_MADCTL = 0x36

# Display data latch order, 0: refresh left to right, 1: refresh right to left
MH_BIT = 1 << 2
# RGB/BGR order, 0: RGB, 1: BGR
BGR_BIT = 1 << 3
# Line address order, 0: refresh top to bottom, 1: refresh bottom to top
ML_BIT = 1 << 4
# Row/Column order, 0: normal mode, 1: reverse mode
MV_BIT = 1 << 5
# Column address order, 0: left to right, 1: right to left
MX_BIT = 1 << 6
# Row address order, 0: top to bottom, 1: bottom to top
MY_BIT = 1 << 7

ORIENTATION_TABLE = (
    MX_BIT,
    MV_BIT,
    MY_BIT,
    MY_BIT | MX_BIT | MV_BIT
)

# Vertical scroll start address
# Defines the vertical scrolling starting point.
# prams: 2
# arch: 1
VSCSAD = 0x37

# Idle Mode Off
# Full color depth is used on the display panel.
# params: None
# arch: 1
IDMOFF = 0x38

# Idle Mode On
# Fall into IDLE mode (8 color depth is displayed)
# Params: None
# arch: 1
IDMON = 0x39

# Defines the format of RGB picture data
# Defines how many bits per pixel are used in the interface.
# prams: 1
# arch: 1, 2, 3
WR_COLMOD = 0x3A

# Memory write continue
WR_RAMCONT = 0x3C

# Memory read continue
# Read image data from the peripheral continuing after the last
# read_memory_continue or read_memory_start.
# params: variable
# arcg: 1, 2
RD_RAMCONT = 0x3E

# Set tear scanline
# Synchronization information is sent from the display module to the host
# processor when the display device refresh reaches the provided scanline.
# params: 2
# arch: 1
WR_TEARSCNLN = 0x44

# Get scanline
# Get the current scanline.
# params: 2
# arch: 1, 2
RD_SCANLINE = 0x45

# Write display brightness
WR_DISPBRGHT = 0x51

# Read display brightness value
RD_DISPBRGHT = 0x52

# Write Control Display
WR_CTRLDISP = 0x53

# Read Control Display
RD_CTRLDISP = 0x53

# Write Content Adaptive Brightness Control
WR_ADPBRHTCTRL = 0x55

# Read Content Adaptive Brightness Control
RD_ADPBRHTCTRL = 0x56

# Write CABC Minimum Brightness
WR_CABCMIN = 0x5E

# Read CABC Minimum Brightness
RD_CABCMIN = 0x5F

# Vcore voltage Control
VCORECTRL = 0xA7

# Interface Mode Control
IFMODE = 0xB0

# Frame Rate Control (In Normal Mode/Full Colors
FRMCTR1 = 0xB1

# Frame Rate Control 2 (In Idle Mode/8 colors)
FRMCTR2 = 0xB2

# Frame Rate Control3 (In Partial Mode/Full Colors)
FRMCTR3 = 0xB3

# Frame Memory Access and Interface Setting
IFACESET = 0xB3

# Display Inversion Control
DINVCTRL = 0xB4

# Display Mode and Frame Memory Write Mode Setting
DISPMODE = 0xB4

# Blanking Porch Control
BPCTRL = 0xB5

# Display Function Control
DFUNCTRL = 0xB6

# Entry Mode Set
SETEM = 0xB7

# Backlight Control 1
BKLTCTRL1 = 0xB8

# Backlight Control 2
BKLTCTRL2 = 0xB9

# Backlight Control 3
BKLTCTRL3 = 0xBA

# Backlight Control 4
BKLTCTRL4 = 0xBB

# Backlight Control 5
BKLTCTRL5 = 0xBC

# Backlight Control 7
BKLTCTRL7 = 0xBE

# Backlight Control 8
BKLTCTRL8 = 0xBF

# TE Control
TECTRL = 0xBA

# Power Control 1
PWRCTL1 = 0xC0

# Power Control 2
PWRCTL2 = 0xC1

# Power Control 3
PWRCTL3 = 0xC2

# Vreg1a Volatge Control
VREG1ACTL = 0xC3

# Vreg1b Volatge Control
VREG1BCTL = 0xC4

# VCOM Control 1
VCOMCTL1 = 0xC5

# Interface Control
IFACECTRL = 0xC6

# VCOM Control 1
VCOMCTL2 = 0xC7

# Gamma setting
GAMMASET = 0xC8

# Vreg2a Volatge Control
VREG2ACTL = 0xC9

# Power control A
PWRCTRLA = 0xCB

# Power control B
PWRCTRLB = 0xCF

# NV Memory Write
WRNVRAM = 0xD0

# NV Memory Protection Key
NVPROTKEY = 0xD1

# NV memory status read
RDNVSTAT = 0xD2

# Read ID4
RDID4 = 0xD3

# Read ID1
RDID1 = 0xDA

# Read ID2
RDID2 = 0xDB

# Read ID3
RDID3 = 0xDC

# Positive Gamma Control
PGC = 0xE0

# Negative Gamma Control
NGC = 0xE1

# Digital Gamma Control 1
DGC1 = 0xE2

# Digital Gamma Control 2
DGC2 = 0xE3

# Frame Rate
FRMRATE = 0xE8

# SPI Data Control
SPICTL = 0xE9

# Driver timing control A1
DRVTIMCTRLA1 = 0xE8

# Driver timing control A2
DRVTIMCTRLA2 = 0xE9

# Driver timing control B
DRVTIMCTRLB = 0xEA

# Charge Pump Frequent Control
CHGPCTRL = 0xEC

# Power on sequence control
PWRONSQCTRL = 0xED

# Enable 3 gamma control
ENA3GAMMA = 0xF2

# Interface Control
IFACECTRL = 0xF6

# Pump ratio control
PUMPRATIOCTRL = 0xF7
