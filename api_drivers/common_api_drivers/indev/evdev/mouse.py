# Copyright (c) 2024 - 2025 Kevin G. Schlosser

'''
# LVGL indev driver for evdev mouse device
# (for the unix micropython port)

import struct
import select

import os

import lcd_bus
import pointer_framework
from micropython import const  # NOQA
import lvgl as lv  # NOQA

#
# _REL_X			0x00
# _REL_Y			0x01
# _REL_Z			0x02
# _REL_RX			0x03
# _REL_RY			0x04
# _REL_RZ			0x05
# _REL_HWHEEL		0x06
# _REL_DIAL		0x07
# _REL_WHEEL		0x08
# _REL_WHEEL_HI_RES	0x0C
# _REL_HWHEEL_HI_RES	0x0C
#
# _ABS_X			0x00
# _ABS_Y			0x01
# _ABS_Z			0x02
# _ABS_RX			0x03
# _ABS_RY			0x04
# _ABS_RZ			0x05
# _ABS_THROTTLE		0x06
# _ABS_RUDDER		0x07
# _ABS_WHEEL		0x08
# _ABS_GAS			0x09
# _ABS_BRAKE		0x0a
# _ABS_HAT0X		0x10
# _ABS_HAT0Y		0x11
# _ABS_HAT1X		0x12
# _ABS_HAT1Y		0x13
# _ABS_HAT2X		0x14
# _ABS_HAT2Y		0x15
# _ABS_HAT3X		0x16
# _ABS_HAT3Y		0x17
# _ABS_PRESSURE		0x18
# _ABS_DISTANCE		0x19
# _ABS_TILT_X		0x1a
# _ABS_TILT_Y		0x1b
# _ABS_TOOL_WIDTH		0x1c
# _ABS_VOLUME		0x20
# _ABS_PROFILE		0x21
#
# _ABS_MT_POSITION_X	0x35	/* Center X touch position */
# _ABS_MT_POSITION_Y	0x36	/* Center Y touch position */
# _ABS_MT_TOOL_TYPE	0x37	/* Type of touching device */
# _ABS_MT_TOOL_X		0x3c	/* Center X tool position */
# _ABS_MT_TOOL_Y		0x3d	/* Center Y tool position */


class EvdevMouseDriver(pointer_framework.PointerDriver):
    def __init__(self, device):
        self.device = device
        self._last_mouse_x = 0
        self._last_mouse_y = 0

        super().__init__()

    def _get_key(self, _, data):
        # BTN_RIGHT
        # BTN_MIDDLE
        # BTN_SIDE
        # BTN_EXTRA
        # BTN_FORWARD
        # BTN_BACK
        # BTN_TASK
        pass


    def _get_wheel(self, _, data):
        # REL_HWHEEL
        # REL_WHEEL
        #
        # REL_WHEEL_HI_RES
        # REL_HWHEEL_HI_RES
        pass

    def _get_coords(self):
        # event = self.device.read()
        #
        # if event is None:
        #     return None
        #
        # if event.type == _EV_KEY:
        #     key = event.code
        #     state = event.code
        #
        # elif event.type == _EV_REL:
        #
        #     pos = event.value
        #
        # if not self._poll.poll()[0][1] & select.POLLIN:
        #     return None
        #
        # EV_KEY
        # BTN_LEFT
        # REL_X
        # REL_Y
        #
        # if (mouse_data[0] & 1) != 1:
        #     return None
        #
        # # Data is relative, update coordinates
        # x = self._last_mouse_x
        # y = self._last_mouse_y
        #
        # x += mouse_data[1]
        # y -= mouse_data[2]
        #
        # # Handle coordinate overflow cases
        # x = max(min(x, self.get_width() - 1), 0)
        # y = max(min(y, self.get_height() - 1), 0)
        #
        # # Draw cursor, if needed
        #
        # return state, x, y

        pass

    def delete(self):
        # self._evdev.close()
        # if self.cursor and hasattr(self.cursor, 'delete'):
        #     self.cursor.delete()
        # self._indev_drv.enable(False)
        pass



# evdev driver for mouse
class EvdevMouseWheel(touch_framework.TouchDriver):
    def __init__(self, cursor=None, device='/dev/input/mice'):
        # Open evdev and initialize members
        self._evdev = open(device, 'rb')
        self._poll = select.poll()
        self._poll.register(self._evdev.fileno())
        self.cursor = cursor
        super().__init__()

    def _get_coords(self):
        if not self._poll.poll()[0][1] & select.POLLIN:
            return None

        # Read and parse evdev mouse data
        mouse_data = struct.unpack('bbb', self._evdev.read(3))

        if (mouse_data[0] & 1) != 1:
            return None

        # Data is relative, update coordinates
        x = self._last_x
        y = self._last_y

        x += mouse_data[1]
        y -= mouse_data[2]

        # Handle coordinate overflow cases
        x = max(min(x, self.get_width() - 1), 0)
        y = max(min(y, self.get_height() - 1), 0)

        # Draw cursor, if needed
        if self.cursor:
            self.cursor(x, y)

        return x, y

    def delete(self):
        self._evdev.close()
        if self.cursor and hasattr(self.cursor, 'delete'):
            self.cursor.delete()
        self._indev_drv.enable(False)

'''