import display_driver_framework
import lvgl as lv

STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class RGBDisplay(display_driver_framework.DisplayDriver):

    def init(self):
        self._disp_drv.sw_rotate = 1
        display_driver_framework.DisplayDriver.init(self)

    def _dummy_set_memory_location(self, *_, **__):
        return -1

    def _flush_cb(self, _, area, color_p):
        x1 = area.x1 + self._offset_x
        x2 = area.x2 + self._offset_x

        y1 = area.y1 + self._offset_y
        y2 = area.y2 + self._offset_y

        size = (
            (x2 - x1 + 1) *
            (y2 - y1 + 1) *
            lv.color_format_get_size(self._color_space)
        )

        # we have to use the __dereference__ method because this method is
        # what converts from the C_Array object the binding passes into a
        # memoryview object that can be passed to the bus drivers
        data_view = color_p.__dereference__(size)

        self._data_bus.tx_color(
            -1, data_view, x1, y1, x2, y2,
            rotation=self._rotation,
            last_flush=self._disp_drv.flush_is_last()
        )
