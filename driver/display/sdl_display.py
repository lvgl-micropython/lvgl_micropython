import lvgl as lv  # NOQA
import display_driver_framework


class SDL(display_driver_framework.DisplayDriver):

    def __init__(
        self,
        display_width=480,
        display_height=320,
        offset_x=0,
        offset_y=0,
        color_space=lv.COLOR_FORMAT.RGB888,
    ):

        super().__init__(
            data_bus=None,
            display_width=display_width,
            display_height=display_height,
            frame_buffer1=None,
            frame_buffer2=None,
            reset_pin=None,
            reset_state=display_driver_framework.STATE_HIGH,
            power_pin=None,
            power_on_state=display_driver_framework.STATE_HIGH,
            backlight_pin=None,
            backlight_on_state=display_driver_framework.STATE_HIGH,
            offset_x=offset_x,
            offset_y=offset_y,
            color_byte_order=display_driver_framework.BYTE_ORDER_RGB,
            color_space=color_space,
            rgb565_byte_swap=False
        )

        try:
            import task_handler  # NOQA

            self._task_handler = task_handler.TaskHandler()

        except ImportError:
            self._task_handler = None

        self._disp_drv = lv.sdl_window_create(self.display_width, self.display_height)

        buf_size = display_width * display_height
        buf_size *= lv.color_format_get_size(
            self._disp_drv.get_color_format()
        )

        buf1 = self._disp_drv.buf_1
        buf2 = self._disp_drv.buf_2

        if buf1.data_size > 0:
            buf1 = buf1.data.__dereference__(buf_size)
        else:
            buf1 = None
        if buf2.data_size > 0:
            buf2 = buf2.data.__dereference__(buf_size)
        else:
            buf2 = None

        self._frame_buffer1 = buf1
        self._frame_buffer2 = buf2

        self._disp_drv.set_offset(self._offset_x, self._offset_y)

    def set_offset(self, x, y):
        rot90 = lv.DISPLAY_ROTATION._90  # NOQA
        rot270 = lv.DISPLAY_ROTATION._270  # NOQA

        if self._rotation in (rot90, rot270):
            x, y = y, x

        self._offset_x, self._offset_y = x, y
        self._disp_drv.set_offset(x, y)

    def invert_colors(self, value):
        pass

    invert_colors = property(None, invert_colors)

    def set_rotation(self, value):
        self._disp_drv.set_orientation(value)
        self._rotation = value

    def init(self):
        self._initilized = True

    def set_params(self, cmd, params=None):
        pass

    def get_params(self, cmd, params):
        pass

    def get_power(self):
        return True

    def delete(self):
        self._disp_drv.delete()

    def __del__(self):
        self._disp_drv.delete()

    def reset(self):
        pass

    def get_backlight(self):
        return 100.0

    def _dummy_set_memory_location(self, *_, **__):  # NOQA
        return 0

    def _set_memory_location(self, x1, y1, x2, y2):
        return 0

    def _flush_cb(self, _, area, color_p):
        pass

    def _flush_ready_cb(self):
        pass

    def _madctl(self, colormode, rotations, rotation=None):
        return 0




