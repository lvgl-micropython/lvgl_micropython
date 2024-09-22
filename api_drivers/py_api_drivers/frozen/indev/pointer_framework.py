import lvgl as lv  # NOQA
import _indev_base
import micropython  # NOQA
from lcd_utils import remap as _remap  # NOQA

remap = _remap


class PointerDriver(_indev_base.IndevBase):

    def __init__(self, touch_cal=None, startup_rotation=lv.DISPLAY_ROTATION._0, debug=False):  # NOQA
        self._last_x = -1
        self._last_y = -1
        self._last_state = self.RELEASED

        super().__init__(debug=debug)

        if touch_cal is None:
            from touch_cal_data import TouchCalData

            touch_cal = TouchCalData(f'{self.__class__.__name__}_{self.id}')

        self._cal = touch_cal
        self._orig_width = self._width
        self._orig_height = self._height
        self._set_type(lv.INDEV_TYPE.POINTER)  # NOQA
        self._cal_running = None
        self._startup_rotation = startup_rotation

        self._indev_drv.enable(True)

    def enable_input_priority(self):
        self._indev_drv.set_mode(lv.INDEV_MODE.EVENT)
        self.__timer = lv.timer_create(self.__ip_callback, 33, None)  # NOQA
        self.__timer.set_repeat_count(-1)

    def __ip_callback(self, _):
        self.read()
        last_state = self._last_state

        while self._last_state == self.PRESSED:
            lv.refr_now(self._disp_drv)
            self.read()

        if last_state == self.PRESSED:
            lv.refr_now(self._disp_drv)

    def __cal_callback(self, alphaX, betaX, deltaX, alphaY, betaY, deltaY):
        self._cal.alphaX = alphaX
        self._cal.betaX = betaX
        self._cal.deltaX = deltaX
        self._cal.alphaY = alphaY
        self._cal.betaY = betaY
        self._cal.deltaY = deltaY
        self._cal.save()
        self._cal_running = None

    def calibrate(self, update_handler=None):
        if self._cal_running:
            return

        import time
        import touch_calibrate
        self._cal_running = touch_calibrate.TPCal(self, self.__cal_callback)
        while self._cal_running:
            if update_handler is not None:
                delay = update_handler()
                time.sleep_ms(delay)
            else:
                time.sleep_ms(33)

        self._indev_drv.set_read_cb(self._read)

    @property
    def is_calibrated(self):
        if self._cal_running:
            return False

        cal = self._cal

        return None not in (
            cal.alphaX,
            cal.betaX,
            cal.deltaX,
            cal.alphaY,
            cal.betaY,
            cal.deltaY
        )

    def _get_coords(self):
        # this method needs to be overridden.
        # the returned value from this method is going to be a tuple
        # of (state, x, y) or None if no touch even has occured
        raise NotImplementedError

    def _calc_coords(self, x, y):
        if self.is_calibrated:
            cal = self._cal
            x = int(round(x * cal.alphaX + y * cal.betaX + cal.deltaX))
            y = int(round(x * cal.alphaY + y * cal.betaY + cal.deltaY))

        if (
            self._startup_rotation == lv.DISPLAY_ROTATION._180 or  # NOQA
            self._startup_rotation == lv.DISPLAY_ROTATION._270  # NOQA
        ):
            x = self._orig_width - x - 1
            y = self._orig_height - y - 1

        if (
            self._startup_rotation == lv.DISPLAY_ROTATION._90 or  # NOQA
            self._startup_rotation == lv.DISPLAY_ROTATION._270  # NOQA
        ):
            x, y = self._orig_height - y - 1, x

        return x, y

    def _read(self, drv, data):  # NOQA
        coords = self._get_coords()
        data.continue_reading = False

        if coords is None:
            state = self.RELEASED
            x, y = self._last_x, self._last_y
        else:
            state, x, y = coords

        if None in (x, y):
            x, y = self._last_x, self._last_y

        data.point.x, data.point.y = (
            self._calc_coords(x, y)
        )

        data.state = state

        if (
            self._debug and
            (x != self._last_x or y != self._last_y or self._last_state != state)  # NOQA
        ):
            print(f'{self.__class__.__name__}(raw_x={self._last_x}, raw_y={self._last_y}, x={data.point.x}, y={data.point.y}, state={"PRESSED" if data.state else "RELEASED"})')  # NOQA

        self._last_state = state
        self._last_x, self._last_y = x, y

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

