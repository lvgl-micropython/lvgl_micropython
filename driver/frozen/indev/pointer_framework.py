import lvgl as lv  # NOQA


def _remap(value, old_min, old_max, new_min, new_max):
    return int((((value - old_min) * (new_max - new_min)) / (old_max - old_min)) + new_min)


class PointerDriver:
    _instance_counter = 1

    PRESSED = lv.INDEV_STATE.PRESSED
    RELEASED = lv.INDEV_STATE.RELEASED

    def get_width(self):
        return self._width

    def get_height(self):
        return self._height

    def get_rotation(self):
        return self._disp_drv.get_rotation()

    def __init__(self, touch_cal=None):  # NOQA
        self.__class__._instance_counter += 1
        self.id = self.__class__._instance_counter

        if not lv.is_initialized():
            lv.init()

        disp = lv.display_get_default()

        if disp is None:
            raise RuntimeError(
                'the display driver must be initilized '
                'before the pointer driver'
            )

        self._disp_drv = disp

        width = self._disp_drv.get_physical_horizontal_resolution()
        height = self._disp_drv.get_physical_vertical_resolution()

        self._last_x = -1
        self._last_y = -1
        self._current_state = self.RELEASED

        self._height = height
        self._width = width

        if touch_cal is None:
            from touch_cal_data import TouchCalData

            touch_cal = TouchCalData(f'{self.__class__.__name__}_{self.id}')

        self._config = touch_cal

        indev_drv = lv.indev_create()
        indev_drv.init()  # NOQA
        indev_drv.set_type(lv.INDEV_TYPE.POINTER)
        indev_drv.set_read_cb(self._read)
        indev_drv.set_driver_data(self)
        indev_drv.set_display(disp)
        self._indev_drv = indev_drv

    def calibrate(self):
        import touch_calibrate

        self._disp_drv.set_default()
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

    def get_type(self):
        return self._indev_drv.get_type()

    def read(self):
        self._indev_drv.read()

    def send_event(self, code, param):
        return self._indev_drv.send_event(code, param)

    def remove_event(self, index):
        return self._indev_drv.remove_event(index)

    def get_event_dsc(self, index):
        return self._indev_drv.get_event_dsc(index)

    def get_event_count(self):
        return self._indev_drv.get_event_count()

    def add_event_cb(self, event_cb, filter, user_data):
        self._indev_drv.add_event_cb(event_cb, filter, user_data)

    def search_obj(self, point):
        return self._indev_drv.search_obj(point)

    def delete_read_timer(self):
        self._indev_drv.delete_read_timer()

    def get_read_timer(self):
        return self._indev_drv.get_read_timer()

    def get_active_obj(self):
        return self._indev_drv.get_active_obj()

    def wait_release(self):
        self._indev_drv.wait_release()

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

    def get_state(self):
        return self._indev_drv.get_state()

    def enable(self, en):
        self._indev_drv.enable(en)

    def get_group(self):
        return self._indev_drv.get_group()

    def set_group(self, group):
        self._indev_drv.set_group(group)

    def set_cursor(self, cur_obj):
        self._indev_drv.set_cursor(cur_obj)

    def reset_long_press(self):
        self._indev_drv.reset_long_press()

    def reset(self, obj):
        self._indev_drv.reset(obj)

    def get_disp(self):
        return self._disp_drv

    @staticmethod
    def active():
        indev = lv.indev_active()
        return indev.get_driver_data()
