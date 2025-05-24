# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import lvgl as lv


class AutoRotation:

    def __init__(self, device, delay, lock_rotation=False, adjustment=0.0):

        if adjustment > 180 or adjustment < -180:
            raise ValueError('adjustment range is -180.0 to +180.0')

        scrn = lv.screen_active()
        disp = scrn.get_display()

        self._device = device

        self._timer = lv.timer_create(self._timer_cb, delay)
        self._timer.set_repeat_count(-1)

        self._fusion = fusion.Fusion()
        self._lock_rotation = lock_rotation

        self._last_rotation = disp.get_rotation()
        self._last_free_rotation = 0
        self._adjustment = adjustment

    @property
    def adjustment(self):
        return self._adjustment

    @adjustment.setter
    def adjustment(self, value):
        self._adjustment = value
        self._timer_cb(None)

    @property
    def lock_rotation(self):
        return self._lock_rotation

    @lock_rotation.setter
    def lock_rotation(self, value):
        self._lock_rotation = value
        self._timer_cb(None)

    def __del__(self):
        if self._timer is not None:
            self._timer.delete()

    def _timer_cb(self, _):
        roll, pitch, yaw = self._device.read()

        roll += self._adjustment

        if roll > 180.00:
            roll -= 360.0

        if roll < -180.0:
            roll += 360.0

        if roll < 0.0:
            roll += 360.0

        scrn = lv.screen_active()
        disp = scrn.get_display()

        if self._lock_rotation:

            if 45 <= roll < 135:
                new_rotation = lv.DISPLAY_ROTATION._90

            elif 135 <= roll < 225:
                new_rotation = lv.DISPLAY_ROTATION._180

            elif 225 <= roll < 315:
                new_rotation = lv.DISPLAY_ROTATION._270

            else:
                new_rotation = lv.DISPLAY_ROTATION._0

            if new_rotation != self._last_rotation:
                self._last_rotation = new_rotation
                disp.set_rotation(new_rotation)
        else:
            roll = int(roll * 10.0)

            if self._last_rotation != lv.DISPLAY_ROTATION._0:
                self._last_rotation = lv.DISPLAY_ROTATION._0
                disp.set_rotation(lv.DISPLAY_ROTATION._0)

            if roll != self._last_free_rotation:
                self._last_free_rotation = roll
                top_layer = disp.get_layer_top()
                sys_layer = disp.get_layer_sys()
                bottom_layer = disp.get_layer_bottom()

                top_layer_pivot_x = int(top_layer.get_width() / 2)
                top_layer_pivot_y = int(top_layer.get_height() / 2)
                top_layer.set_style_transform_pivot_x(top_layer_pivot_x, lv.PART.ANY)
                top_layer.set_style_transform_pivot_y(top_layer_pivot_y, lv.PART.ANY)
                top_layer.set_style_transform_rotation(roll, lv.PART.ANY)

                sys_layer_pivot_x = int(sys_layer.get_width() / 2)
                sys_layer_pivot_y = int(sys_layer.get_height() / 2)
                sys_layer.set_style_transform_pivot_x(sys_layer_pivot_x, lv.PART.ANY)
                sys_layer.set_style_transform_pivot_y(sys_layer_pivot_y, lv.PART.ANY)
                sys_layer.set_style_transform_rotation(roll, lv.PART.ANY)

                bottom_layer_pivot_x = int(bottom_layer.get_width() / 2)
                bottom_layer_pivot_y = int(bottom_layer.get_height() / 2)
                bottom_layer.set_style_transform_pivot_x(bottom_layer_pivot_x, lv.PART.ANY)
                bottom_layer.set_style_transform_pivot_y(bottom_layer_pivot_y, lv.PART.ANY)
                bottom_layer.set_style_transform_rotation(roll, lv.PART.ANY)

                scrn_pivot_x = int(scrn.get_width() / 2)
                scrn_pivot_y = int(scrn.get_height() / 2)
                scrn.set_style_transform_pivot_x(scrn_pivot_x, lv.PART.ANY)
                scrn.set_style_transform_pivot_y(scrn_pivot_y, lv.PART.ANY)
                scrn.set_style_transform_rotation(roll, lv.PART.ANY)
