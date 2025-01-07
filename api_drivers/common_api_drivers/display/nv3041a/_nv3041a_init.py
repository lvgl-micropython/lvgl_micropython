# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import time
import lvgl as lv
import lcd_bus


_NOP = const(0x00)
_RD_SYSID = const(0x04)
_RD_STATE = const(0x09)
_RD_DISP = const(0x0A)
_RD_MADCTL = const(0x0B)
_RD_IM = const(0x0D)
_RD_SM = const(0x0E)
_SLPIN = const(0x10)
_SLPOUT = const(0x11)
_PTLON = const(0x12)
_NORMAL = const(0x13)
_WR_SYSID = const(0x14)
_INVOFF = const(0x20)
_INVON = const(0x21)
_DISPOFF = const(0x28)
_DISPON = const(0x29)
_COL_ADR = const(0x2A)
_ROW_ADR = const(0x2B)
_PTL_ADR = const(0x30)
_SCROLL_ADR = const(0x33)
_TEOFF = const(0x34)
_TEON = const(0x35)
_MADCTL = const(0x36)
_VSCSAD = const(0x37)
_IDMOFF = const(0x38)
_IDMON = const(0x39)
_COLMOD = const(0x3A)
_MACTL_USR = const(0x40)
_BUS_WD = const(0x41)
_QSPI_DCTL = const(0x43)
_FSM_VBP = const(0x44)
_FSM_VFP = const(0x45)
_FSM_HBP_ODD = const(0x46)
_FSM_HFP_ODD = const(0x47)
_FSM_HBP_EVEN = const(0x48)
_FSM_HFP_EVEN = const(0x49)
_SCAN_VRES = const(0x4A)
_SCAN_HRES = const(0x4B)
_RGB_HBP_ODD = const(0x4C)
_RGB_HFP_ODD = const(0x4D)
_RGB_HBP_EVEN = const(0x4E)
_RGB_HFP_EVEN = const(0x4F)
_GATE_SCAN = const(0x50)
_GATE_ST_O = const(0x51)
_GATE_ED_O = const(0x52)
_GATE_ST_E = const(0x53)
_GATE_ED_E = const(0x54)
_PANEL_CTRL = const(0x55)
_SRC_LOAD = const(0x56)
_SRC_CS_ST = const(0x57)
_SRC_CS_PW = const(0x58)
_SRC_CS_NW = const(0x59)
_SRC_PC_ST_O = const(0x5A)
_SRC_PC_WD_O = const(0x5B)
_SRC_PC_ST_E = const(0x5C)
_SRC_PC_WD_E = const(0x5D)
_SRC_POL_SW = const(0x5E)
_SRC_OP_ST_O = const(0x60)
_SRC_OP_ST_E = const(0x61)
_SRC_OP_ED_MSB = const(0x62)
_SRC_OP_ED_O_LSB = const(0x63)
_SRC_OP_ED_E_LSB = const(0x64)
_SRC_OFC = const(0x65)
_CLR_SCR = const(0x66)
_SRC_IBIAS = const(0x67)
_PTL_DAT = const(0x68)
_LVD_SET = const(0x6E)
_USR_GVDD = const(0x6F)
_USR_GVCL = const(0x78)
_USR_VGSP = const(0x7A)
_GVREF2V = const(0x7C)
_VDDS_TRIM = const(0x7D)
_GAM_VRP0 = const(0x80)
_GAM_VRP1 = const(0x81)
_GAM_VRP2 = const(0x82)
_GAM_VRP3 = const(0x83)
_GAM_VRP4 = const(0x84)
_GAM_VRP5 = const(0x85)
_GAM_PRP0 = const(0x86)
_GAM_PRP1 = const(0x87)
_GAM_PKP0 = const(0x88)
_GAM_PKP1 = const(0x89)
_GAM_PKP2 = const(0x8A)
_GAM_PKP3 = const(0x8B)
_GAM_PKP4 = const(0x8C)
_GAM_PKP5 = const(0x8D)
_GAM_PKP6 = const(0x8E)
_GAM_PKP7 = const(0x8F)
_GAM_PKP8 = const(0x90)
_GAM_PKP9 = const(0x91)
_GAM_PKP10 = const(0x92)
_GAM_VRN0 = const(0xA0)
_GAM_VRN1 = const(0xA1)
_GAM_VRN2 = const(0xA2)
_GAM_VRN3 = const(0xA3)
_GAM_VRN4 = const(0xA4)
_GAM_VRN5 = const(0xA5)
_GAM_PRN0 = const(0xA6)
_GAM_PRN1 = const(0xA7)
_GAM_PKN0 = const(0xA8)
_GAM_PKN1 = const(0xA9)
_GAM_PKN2 = const(0xAA)
_GAM_PKN3 = const(0xAB)
_GAM_PKN4 = const(0xAC)
_GAM_PKN5 = const(0xAD)
_GAM_PKN6 = const(0xAE)
_GAM_PKN7 = const(0xAF)
_GAM_PKN8 = const(0xB0)
_GAM_PKN9 = const(0xB1)
_GAM_PKN10 = const(0xB2)
_BIAS_VBG = const(0xC0)
_MV_CLP = const(0xC1)
_VGH_CLP = const(0xC2)
_VGL_CLP = const(0xC3)
_MV_TD = const(0xC4)
_MV_SS_CTRL = const(0xC5)
_RATIO_CTRL = const(0xC6)
_MV_PUMP_CLK = const(0xC7)
_HV_PUMP_CLK = const(0xC8)
_MV_CLK_CLP = const(0xC9)
_RD_SYSID1 = const(0xDA)
_RD_SYSID2 = const(0xDB)
_RD_SYSID3 = const(0xDC)
_RGB_CTL = const(0xE1)
_RGB_POL = const(0xE2)
_INTF_VBP = const(0xE3)
_INTF_HBP = const(0xE4)
_DVDD_TRIM = const(0xE5)
_ESD_CTRL = const(0xE6)
_TE_CTRL = const(0xE7)
_OTP_CTRL1 = const(0xF1)
_OTP_CTRL2 = const(0xF2)
_OTP_CTRL3 = const(0xF3)
_OTP_CRCH = const(0xF4)
_OTP_CRCL = const(0xF5)
_OTP_RDD = const(0xF6)


def init(self):
    param_buf = bytearray(1)
    param_mv = memoryview(param_buf)

    num_lanes = self._data_bus.get_lane_count()
    color_size = lv.color_format_get_size(self._color_space)

    if color_size == 2:  # NOQA
        COLMOD = 0x01
    elif color_size == 3:
        COLMOD = 0x00
    else:
        raise RuntimeError(
            'NV3401A IC only supports '
            'lv.COLOR_FORMAT.RGB888 and lv.COLOR_FORMAT.RGB565'
        )

    if isinstance(self._data_bus, lcd_bus.SPIBus) and num_lanes == 2:
        BUS_WD = COLMOD
    elif isinstance(self._data_bus, lcd_bus.I80Bus):
        if num_lanes == 8:
            BUS_WD = 0x01
        elif num_lanes == 9:
            if COLMOD != 0x00:
                raise RuntimeError('9 bit I80 only supports RGB666 (RGB888)')
            BUS_WD = 0x02
        elif num_lanes == 16:
            if COLMOD == 0x00:
                BUS_WD = 0x33
            else:
                BUS_WD = 0x03
        else:
            raise RuntimeError('Only 8, 9 and 16 I80 lanes are supported')
    else:
        BUS_WD = 0x00

    param_buf[0] = 0xA5
    self.set_params(0xFF, param_mv[:1])

    param_buf[0] = 0xC0
    self.set_params(_MADCTL, param_mv[:1])

    param_buf[0] = COLMOD
    self.set_params(_COLMOD, param_mv[:1])

    param_buf[0] = BUS_WD
    self.set_params(_BUS_WD, param_mv[:1])

    param_buf[0] = 0x15
    self.set_params(_FSM_VBP, param_mv[:1])

    param_buf[0] = 0x15
    self.set_params(_FSM_VFP, param_mv[:1])

    param_buf[0] = 0x03
    self.set_params(_VDDS_TRIM, param_mv[:1])

    param_buf[0] = 0xBB
    self.set_params(_MV_CLP, param_mv[:1])
    param_buf[0] = 0x05
    self.set_params(_VGH_CLP, param_mv[:1])
    param_buf[0] = 0x10
    self.set_params(_VGL_CLP, param_mv[:1])

    param_buf[0] = 0x3E
    self.set_params(_RATIO_CTRL, param_mv[:1])

    param_buf[0] = 0x25
    self.set_params(_MV_PUMP_CLK, param_mv[:1])
    param_buf[0] = 0x11
    self.set_params(_HV_PUMP_CLK, param_mv[:1])

    param_buf[0] = 0x5F
    self.set_params(_USR_VGSP, param_mv[:1])

    param_buf[0] = 0x44
    self.set_params(_USR_GVDD, param_mv[:1])

    param_buf[0] = 0x70
    self.set_params(_USR_GVCL, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(_MV_CLK_CLP, param_mv[:1])

    param_buf[0] = 0x21
    self.set_params(_SRC_IBIAS, param_mv[:1])

    param_buf[0] = 0x0A
    self.set_params(_GATE_ST_O, param_mv[:1])
    param_buf[0] = 0x0A
    self.set_params(_GATE_ST_E, param_mv[:1])

    param_buf[0] = 0x76
    self.set_params(_GATE_ED_O, param_mv[:1])
    param_buf[0] = 0x76
    self.set_params(_GATE_ED_E, param_mv[:1])

    param_buf[0] = 0x0A
    self.set_params(_FSM_HBP_ODD, param_mv[:1])
    param_buf[0] = 0x2A
    self.set_params(_FSM_HFP_ODD, param_mv[:1])

    param_buf[0] = 0x0A
    self.set_params(_FSM_HBP_EVEN, param_mv[:1])
    param_buf[0] = 0x1A
    self.set_params(_FSM_HFP_EVEN, param_mv[:1])

    param_buf[0] = 0x43
    self.set_params(_SRC_LOAD, param_mv[:1])

    param_buf[0] = 0x42
    self.set_params(_SRC_CS_ST, param_mv[:1])
    param_buf[0] = 0x3C
    self.set_params(_SRC_CS_PW, param_mv[:1])
    param_buf[0] = 0x64
    self.set_params(_SRC_CS_NW, param_mv[:1])

    param_buf[0] = 0x41
    self.set_params(_SRC_PC_ST_O, param_mv[:1])
    param_buf[0] = 0x02
    self.set_params(_SRC_PC_ST_E, param_mv[:1])
    param_buf[0] = 0x3C
    self.set_params(_SRC_PC_WD_O, param_mv[:1])
    param_buf[0] = 0x3C
    self.set_params(_SRC_PC_WD_E, param_mv[:1])

    param_buf[0] = 0x80
    self.set_params(_SRC_OP_ST_O, param_mv[:1])
    param_buf[0] = 0x3F
    self.set_params(_SRC_OP_ST_E, param_mv[:1])
    param_buf[0] = 0x21
    self.set_params(_SRC_OP_ED_MSB, param_mv[:1])
    param_buf[0] = 0x07
    self.set_params(_SRC_OP_ED_O_LSB, param_mv[:1])
    param_buf[0] = 0xE0
    self.set_params(_SRC_OP_ED_E_LSB, param_mv[:1])

    param_buf[0] = 0x1F
    self.set_params(_SRC_POL_SW, param_mv[:1])

    param_buf[0] = 0x02
    self.set_params(_SRC_OFC, param_mv[:1])

    param_buf[0] = 0x20
    self.set_params(0xCA, param_mv[:1])
    param_buf[0] = 0x52
    self.set_params(0xCB, param_mv[:1])
    param_buf[0] = 0x10
    self.set_params(0xCC, param_mv[:1])
    param_buf[0] = 0x42
    self.set_params(0xCD, param_mv[:1])

    param_buf[0] = 0x20
    self.set_params(0xD0, param_mv[:1])
    param_buf[0] = 0x52
    self.set_params(0xD1, param_mv[:1])
    param_buf[0] = 0x10
    self.set_params(0xD2, param_mv[:1])
    param_buf[0] = 0x42
    self.set_params(0xD3, param_mv[:1])
    param_buf[0] = 0x0A
    self.set_params(0xD4, param_mv[:1])
    param_buf[0] = 0x32
    self.set_params(0xD5, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(_GAM_VRP0, param_mv[:1])
    param_buf[0] = 0x07
    self.set_params(_GAM_VRP1, param_mv[:1])
    param_buf[0] = 0x02
    self.set_params(_GAM_VRP2, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(_GAM_VRN0, param_mv[:1])
    param_buf[0] = 0x06
    self.set_params(_GAM_VRN1, param_mv[:1])
    param_buf[0] = 0x01
    self.set_params(_GAM_VRN2, param_mv[:1])

    param_buf[0] = 0x11
    self.set_params(_GAM_PRP0, param_mv[:1])
    param_buf[0] = 0x27
    self.set_params(_GAM_PRP1, param_mv[:1])
    param_buf[0] = 0x37
    self.set_params(_GAM_VRP3, param_mv[:1])
    param_buf[0] = 0x35
    self.set_params(_GAM_VRP4, param_mv[:1])
    param_buf[0] = 0x3F
    self.set_params(_GAM_VRP5, param_mv[:1])

    param_buf[0] = 0x10
    self.set_params(_GAM_PRN0, param_mv[:1])
    param_buf[0] = 0x27
    self.set_params(_GAM_PRN1, param_mv[:1])
    param_buf[0] = 0x37
    self.set_params(_GAM_VRN3, param_mv[:1])
    param_buf[0] = 0x35
    self.set_params(_GAM_VRN4, param_mv[:1])
    param_buf[0] = 0x3F
    self.set_params(_GAM_VRN5, param_mv[:1])

    param_buf[0] = 0x0B
    self.set_params(_GAM_PKP0, param_mv[:1])
    param_buf[0] = 0x14
    self.set_params(_GAM_PKP1, param_mv[:1])
    param_buf[0] = 0x1A
    self.set_params(_GAM_PKP2, param_mv[:1])
    param_buf[0] = 0x0A
    self.set_params(_GAM_PKP3, param_mv[:1])
    param_buf[0] = 0x14
    self.set_params(_GAM_PKP4, param_mv[:1])
    param_buf[0] = 0x17
    self.set_params(_GAM_PKP5, param_mv[:1])
    param_buf[0] = 0x16
    self.set_params(_GAM_PKP6, param_mv[:1])
    param_buf[0] = 0x1B
    self.set_params(_GAM_PKP7, param_mv[:1])
    param_buf[0] = 0x04
    self.set_params(_GAM_PKP8, param_mv[:1])
    param_buf[0] = 0x0A
    self.set_params(_GAM_PKP9, param_mv[:1])
    param_buf[0] = 0x16
    self.set_params(_GAM_PKP10, param_mv[:1])

    param_buf[0] = 0x0B
    self.set_params(_GAM_PKN0, param_mv[:1])
    param_buf[0] = 0x14
    self.set_params(_GAM_PKN1, param_mv[:1])
    param_buf[0] = 0x1B
    self.set_params(_GAM_PKN2, param_mv[:1])
    param_buf[0] = 0x0A
    self.set_params(_GAM_PKN3, param_mv[:1])
    param_buf[0] = 0x08
    self.set_params(_GAM_PKN4, param_mv[:1])
    param_buf[0] = 0x07
    self.set_params(_GAM_PKN5, param_mv[:1])
    param_buf[0] = 0x06
    self.set_params(_GAM_PKN6, param_mv[:1])
    param_buf[0] = 0x07
    self.set_params(_GAM_PKN7, param_mv[:1])
    param_buf[0] = 0x04
    self.set_params(_GAM_PKN8, param_mv[:1])
    param_buf[0] = 0x0A
    self.set_params(_GAM_PKN9, param_mv[:1])
    param_buf[0] = 0x15
    self.set_params(_GAM_PKN10, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(0xFF, param_mv[:1])

    param_buf[0] = 0x00
    self.set_params(_SLPOUT, param_mv[:1])

    time.sleep_ms(120)  # NOQA

    param_buf[0] = 0x00
    self.set_params(_DISPON, param_mv[:1])

    time.sleep_ms(100)  # NOQA
