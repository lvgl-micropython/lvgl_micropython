from micropython import const  # NOQA


_CONFIG_START_REG = const(0x8047)
_CONFIG_VERSION_POS = const(0x00)
_X_OUTPUT_MAX_LOW_POS = const(0x01)
_X_OUTPUT_MAX_HIGH_POS = const(0x02)
_Y_OUTPUT_MAX_LOW_POS = const(0x03)
_Y_OUTPUT_MAX_HIGH_POS = const(0x04)
# Touch Number 0x05
# Module_Switch1 0x06
# Module_Switch2 0x07
# Shake_Count 0x08
# Filter 0x09
# Large_Touch 0x0A
_NOISE_REDUCTION_POS = const(0x0B)
_TOUCH_PRESS_LEVEL_POS = const(0x0C)
_TOUCH_LEAVE_LEVEL_POS = const(0x0D)
# Low_Power_Control 0x0E
# Refresh_Rate 0x0F
# x_threshold 0x10
# y_threshold 0x11
# X_Speed_Limit 0x12
# y_Speed_Limit 0x13
_VER_SPACE_POS = const(0x14)  # const(0x805B)  # low 4 bits are bottom and hight is top
_HOR_SPACE_POS = const(0x15)  # const(0x805C)  # low 4 bits is right and high is left

_CONFIG_CHKSUM_REG = const(0x80FF)
_CONFIG_FRESH_REG = const(0x8100)
# 0-15 * 32


class GT911Extension(object):

    def __init__(self, indev, i2c):
        self._indev = indev
        self._i2c = i2c

        self._config_data = bytearray(186)
        self._config_mv = memoryview(self._config_data)

        self._indev._read_reg(_CONFIG_START_REG, buf=self._config_mv[2:])

    @property
    def width(self):
        return (
            (self._config_data[_X_OUTPUT_MAX_HIGH_POS + 2] << 8) |
            self._config_data[_X_OUTPUT_MAX_LOW_POS + 2]
        )

    @width.setter
    def width(self, value):
        self._config_data[_X_OUTPUT_MAX_LOW_POS + 2] = value & 0xFF
        self._config_data[_X_OUTPUT_MAX_HIGH_POS + 2] = (value >> 8) & 0xFF

    @property
    def height(self):
        return (
            (self._config_data[_Y_OUTPUT_MAX_HIGH_POS + 2] << 8) |
            self._config_data[_Y_OUTPUT_MAX_LOW_POS + 2]
        )

    @height.setter
    def height(self, value):
        self._config_data[_Y_OUTPUT_MAX_LOW_POS + 2] = value & 0xFF
        self._config_data[_Y_OUTPUT_MAX_HIGH_POS + 2] = (value >> 8) & 0xFF

    @property
    def noise_reduction(self):
        return self._config_data[_NOISE_REDUCTION_POS + 2] & 0x0F

    @noise_reduction.setter
    def noise_reduction(self, value):
        upper_val = self._config_data[_NOISE_REDUCTION_POS + 2] >> 4
        self._config_data[_NOISE_REDUCTION_POS + 2] = (upper_val << 4) | (value & 0x0F)

    @property
    def touch_press_level(self):
        return self._config_data[_TOUCH_PRESS_LEVEL_POS + 2]

    @touch_press_level.setter
    def touch_press_level(self, value):
        self._config_data[_TOUCH_PRESS_LEVEL_POS + 2] = value & 0xFF

    @property
    def touch_leave_level(self):
        return self._config_data[_TOUCH_LEAVE_LEVEL_POS + 2]

    @touch_leave_level.setter
    def touch_leave_level(self, value):
        self._config_data[_TOUCH_LEAVE_LEVEL_POS + 2] = value & 0xFF

    @property
    def pad_left(self):
        return self._config_data[_HOR_SPACE_POS + 2] >> 4

    @pad_left.setter
    def pad_left(self, value):
        self._config_data[_HOR_SPACE_POS + 2] = (value << 4) | self.pad_right

    @property
    def pad_right(self):
        return self._config_data[_HOR_SPACE_POS + 2] & 0xF

    @pad_right.setter
    def pad_right(self, value):
        self._config_data[_HOR_SPACE_POS + 2] = (self.pad_left << 4) | (value & 0xF)

    @property
    def pad_top(self):
        return self._config_data[_VER_SPACE_POS + 2] >> 4

    @pad_top.setter
    def pad_top(self, value):
        self._config_data[_VER_SPACE_POS + 2] = (value << 4) | self.pad_bottom

    @property
    def pad_bottom(self):
        return self._config_data[_VER_SPACE_POS + 2] & 0xF

    @pad_bottom.setter
    def pad_bottom(self, value):
        self._config_data[_VER_SPACE_POS + 2] = (self.pad_top << 4) | (value & 0xF)

    def save(self):
        checksum = ((~sum(self._config_data[2:])) + 1) & 0xFF

        self._config_data[0] = _CONFIG_CHKSUM_REG >> 8
        self._config_data[1] = _CONFIG_CHKSUM_REG & 0xFF
        self._indev._write_reg(_CONFIG_CHKSUM_REG, buf=self._config_mv)

        self._config_data[2] = checksum
        self._indev._write_reg(_CONFIG_CHKSUM_REG, buf=self._config_mv[:3])

        self._config_data[0] = _CONFIG_FRESH_REG >> 8
        self._config_data[1] = _CONFIG_FRESH_REG & 0xFF
        self._config_data[2] = 0x01
        self._indev._write_reg(_CONFIG_FRESH_REG, buf=self._config_mv[:3])

        self._indev.hw_reset()
