import lvgl as lv  # NOQA


def _remap(value, old_min, old_max, new_min, new_max):
    return int((((value - old_min) * (new_max - new_min)) / (old_max - old_min)) + new_min)


class PointerDriver:
    _instance_counter = 1

    def get_width(self):
        return self._width

    def get_height(self):
        return self._height

    def get_rotation(self):
        return self._disp_drv.get_rotation()

    def add_cursor(self, cursor):
        if cursor not in self._cursors:
            self._cursors.append(cursor)

    def remove_cursor(self, cursor):
        if cursor in self._cursors:
            self._cursors.remove(cursor)


    def __init__(self, touch_cal=None):  # NOQA
        self.__class__._instance_counter += 1
        self.id = self.__class__._instance_counter
        self._cursors = []

        if not lv.is_initialized():
            lv.init()

        disp = lv.display_get_default()

        if disp is None:
            raise RuntimeError(
                'the display driver must be initilized before the touch driver'
            )

        self._disp_drv = disp.get_driver_data()

        width = self._disp_drv.get_physical_horizontal_resolution()
        height = self._disp_drv.get_physical_vertical_resolution()

        self._last_x = -1
        self._last_y = -1
        self._current_state = lv.INDEV_STATE.RELEASED

        self._height = height
        self._width = width

        if touch_cal is None:
            from touch_cal_data import TouchCalData

            touch_cal = TouchCalData(f'{self.__class__.__name__}_{self.id}')

        self._config = touch_cal

        indev_drv = lv.indev_t()
        indev_drv.init()  # NOQA
        indev_drv.type = lv.INDEV_TYPE.POINTER
        indev_drv.read_cb = self._read
        self._indev_drv = indev_drv.register()  # NOQA
        self._indev_drv.set_driver_data(self)
        self._indev_drv.set_disp(disp)

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
        # the returned value from this method is going to be a tuple of x, y
        # or None if no touch even has occured
        raise NotImplementedError

    def _read(self, drv, data):  # NOQA
        coords = self._get_coords()

        if coords is None:  # ignore no touch & multi touch
            if self._current_state != lv.INDEV_STATE.RELEASED:
                self._current_state = lv.INDEV_STATE.RELEASED
                res = True
            else:
                res = False

            data.point.x = self._last_x
            data.point.y = self._last_y
            data.state = self._current_state
            # print("raw(x={0}, y={1}) point(x={2} y={3})".format(-1, -1, data.point.x, data.point.y))  # NOQA
            data.continue_reading = False
            return res

        x, y = coords

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

        if orientation == lv.DISPLAY_ROTATION._0:
            xpos = _remap(x, left, right, 0, self._width)
            ypos = _remap(y, top, bottom, 0, self._height)
        elif orientation == lv.DISPLAY_ROTATION._90:
            xpos = _remap(y, top, bottom, 0, self._width)
            ypos = _remap(x, right, left, 0, self._height)
        elif orientation == lv.DISPLAY_ROTATION._270:
            xpos = _remap(x, right, left, 0, self._width)
            ypos = _remap(y, bottom, top, 0, self._height)
        elif orientation == lv.DISPLAY_ROTATION._180:
            xpos = _remap(y, bottom, top, 0, self._width)
            ypos = _remap(x, left, right, 0, self._height)
        else:
            raise RuntimeError

        self._last_x = data.point.x = xpos
        self._last_y = data.point.y = ypos
        self._current_state = data.state = lv.INDEV_STATE.PRESSED

        data.continue_reading = True
        # print("raw(x={0}, y={1}) point(x={2} y={3})".format(x, y, data.point.x, data.point.y))  # NOQA
        point = lv.point_t(dict(x=xpos, y=ypos))
        for cursor in self._cursors:
            if cursor(point):
                break

        return True

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

    def set_button_points(self, points):
        self._indev_drv.set_button_points(points)

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
