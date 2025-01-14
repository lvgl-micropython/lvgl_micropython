# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import pointer_framework
import lvgl as lv  # NOQA
import micropython  # NOQA


class SDLPointer(pointer_framework.PointerDriver):
    def __init__(
        self,
        *_,
        startup_rotation=pointer_framework.lv.DISPLAY_ROTATION._0,  # NOQA
        debug=False,
        **__
    ):  # NOQA
        super().__init__(
            touch_cal=None, startup_rotation=startup_rotation, debug=debug
        )

        self.__current_state = self.RELEASED
        self.__x = -1
        self.__y = -1
        self.__wheel_x = 0
        self.__wheel_y = 0
        self.__scroll_obj = None
        self.__button_state = self.RELEASED
        self.set_mode(lv.INDEV_MODE.EVENT)  # NOQA

        self._py_disp_drv._data_bus.register_mouse_callback(self._mouse_cb)  # NOQA

    def set_mode(self, mode):
        self._indev_drv.set_mode(mode)  # NOQA

    def _get_object(self):
        if not self.__wheel_x and not self.__wheel_y:
            return

        point = lv.point_t({'x': self.__x, 'y': self.__y})

        obj = lv.indev_search_obj(self._disp_drv.get_layer_sys(), point)
        if not obj:
            obj = lv.indev_search_obj(self._disp_drv.get_layer_top(), point)
        if not obj:
            obj = lv.indev_search_obj(self._disp_drv.get_screen_active(), point)
        if not obj:
            obj = lv.indev_search_obj(self._disp_drv.get_layer_bottom(), point)

        if obj:
            dir_flags = obj.get_scroll_dir()  # NOQA

            if self.__wheel_x and dir_flags | lv.DIR.HOR == dir_flags:  # NOQA
                pass
            else:
                self.__wheel_x = 0

            if self.__wheel_y and dir_flags | lv.DIR.VER == dir_flags:  # NOQA
                pass
            else:
                self.__wheel_y = 0

            if self.__wheel_x or self.__wheel_y:
                return obj

        self.__wheel_x = 0
        self.__wheel_y = 0

    def _mouse_cb(self, _, state, x, y, wheel_x, wheel_y):
        self.__x = x
        self.__y = y
        self.__wheel_x = wheel_x
        self.__wheel_y = wheel_y

        if state:
            self.__button_state = self.PRESSED
        else:
            self.__button_state = self.RELEASED

        self.read()

    def _get_coords(self):
        obj = self._get_object()

        if obj is not None:
            obj.scroll_by_bounded(self.__wheel_x, self.__wheel_y, 1)  # NOQA
            self.__wheel_x = 0
            self.__wheel_y = 0

        return self.__button_state, self.__x, self.__y
