from micropython import const
import pointer_framework
import i2c as _i2c

_I2C_SLAVE_ADDR = const(0x38)

# ** Maximum border values of the touchscreen pad that the chip can handle **
_MAX_WIDTH = const(800)
_MAX_HEIGHT = const(480)

# Max detectable simultaneous touch points
_MAX_TOUCH_PNTS = const(2)

# Register of the current mode
_DEV_MODE_REG = const(0x00)

# ** Possible modes as of FT6X36_DEV_MODE_REG **
_DEV_MODE_WORKING = const(0x00)
_DEV_MODE_FACTORY = const(0x04)

_DEV_MODE_MASK = const(0x70)
_DEV_MODE_SHIFT = const(4)

# Gesture ID register
_GEST_ID_REG = const(0x01)

# ** Possible values returned by FT6X36_GEST_ID_REG **
_GEST_ID_NO_GESTURE = const(0x00)
_GEST_ID_MOVE_UP = const(0x10)
_GEST_ID_MOVE_RIGHT = const(0x14)
_GEST_ID_MOVE_DOWN = const(0x18)
_GEST_ID_MOVE_LEFT = const(0x1C)
_GEST_ID_ZOOM_IN = const(0x48)
_GEST_ID_ZOOM_OUT = const(0x49)

# Status register: stores number of active touch points (0, 1, 2)
_TD_STAT_REG = const(0x02)
_TD_STAT_MASK = const(0x0F)
_TD_STAT_SHIFT = const(0x00)

# ** Touch events **
_TOUCH_EVT_FLAG_PRESS_DOWN = const(0x00)
_TOUCH_EVT_FLAG_LIFT_UP = const(0x01)
_TOUCH_EVT_FLAG_CONTACT = const(0x02)
_TOUCH_EVT_FLAG_NO_EVENT = const(0x03)

_TOUCH_EVT_FLAG_SHIFT = const(6)
_TOUCH_EVT_FLAG_MASK = const(3 << 6)

_MSB_MASK = const(0x0F)
_MSB_SHIFT = const(0)
_LSB_MASK = const(0xFF)
_LSB_SHIFT = const(0)

_P1_XH_REG = const(0x03)
_P1_XL_REG = const(0x04)
_P1_YH_REG = const(0x05)
_P1_YL_REG = const(0x06)

# ** Register reporting touch pressure - read only **
_P1_WEIGHT_REG = const(0x07)
_TOUCH_WEIGHT_MASK = const(0xFF)
_TOUCH_WEIGHT_SHIFT = const(0)

# Touch area register
_P1_MISC_REG = const(0x08)

# ** Values related to FT6X36_Pn_MISC_REG **
_TOUCH_AREA_MASK = const(0x04 << 4)
_TOUCH_AREA_SHIFT = const(0x04)

_P2_XH_REG = const(0x09)
_P2_XL_REG = const(0x0A)
_P2_YH_REG = const(0x0B)
_P2_YL_REG = const(0x0C)
_P2_WEIGHT_REG = const(0x0D)
_P2_MISC_REG = const(0x0E)

# ** Threshold for touch detection **
_TH_GROUP_REG = const(0x80)

# ** Values FT6X36_TH_GROUP_REG : threshold related **
_THRESHOLD_MASK = const(0xFF)
_THRESHOLD_SHIFT = const(0)


# ** Filter function coefficients **
_TH_DIFF_REG = const(0x85)

# Control register
_CTRL_REG = const(0x86)

# Will keep the Active mode when there is no touching
_CTRL_KEEP_ACTIVE_MODE = const(0x00)

# Switching from Active mode to Monitor mode
# automatically when there is no touching
_CTRL_KEEP_AUTO_SWITCH_MONITOR_MODE = const(0x01)

# The time period of switching from Active mode to
# Monitor mode when there is no touching
_TIME_ENTER_MONITOR_REG = const(0x87)

# Report rate in Active mode
_PERIOD_ACTIVE_REG = const(0x88)
# Report rate in Monitor mode
_PERIOD_MONITOR_REG = const(0x89)

# The value of the minimum allowed angle while Rotating gesture mode
_RADIAN_VALUE_REG = const(0x91)

# Maximum offset while Moving Left and Moving Right gesture
_OFFSET_LEFT_RIGHT_REG = const(0x92)
# Maximum offset while Moving Up and Moving Down gesture
_OFFSET_UP_DOWN_REG = const(0x93)

# Minimum distance while Moving Left and Moving Right gesture
_DISTANCE_LEFT_RIGHT_REG = const(0x94)
# Minimum distance while Moving Up and Moving Down gesture
_DISTANCE_UP_DOWN_REG = const(0x95)

# High 8-bit of LIB Version info
_LIB_VER_H_REG = const(0xA1)
# Low 8-bit of LIB Version info
_LIB_VER_L_REG = const(0xA2)

# 0x36 for ft6236; 0x06 for ft6206
_CHIPSELECT_REG = const(0x36)

_FT6206_CHIPID = const(0x06)
_FT6236_CHIPID = const(0x36)
_FT6336_CHIPID = const(0x64)
_VENDID = const(0x11)
_CHIPID_REG = const(0xA3)

_POWER_MODE_REG = const(0xA5)
_FIRMWARE_ID_REG = const(0xA6)
_RELEASECODE_REG = const(0xAF)
_PANEL_ID_REG = const(0xA8)
_OPMODE_REG = const(0xBC)

_G_MODE = const(0xA4)

_PWR_MODE = const(0xA5)

ROTATION_0 = const(-1)
ROTATION_90 = const(-2)
ROTATION_180 = const(-3)
ROTATION_270 = const(-4)


class FT6x36(pointer_framework.PointerDriver):

    def _i2c_read8(self, register_addr):
        self._i2c.read_mem(register_addr, buf=self._mv[:1])
        return self._buf[0]

    def _i2c_write8(self, register_addr, data):
        self._buf[0] = data
        self._i2c.write_mem(register_addr, self._mv[:1])

    def __init__(self, bus, touch_cal=None):  # NOQA
        self._buf = bytearray(5)
        self._mv = memoryview(self._buf)
        self._i2c = _i2c.I2CDevice(bus, _I2C_SLAVE_ADDR)

        data = self._i2c_read8(_PANEL_ID_REG)
        print("Device ID: 0x%02x" % data)
        if data != _VENDID:
            raise RuntimeError()

        data = self._i2c_read8(_CHIPID_REG)
        print("Chip ID: 0x%02x" % data)

        if data not in (_FT6206_CHIPID, _FT6236_CHIPID, _FT6336_CHIPID):
            raise RuntimeError()

        data = self._i2c_read8(_DEV_MODE_REG)
        print("Device mode: 0x%02x" % data)

        data = self._i2c_read8(_FIRMWARE_ID_REG)
        print("Firmware ID: 0x%02x" % data)

        data = self._i2c_read8(_RELEASECODE_REG)
        print("Release code: 0x%02x" % data)

        self._i2c_write8(_DEV_MODE_REG, _DEV_MODE_WORKING)
        self._i2c_write8(_PERIOD_ACTIVE_REG, 0x0E)
        self._i2c_write8(_G_MODE, 0x00)
        super().__init__(touch_cal)

    def _get_coords(self):
        buf = self._buf
        mv = self._mv

        try:
            self._i2c.read_mem(_TD_STAT_REG, buf=mv)
        except OSError:
            return None

        touch_pnt_cnt = buf[0]

        if touch_pnt_cnt != 1:
            return None

        x = (
            ((buf[1] & _MSB_MASK) << 8) |
            (buf[2] & _LSB_MASK)
        )
        y = (
            ((buf[3] & _MSB_MASK) << 8) |
            (buf[4] & _LSB_MASK)
        )
        return x, y
