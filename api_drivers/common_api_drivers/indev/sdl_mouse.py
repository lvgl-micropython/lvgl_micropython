from micropython import const
import pointer_framework
import lvgl as lv  # NOQA


SDL_BUTTON_LEFT = const(0x01)
SDL_BUTTON_MIDDLE = const(0x02)
SDL_BUTTON_RIGHT = const(0x03)
SDL_BUTTON_X1 = const(0x04)
SDL_BUTTON_X2 = const(0x05)
SDL_BUTTON_LMASK = const(0x01)
SDL_BUTTON_MMASK = const(0x02)
SDL_BUTTON_RMASK = const(0x04)
SDL_BUTTON_X1MASK = const(0x08)
SDL_BUTTON_X2MASK = const(0x10)


class SDLMouse(pointer_framework.PointerDriver):
    def __init__(self):
        super().__init__(None)

        self.__current_state = self.RELEASED
        self.__x = -1
        self.__y = -1
        self.__wheel_x = 0
        self.__wheel_y = 0
        self.__scroll_obj = None
        self.__button_state = self.RELEASED

        self._py_disp_drv._data_bus.register_mouse_button_callback(self._button_cb)  # NOQA
        self._py_disp_drv._data_bus.register_mouse_wheel_callback(self._wheel_cb)  # NOQA
        self._py_disp_drv._data_bus.register_mouse_motion_callback(self._motion_cb)  # NOQA

    def _get_coords(self):
        if self.__scroll_obj is not None:
            self.__scroll_obj.scroll_by_bounded(self.__wheel_x, self.__wheel_y, 1)  # NOQA
            self.__scroll_obj = None
            return None

        return self.__button_state, self.__x, self.__y

    def _button_cb(self, kwargs):
        button = kwargs['button']

        if button | SDL_BUTTON_LMASK == button:
            if kwargs['state']:
                self.__button_state = self.PRESSED
            else:
                self.__button_state = self.RELEASED

            self.__x = kwargs['x']
            self.__y = kwargs['y']

    def _motion_cb(self, kwargs):
        self.__x = kwargs['x']
        self.__y = kwargs['y']

        state = kwargs['state']

        if state | SDL_BUTTON_LMASK == state:
            self.__button_state = self.PRESSED
        else:
            self.__button_state = self.RELEASED

    def _wheel_cb(self, kwargs):
        x = kwargs['x']
        y = kwargs['y']

        point = lv.point_t({'x': kwargs['mouseX'], 'y': kwargs['mousey']})

        obj = lv.indev_search_obj(self._disp_drv.get_layer_sys(), point)
        if not obj:
            obj = lv.indev_search_obj(self._disp_drv.get_layer_top(), point)
        if not obj:
            obj = lv.indev_search_obj(self._disp_drv.get_screen_active(), point)
        if not obj:
            obj = lv.indev_search_obj(self._disp_drv.get_layer_bottom(), point)

        if obj:
            dir_flags = obj.get_scroll_dir()

            if x and dir_flags | lv.DIR_HOR == dir_flags:
                self.__wheel_x = x
            else:
                self.__wheel_x = 0

            if y and dir_flags | lv.DIR_VER == dir_flags:
                self.__wheel_y = y
            else:
                self.__wheel_y = 0

            if self.__wheel_x or self.__wheel_y:
                self.__scroll_obj = obj
            else:
                self.__scroll_obj = None
        else:
            self.__wheel_x = 0
            self.__wheel_y = 0
            self.__scroll_obj = None
