import lvgl as lv  # NOQA
import _indev_base


def _remap(value, old_min, old_max, new_min, new_max):
    return int((((value - old_min) * (new_max - new_min)) / (old_max - old_min)) + new_min)


class PointerDriver(_indev_base.IndevBase):

    def __init__(self, touch_cal=None):  # NOQA
        self._last_x = -1
        self._last_y = -1

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

    def _read(self, drv, data):  # NOQA
        coords = self._get_coords()

        if coords is None:  # ignore no touch & multi touch
            if self._current_state != self.RELEASED:
                self._current_state = self.RELEASED

            data.point.x = self._last_x
            data.point.y = self._last_y
            data.state = self._current_state
            # print("raw(x={0}, y={1}) point(x={2} y={3})".format(-1, -1, data.point.x, data.point.y))  # NOQA
            data.continue_reading = False
            return

        state, x, y = coords

        if None not in (x, y):
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

            if rotation == lv.DISPLAY_ROTATION._0:  # NOQA
                xpos = _remap(x, left, right, 0, self._orig_width)
                ypos = _remap(y, top, bottom, 0, self._orig_height)
            elif rotation == lv.DISPLAY_ROTATION._90:  # NOQA
                xpos = _remap(y, bottom, top, 0, self._orig_height)
                ypos = _remap(x, left, right, 0,  self._orig_width)
            elif rotation == lv.DISPLAY_ROTATION._180:  # NOQA
                xpos = _remap(x, right, left, 0, self._orig_width)
                ypos = _remap(y, bottom, top, 0, self._orig_height)
            elif rotation == lv.DISPLAY_ROTATION._270:  # NOQA
                xpos = _remap(y, top, bottom, 0, self._orig_height)
                ypos = _remap(x, right, left, 0, self._orig_width)
            else:
                raise RuntimeError

            self._last_x = xpos
            self._last_y = ypos

        data.state = self._current_state
        data.point.x = self._last_x
        data.point.y = self._last_y

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

