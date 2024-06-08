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

        super().__init__(debug=debug)

        if touch_cal is None:
            from touch_cal_data import TouchCalData

            touch_cal = TouchCalData(f'{self.__class__.__name__}_{self.id}')

        self._cal = touch_cal
        self._orig_width = self._width
        self._orig_height = self._height
        self._set_type(lv.INDEV_TYPE.POINTER)  # NOQA
        self.__cal_running = None

    def __cal_callback(self, alphaX, betaX, deltaX, alphaY, betaY, deltaY):
        self._cal.alphaX = alphaX
        self._cal.betaX = betaX
        self._cal.deltaX = deltaX
        self._cal.alphaY = alphaY
        self._cal.betaY = betaY
        self._cal.deltaY = deltaY
        self._cal.save()
        self.__cal_running = None

    def calibrate(self, update_handler=None):
        if self.__cal_running:
            return

        import time
        import touch_calibrate
        self.__cal_running = touch_calibrate.TPCal(self, self.__cal_callback)
        while self.__cal_running:
            if update_handler is not None:
                delay = update_handler()
                time.sleep_ms(delay)
            else:
                time.sleep_ms(33)

        self._indev_drv.set_read_cb(self._read)

    @property
    def is_calibrated(self):
        if self.__cal_running:
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
            xpos = int(round(x * cal.alphaX + y * cal.betaX + cal.deltaX))
            ypos = int(round(x * cal.alphaY + y * cal.betaY + cal.deltaY))
            return xpos, ypos

        return x, y

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
            print(f'{self.__class__.__name__}(raw_x={self._last_x}, raw_y={self._last_y}, x={data.point.x}, y={data.point.y}, state={"PRESSED" if data.state else "RELEASED"})')

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

