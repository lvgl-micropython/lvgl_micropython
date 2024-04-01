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

        self._config = touch_cal

        self._set_type(lv.INDEV_TYPE_POINTER)

    def calibrate(self):
        import touch_calibrate

        self._py_disp_drv.set_default()
        touch_calibrate.run()

    @property
    def is_calibrated(self):
        left = self._config.left
        top = self._config.top
        right = self._config.right
        bottom = self._config.bottom

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
                res = True
            else:
                res = False

            data.point.x = self._last_x
            data.point.y = self._last_y
            data.state = self._current_state
            # print("raw(x={0}, y={1}) point(x={2} y={3})".format(-1, -1, data.point.x, data.point.y))  # NOQA
            data.continue_reading = False
            return res

        state, x, y = coords

        if None not in (x, y):
            config = self._config
            orientation = self.get_rotation()
            left = config.left
            right = config.right
            top = config.top
            bottom = config.bottom

            if left is None:
                left = 0
            if right is None:
                right = self._width
            if top is None:
                top = 0
            if bottom is None:
                bottom = self._height

            if orientation == lv.DISPLAY_ROTATION._0:  # NOQA
                xpos = _remap(x, left, right, 0, self._width)
                ypos = _remap(y, top, bottom, 0, self._height)
            elif orientation == lv.DISPLAY_ROTATION._90:  # NOQA
                xpos = _remap(y, top, bottom, 0, self._width)
                ypos = _remap(x, right, left, 0, self._height)
            elif orientation == lv.DISPLAY_ROTATION._270:  # NOQA
                xpos = _remap(x, right, left, 0, self._width)
                ypos = _remap(y, bottom, top, 0, self._height)
            elif orientation == lv.DISPLAY_ROTATION._180:  # NOQA
                xpos = _remap(y, bottom, top, 0, self._width)
                ypos = _remap(x, left, right, 0, self._height)
            else:
                raise RuntimeError

            self._last_x = xpos
            self._last_y = ypos

        if state is not None:
            if self._current_state == state == self.RELEASED:
                res = False
                data.continue_reading = False
            else:
                res = True
                data.continue_reading = True

            self._current_state = state

        elif self._current_state == self.RELEASED:
            res = False
            data.continue_reading = False
        else:
            data.continue_reading = True
            res = True

        data.state = self._current_state
        data.point.x = self._last_x
        data.point.y = self._last_y

        return res

    def get_vect(self, point):
        lv.indev_get_vect(self._indev_drv, point)

    def get_scroll_obj(self):
        return lv.indev_get_scroll_obj(self._indev_drv)

    def get_scroll_dir(self):
        return lv.indev_get_scroll_dir(self._indev_drv)

    def get_gesture_dir(self):
        return lv.indev_get_gesture_dir(self._indev_drv)

    def get_point(self, point):
        lv.indev_get_point(self._indev_drv, point)

    def set_cursor(self, cur_obj):
        lv.indev_set_cursor(self._indev_drv, cur_obj)

    def reset_long_press(self):
        lv.indev_reset_long_press(self._indev_drv)
