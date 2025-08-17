# Based on the work by straga (https://github.com/straga)
# https://github.com/straga/micropython_lcd/blob/master/device/JC3248W535/driver/axs15231b/axs15231b.py
# Copyright (c) 2024 - 2025 Kevin G. Schlosser


import display_driver_framework
from micropython import const  # NOQA

import lcd_bus
import lvgl as lv  # NOQA


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR

_RASET = const(0x2B)
_CASET = const(0x2A)
_MADCTL = const(0x36)

_RAMWR = const(0x2C)
_RAMWRC = const(0x3C)

_WRITE_CMD = const(0x02)
_WRITE_COLOR = const(0x32)

_MADCTL_MH = const(0x04)  # Refresh 0=Left to Right, 1=Right to Left
_MADCTL_BGR = const(0x08)  # BGR color order
_MADCTL_ML = const(0x10)  # Refresh 0=Top to Bottom, 1=Bottom to Top

_MADCTL_MV = const(0x20)  # 0=Normal, 1=Row/column exchange
_MADCTL_MX = const(0x40)  # 0=Left to Right, 1=Right to Left
_MADCTL_MY = const(0x80)  # 0=Top to Bottom, 1=Bottom to Top

_WRDISBV = const(0x51)
_SW_RESET = const(0x01)


class AXS15231B(display_driver_framework.DisplayDriver):

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
        num_lanes = data_bus.get_lane_count()

        if isinstance(data_bus, lcd_bus.SPIBus) and num_lanes == 4:
            self.__qspi = True
            _cmd_bits = 32
        else:
            if isinstance(data_bus, lcd_bus.I80Bus) and num_lanes > 8:
                raise RuntimeError('Only 8 lanes is supported when using the I80Bus')

            self.__qspi = False
            _cmd_bits = 8

        if not isinstance(data_bus, (lcd_bus.RGBBus, lcd_bus.I80Bus, lcd_bus.SPIBus)):
            raise RuntimeError('incompatable bus driver')

        self._brightness = 0xD0

        super().__init__(
            data_bus,
            display_width,
            display_height,
            frame_buffer1,
            frame_buffer2,
            reset_pin,
            reset_state,
            power_pin,
            power_on_state,
            backlight_pin,
            backlight_on_state,
            offset_x,
            offset_y,
            color_byte_order,
            color_space,  # NOQA
            rgb565_byte_swap,
            _cmd_bits=_cmd_bits,
            _param_bits=8,
            _init_bus=True
        )

    def reset(self):
        if self._reset_pin is None:
            self.set_params(_SW_RESET)
        else:
            self._reset_pin.value(not self._reset_state)
            time.sleep_ms(10)  # NOQA
            self._reset_pin.value(self._reset_state)
            time.sleep_ms(10)  # NOQA
            self._reset_pin.value(not self._reset_state)
        time.sleep_ms(120)  # NOQA

    def init(self, type=None):  # NOQA
        if not isinstance(self._data_bus, lcd_bus.RGBBus):
            super().init(type)
        else:
            self._initilized = True

    def set_brightness(self, value):
        value = int(value / 100.0 * 255)
        value = max(0x00, min(value, 0xFF))

        self._brightness = value

        self._param_buf[0] = value
        self.set_params(_WRDISBV, self._param_mv[:1])

    def get_brightness(self):
        return round(self._brightness / 255.0 * 100.0, 1)

    def set_params(self, cmd, params=None):
        if self.__qspi:
            cmd &= 0xFF
            cmd <<= 8
            cmd |= _WRITE_CMD << 24

        self._data_bus.tx_param(cmd, params)

    def _set_memory_location(self, x1: int, y1: int, x2: int, y2: int):
        if y1 == 0:
            cmd = _RAMWR
        else:
            cmd = _RAMWRC

        param_buf = self._param_buf  # NOQA

        param_buf[0] = (x1 >> 8) & 0xFF
        param_buf[1] = x1 & 0xFF
        param_buf[2] = (x2 >> 8) & 0xFF
        param_buf[3] = x2 & 0xFF

        self._data_bus.tx_param(_CASET, self._param_mv)

        if self.__qspi:
            cmd &= 0xFF
            cmd <<= 8
            cmd |= _WRITE_COLOR << 24
        else:
            param_buf[0] = (y1 >> 8) & 0xFF
            param_buf[1] = y1 & 0xFF
            param_buf[2] = (y2 >> 8) & 0xFF
            param_buf[3] = y2 & 0xFF
            self._data_bus.tx_param(_RASET, self._param_mv)

        return cmd
