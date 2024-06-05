import lvgl as lv  # NOQA
import _indev_base
import micropython  # NOQA
from lcd_utils import remap as _remap  # NOQA

remap = _remap


class PointerDriver(_indev_base.IndevBase):

    def __init__(self, touch_cal=None, debug=False):  # NOQA
        self._last_x = -1
        self._last_y = -1
        self.__debug = debug

        super().__init__()

        if touch_cal is None:
            from touch_cal_data import TouchCalData

            touch_cal = TouchCalData(f'{self.__class__.__name__}_{self.id}')

        self._cal = touch_cal

        self._set_type(lv.INDEV_TYPE.POINTER)  # NOQA

        self._orig_width = self._disp_drv.get_horizontal_resolution()
        self._orig_height = self._disp_drv.get_vertical_resolution()
        self._rotation = self._disp_drv.get_rotation()

        if self._rotation in (lv.DISPLAY_ROTATION._90, lv.DISPLAY_ROTATION._270):  # NOQA
            self._orig_height, self._orig_width = self._orig_width, self._orig_height  # NOQA

        self._disp_drv.add_event_cb(self._on_size_change, lv.EVENT.RESOLUTION_CHANGED, None)  # NOQA

    def _on_size_change(self, e):
        self._width = self._disp_drv.get_horizontal_resolution()
        self._height = self._disp_drv.get_vertical_resolution()
        self._rotation = self._disp_drv.get_rotation()

    def calibrate(self):
        import touch_calibrate

        self._py_disp_drv.set_default()
        self._cal.reset()
        touch_calibrate.run()

    @property
    def is_calibrated(self):
        left = self._cal.left
        top = self._cal.top
        right = self._cal.right
        bottom = self._cal.bottom

        return None not in (left, top, right, bottom)

    def _get_coords(self):
        # this method needs to be overridden.
        # the returned value from this method is going to be a tuple
        # of (state, x, y) or None if no touch even has occured
        raise NotImplementedError

    def _calc_coords(self, x, y):
        cal = self._cal
        rotation = self._rotation
        left = cal.left
        right = cal.right
        top = cal.top
        bottom = cal.bottom

        if left is None:
            left = 0
        if right is None:
            right = self._orig_width
        if top is None:
            top = 0
        if bottom is None:
            bottom = self._orig_height

        if rotation == lv.DISPLAY_ROTATION._90:  # NOQA
            left, right, top, bottom = bottom, top, left, right
            x, y = y, x
        elif rotation == lv.DISPLAY_ROTATION._180:  # NOQA
            left, right, top, bottom = right, left, bottom, top
        elif rotation == lv.DISPLAY_ROTATION._270:  # NOQA
            left, right, top, bottom = top, bottom, right, left
            x, y = y, x

        xpos = _remap(x, left, right, 0, self._width)
        ypos = _remap(y, top, bottom, 0, self._height)
        return xpos, ypos

    def _read(self, drv, data):  # NOQA
        coords = self._get_coords()

        if coords is None:
            state = self.RELEASED
            x, y = self._last_x, self._last_y
        else:
            state, x, y = coords

        if None not in (x, y):
            self._last_x, self._last_y = x, y
            if coords is None:
                data.continue_reading = False
            else:
                data.continue_reading = True
        else:
            data.continue_reading = False

        data.point.x, data.point.y = (
            self._calc_coords(self._last_x, self._last_y)
        )
        data.state = state

        if self.__debug:
            print(
                f'{self.__class__.__name__}('
                f'raw_x={self._last_x}, '
                f'raw_y={self._last_y}, '
                f'x={data.point.x}, '
                f'y={data.point.y}, '
                f'state={"PRESSED" if data.state else "RELEASED"})'
            )

    def get_vect(self, point):
        self._indev_drv.get_vect(point)

    def get_scroll_obj(self):
        return self._indev_drv.get_scroll_obj()

    def get_scroll_dir(self):
        return self._indev_drv.get_scroll_dir()

    def get_gesture_dir(self):
        return self._indev_drv.get_gesture_dir()

    def get_point(self, point):
        self._indev_drv.get_point(point)

    def set_cursor(self, cur_obj):
        self._indev_drv.set_cursor(cur_obj)

    def reset_long_press(self):
        self._indev_drv.reset_long_press()

