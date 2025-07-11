# Based on the work by straga (https://github.com/straga)
# https://github.com/straga/micropython_lcd/blob/master/device/JC3248W535/driver/axs15231b/axs15231_touch.py
# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import pointer_framework


# Constants
_AXS_MAX_TOUCH_NUMBER = const(1)
_AXS_TOUCH_POINT_NUM = const(1)
I2C_ADDR = 0x3B
BITS = 8


class TouchRecord:
    def __init__(self):
        self.gesture = 0
        self.num = 0
        self.x = 0
        self.y = 0
        self.event = 0


class AXS15231(pointer_framework.PointerDriver):

    def __init__(
        self,
        device,
        touch_cal=None,
        startup_rotation=lv.DISPLAY_ROTATION._0,  # NOQA
        debug=False
    ):
        self._device = device

        super().__init__(touch_cal, startup_rotation, debug)

        self._tx_buf = bytearray(
            [0xB5, 0xAB, 0xA5, 0x5A, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00]
        )
        self._tx_mv = memoryview(self._tx_buf)

        self._rx_buf = bytearray(8)
        self._rx_mv = memoryview(self._rx_buf)

        self.__last_x = -1
        self.__last_y = -1
        self.__last_state = self.RELEASED

    def _read_data(self):
        self._device.write(self._tx_mv)
        self._device.read(buf=self._rx_mv)

        touch_points = []
        data = self._rx_buf

        num_points = data[_AXS_TOUCH_POINT_NUM]

        if num_points and num_points <= _AXS_MAX_TOUCH_NUMBER:
            for i in range(num_points):
                offset = i * 6
                record = TouchRecord()
                record.gesture = data[offset]
                record.num = data[offset + 1]
                record.x = ((data[offset + 2] & 0x0F) << 8) | data[offset + 3]
                record.y = ((data[offset + 4] & 0x0F) << 8) | data[offset + 5]
                record.event = (data[offset + 2] >> 6) & 0x03
                touch_points.append(record)

        return touch_points

    def _get_coords(self):
        touch_data = self._read_data()

        if touch_data:
            self.__last_x = touch_data[0].x
            self.__last_y = touch_data[0].y

            if touch_data[0].event == 1:
                self.__last_state = self.RELEASED

            else:
                self.__last_state = self.PRESSED

        return self.__last_state, self.__last_x, self.__last_y
