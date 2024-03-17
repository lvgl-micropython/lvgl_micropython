from micropython import const

import lvgl as lv
import pointer_framework


_HAT_CENTERED = const(0x00)
_HAT_UP = const(0x01)
_HAT_RIGHT = const(0x02)
_HAT_DOWN = const(0x04)
_HAT_LEFT = const(0x08)
_HAT_LEFTUP = const(0x09)
_HAT_RIGHTUP = const(0x03)
_HAT_LEFTDOWN = const(0x0C)
_HAT_RIGHTDOWN = const(0x06)


class SDLJoystick(pointer_framework.PointerDriver):
    def __init__(self):
        super().__init__(touch_cal=None)
        self._disp_drv._data_bus.register_joystick_axis_motion_callback(self._axis_cb)
        self._disp_drv._data_bus.register_joystick_button_callback(self._button_cb)
        self._disp_drv._data_bus.register_joystick_ball_motion_callback(self._ball_cb)
        self._disp_drv._data_bus.register_joystick_hat_motion_callback(self._hat_cb)

        self.__current_state = self.RELEASED
        self.__x = -1
        self.__y = -1

    def _button_cb(self, kwargs):
        if kwargs['state']:
            self.__current_state = self.PRESSED
        else:
            self.__current_state = self.RELEASED

    def _axis_cb(self, kwargs):
        if kwargs['axis'] == 0:
            factor = kwargs['value'] / 32767.0
            self.__x += int(self.get_width() / 10 * factor)
        elif kwargs['axis'] == 1:
            factor = kwargs['value'] / 32767.0
            self.__y += int(self.get_height() / 10 * factor)

    def _ball_cb(self, kwargs):
        self.__x += kwargs['xrel']
        self.__y += kwargs['yrel']

    def _hat_cb(self, kwargs):
        if kwargs['value'] == _HAT_UP:
            self.__y -= 1
        elif kwargs['value'] == _HAT_RIGHT:
            self.__x += 1
        elif kwargs['value'] == _HAT_DOWN:
            self.__y += 1
        elif kwargs['value'] == _HAT_LEFT:
            self.__x -= 1
        elif kwargs['value'] == _HAT_LEFTUP:
            self.__y -= 1
            self.__x -= 1
        elif kwargs['value'] == _HAT_RIGHTUP:
            self.__y -= 1
            self.__x += 1
        elif kwargs['value'] == _HAT_LEFTDOWN:
            self.__y += 1
            self.__x -= 1
        elif kwargs['value'] == _HAT_RIGHTDOWN:
            self.__y += 1
            self.__x += 1

    def _get_coords(self):
        return self.__current_state, self.__x, self.__y


