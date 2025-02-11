# Copyright (c) 2025 0ut4t1m3
# Copyright (c) 2024 - 2025 Kevin G. Schlosser


from micropython import const  # NOQA
import display_driver_framework
import lvgl as lv


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR

_MADCTL_MY = const(0x80)  # Decreasing in vertical  10000000
_MADCTL_MX = const(0x60)  # Increasing in horizontal  01100000
_MADCTL_MV = const(0x20)  # x, y = y, x    00100000
_MADCTL_MYRS = const(0x01)  # mirror y    00000001
_MADCTL_MXRS = const(0x02)  # mirror x    00000010

# Write Display Brightness
# 0 to 255
_WRDISBV = const(0x51)

# Set Color Enhance
_IMGEHCCTR = const(0x58)
_SLR_EN = const(0x04)  # Sunlight Readable
# Sunlight Readable Level values are 0, 1, and 3

# Set Color Enhance 1
_CESLRCTR_L = const(0x5A)
_CESLRCTR_H = const(0x5B)

# High Brightness Mode
# 0x00 disable
# 0x02 enable
_SETHBM = const(0xB0)
_HBM_EN = const(0x02)


class RM67162(display_driver_framework.DisplayDriver):
    _ORIENTATION_TABLE = (
        _MADCTL_MX,
        _MADCTL_MYRS | _MADCTL_MXRS | _MADCTL_MX & ~_MADCTL_MV,
        _MADCTL_MY | _MADCTL_MXRS | _MADCTL_MX,
        _MADCTL_MY | _MADCTL_MYRS | _MADCTL_MXRS
    )

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
        color_space=lv.COLOR_FORMAT.RGB888,  # NOQA
        rgb565_byte_swap=False
    ):

        self.__brightness = 0
        self.__sunlight = 0
        self.__ambient = 0x7FFF
        self.__high_brightness = 0x00

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
            color_space=color_space,
            rgb565_byte_swap=rgb565_byte_swap
        )

    def set_brightness(self, val):
        # convert the value from a 0.0 - 100.0 scale to a 0 - 255 scale
        val = int(val * 255 / 100)
        # clamp the balue so it is 0 - 255
        val = max(min(val, 255), 0)

        self.__brightness = val
        self._param_buf[0] = val

        self.set_params(_WRDISBV, self._param_mv[:1])

    def get_brightness(self):
        return round(self.__brightness / 255.0 * 100.0, 2)

    def set_ambient_light_level(self, val):
        val = int(val * 65535 / 100)
        val = max(min(val, 65535), 0)

        self.__ambient = val
        self._param_buf[0] = val >> 8
        self.set_params(_CESLRCTR_H, self._param_mv[:1])

        self._param_buf[0] = val & 0xFF
        self.set_params(_CESLRCTR_L, self._param_mv[:1])

    def get_amblent_light_level(self):
        return round(self.__ambient / 65535.0 * 100.0, 2)

    def set_sunlight_enhance(self, val):
        if val == -1:
            self.__sunlight &= ~_SLR_EN
        else:
            val = min(val, 3)
            self.__sunlight = _SLR_EN | val

        self._param_buf[0] = self.__sunlight
        self.set_params(_IMGEHCCTR, self._param_mv[:1])

    def get_sunlight_enhance(self):
        if self.__sunlight & _SLR_EN:
            return self.__sunlight & ~_SLR_EN

        return -1

    def set_high_brightness(self, val):
        if val:
            self.__high_brightness |= _HBM_EN
        else:
            self.__high_brightness &= ~_HBM_EN

        self._param_buf[0] = self.__high_brightness
        self.set_params(_SETHBM, self._param_mv[:1])

    def get_high_brightness(self):
        return bool(self.__high_brightness & _HBM_EN)
