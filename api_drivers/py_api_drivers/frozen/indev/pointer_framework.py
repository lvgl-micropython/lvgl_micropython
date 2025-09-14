# Copyright (c) 2024 - 2025 Kevin G. Schlosser

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
        self._startup_rotation = startup_rotation

        self._indev_drv.enable(True)

    def enable_input_priority(self):
        self._indev_drv.set_mode(lv.INDEV_MODE.EVENT)  # NOQA
        self.__timer = lv.timer_create(self.__ip_callback, 33, None)  # NOQA
        self.__timer.set_repeat_count(-1)  # NOQA

    def __ip_callback(self, _):
        self.read()
        last_state = self._last_state

        while self._last_state == self.PRESSED:
            lv.refr_now(self._disp_drv)
            self.read()

        if last_state == self.PRESSED:
            lv.refr_now(self._disp_drv)

    def calibrate(self):
        import touch_calibrate

        if touch_calibrate.calibrate(self, self._cal):  # NOQA
            self._cal.save()
            return True

        return False

    @property
    def is_calibrated(self):
        cal = self._cal

        return None not in (
            cal.alphaX,
            cal.betaX,
            cal.deltaX,
            cal.alphaY,
            cal.betaY,
            cal.deltaY,
            cal.mirrorX,
            cal.mirrorY
        )

    def _get_coords(self):
        # this method needs to be overridden.
        # the returned value from this method is going to be a tuple
        # of (state, x, y) or None if no touch even has occured
        raise NotImplementedError

    def _calc_coords(self, xt, yt):
        """
        Convert the raw touch coordinates into screen coordinates using
        the calibration data if available.
        :param xt: The raw x coordinate from the touch device
        :param yt: The raw y coordinate from the touch device
        :return: The converted (x, y) screen coordinates
        """

        # xs, ys are the transformed screen coordinates

        if self.is_calibrated:  
            cal = self._cal

            xs = xt * cal.alphaX + yt * cal.betaX + cal.deltaX
            ys = xt * cal.alphaY + yt * cal.betaY + cal.deltaY

            # The above transformation should take care of mirroring if the calibration
            # data has been collected using the 3 point calbration method. However,
            # maybe mirroring would be useful if swapping calibration data between displays
            # that are connected with the axes reversed.
            if cal.mirrorX:
                xs = self._orig_width - xs - 1
            if cal.mirrorY:
                ys = self._orig_height - ys - 1
        else:
            # Assume touch coordinates map directly to screen coordinates

            xs, ys = xt, yt # initialise in case neither rotation is applied

            if (
                self._startup_rotation == lv.DISPLAY_ROTATION._180 or  # NOQA
                self._startup_rotation == lv.DISPLAY_ROTATION._270  # NOQA
            ):
                xs = self._orig_width - xt - 1
                ys = self._orig_height - yt - 1

            if (
                self._startup_rotation == lv.DISPLAY_ROTATION._90 or  # NOQA
                self._startup_rotation == lv.DISPLAY_ROTATION._270  # NOQA
            ):
                xs, ys = self._orig_height - yt - 1, xt

        return int(round(xs)), int(round(ys))

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
            (x != self._last_x or
             y != self._last_y or
             self._last_state != state)
        ):
            template = '{}(raw_x={}, raw_y={}, x={}, y={}, state={})'
            print(template.format(
                self.__class__.__name__,
                x, y, data.point.x, data.point.y,
                "PRESSED" if data.state else "RELEASED"))

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
