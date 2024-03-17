import lvgl as lv  # NOQA


lv_indev_get_driver_data
lv_indev_get_user_data
lv_indev_


class ButtonDriver:
    _instance_counter = 1

    def __init__(self, touch_cal=None):  # NOQA
        self.__class__._instance_counter += 1
        self.id = self.__class__._instance_counter
        self._cursors = []

        if not lv.is_initialized():
            lv.init()

        disp = lv.display_get_default()

        if disp is None:
            raise RuntimeError(
                'the display driver must be initilized before the button driver'
            )

        self._disp_drv = disp.get_driver_data()

        self._last_button = -1
        self._button_points = []
        self._current_state = lv.INDEV_STATE.RELEASED

        indev_drv = lv.indev_t()
        indev_drv.init()  # NOQA
        indev_drv.type = lv.INDEV_TYPE.BUTTON
        indev_drv.read_cb = self._read
        self._indev_drv = indev_drv.register()  # NOQA
        self._indev_drv.set_driver_data(self)
        self._indev_drv.set_disp(disp)

    def set_button_points(self, *points):
        self._indev_drv.set_button_points(list(points))
        self._button_points = list(points)

    def _get_button(self):
        # this method needs to be overridden.
        # the returned value from this method is going to be a keycode
        # or None if no key event has occured
        raise NotImplementedError

    def _read(self, drv, data):  # NOQA
        button = self._get_button()

        if button is None:  # ignore no touch & multi touch
            if self._current_state != lv.INDEV_STATE.RELEASED:
                self._current_state = lv.INDEV_STATE.RELEASED
                res = True
            else:
                res = False

            data.enc_diff = self._last_button
            data.state = self._current_state
            data.continue_reading = False
            return res

        self._last_button = button
        self._current_state = data.state = lv.INDEV_STATE.PRESSED

        data.btn_id = self._last_button
        data.state = self._current_state
        data.continue_reading = True

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
