from micropython import const

import lvgl as lv
import pointer_framework


_SDL_FINGERDOWN = const(0x700)
_SDL_FINGERUP = const(0x701)
_SDL_FINGERMOTION = const(0x702)


class SDLTouch(pointer_framework.PointerDriver):

    def __init__(self, touch_cal=None):
        super().__init__(touch_cal=touch_cal)
        self._disp_drv._data_bus.register_touch_callback(self._touch_cb)
        self.__current_state = self.PRESSED
        self.__x = -1
        self.__y = -1

    def _touch_cb(self, kwargs):
        self.__x = kwargs['x']
        self.__y = kwargs['y']
        if kwargs['type'] == _SDL_FINGERUP:
            self.__current_state = self.RELEASED
        else:
            self.__current_state = self.PRESSED

    def _get_coords(self):
        if self.__current_state == self.RELEASED:
            return None

        return self.__current_state, self.__x, self.__y
