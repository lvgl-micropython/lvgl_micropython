from micropython import const
import pointer_framework


BUTTON_INVALID = -1
BUTTON_A = 0
BUTTON_B = 1
BUTTON_X = 2
BUTTON_Y = 3
BUTTON_BACK = 4
BUTTON_GUIDE = 5
BUTTON_START = 6
BUTTON_LEFT_STICK = 7
BUTTON_RIGHT_STICK = 8
BUTTON_LEFT_SHOULDER = 9
BUTTON_RIGHT_SHOULDER = 10
BUTTON_DPAD_UP = 11
BUTTON_DPAD_DOWN = 12
BUTTON_DPAD_LEFT = 13
BUTTON_DPAD_RIGHT = 14
BUTTON_MISC1 = 15
BUTTON_PADDLE1 = 16
BUTTON_PADDLE2 = 17
BUTTON_PADDLE3 = 18
BUTTON_PADDLE4 = 19
BUTTON_TOUCHPAD = 20

AXIS_INVALID = -1
AXIS_LEFT_X = 0
AXIS_LEFT_Y = 1
AXIS_RIGHT_X = 2
AXIS_RIGHT_Y = 3
AXIS_TRIGGER_LEFT = 4
AXIS_TRIGGER_RIGHT = 5


_SDL_CONTROLLERAXISMOTION = const(0x650)

_SDL_CONTROLLERBUTTONDOWN = const(0x651)
_SDL_CONTROLLERBUTTONUP = const(0x652)

_SDL_CONTROLLERTOUCHPADDOWN = const(0x656)
_SDL_CONTROLLERTOUCHPADMOTION = const(0x657)
_SDL_CONTROLLERTOUCHPADUP = const(0x658)


class SDLController(pointer_framework.PointerDriver):
    def __init__(self):
        super().__init__(touch_cal=None)
        self._disp_drv._data_bus.register_controller_axis_motion_callback(self._axis_cb)
        self._disp_drv._data_bus.register_controller_button_callback(self._button_cb)

        self.__current_state = self.RELEASED
        self.__x = -1
        self.__y = -1

    def _axis_cb(self, kwargs):
        if kwargs['axis'] == AXIS_LEFT_X:
            factor = kwargs['value'] / 32767.0
            self.__x += int(self.get_width() / 10 * factor)
        elif kwargs['axis'] == AXIS_LEFT_Y:
            factor = kwargs['value'] / 32767.0
            self.__y += int(self.get_height() / 10 * factor)

    def _button_cb(self, kwargs):
        if kwargs['button'] == BUTTON_A:
            if not kwargs['state']:
                self.__current_state = self.RELEASED
            else:
                self.__current_state = self.PRESSED

    def _get_coords(self):
        return self.__current_state, self.__x, self.__y


class SDLControllerTouchpad(pointer_framework.PointerDriver):

    def __init__(self, touch_cal=None):
        super().__init__(touch_cal=touch_cal)
        self._disp_drv._data_bus.register_touch_callback(self._touch_cb)
        self.__current_state = self.PRESSED
        self.__x = -1
        self.__y = -1

    def _touch_cb(self, kwargs):
        self.__x = int(self.get_width() * kwargs['x'])
        self.__y = int(self.get_height() * kwargs['y'])

        if kwargs['type'] == _SDL_CONTROLLERTOUCHPADUP:
            self.__current_state = self.RELEASED
        else:
            self.__current_state = self.PRESSED

    def _get_coords(self):
        if self.__current_state == self.RELEASED:
            return None

        return self.__current_state, self.__x, self.__y
