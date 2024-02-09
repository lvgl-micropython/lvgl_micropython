# LVGL indev driver for evdev mouse device
# (for the unix micropython port)

import struct
import select
import touch_framework
import lvgl as lv  # NOQA


class EvdevMouse(touch_framework.TouchDriver):
    def __init__(self, device='/dev/input/mice'):
        # Open evdev and initialize members
        self._evdev = open(device, 'rb')
        self._poll = select.poll()
        self._poll.register(self._evdev.fileno())
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
        point = lv.point_t(dict(x=x, y=y))
        for cursor in self.cursors:
            if cursor(point):
                break

        return x, y

    def delete(self):
        self._evdev.close()
        if self.cursor and hasattr(self.cursor, 'delete'):
            self.cursor.delete()
        self._indev_drv.enable(False)


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
