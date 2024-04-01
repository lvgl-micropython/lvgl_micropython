import lvgl as lv  # NOQA
import display_driver_framework


class IndevBase:
    _instance_counter = 1
    _indevs = []

    PRESSED = lv.INDEV_STATE_PRESSED
    RELEASED = lv.INDEV_STATE_RELEASED

    def __init__(self):  # NOQA
        self.__class__._instance_counter += 1
        self.id = self.__class__._instance_counter

        if not lv.is_initialized():
            lv.init()

        disp = lv.display_get_default()

        if disp is None:
            raise RuntimeError(
                'the display driver must be initilized '
                'before the pointer driver'
            )

        self._disp_drv = disp

        displays = display_driver_framework.DisplayDriver.get_displays()
        for display in displays:
            if display._disp_drv == disp:
                self._py_disp_drv = display
                break
        else:
            raise RuntimeError(
                'Display driver needs to initilized before indev driver'
            )

        self._height = self._py_disp_drv.get_physical_horizontal_resolution()
        self._width = self._py_disp_drv.get_physical_vertical_resolution()

        self._current_state = self.RELEASED

        indev_drv = lv.indev_create()
        lv.indev_set_read_cb(indev_drv, self._read)
        lv.indev_set_display(indev_drv, disp)
        lv.indev_enable(indev_drv, True)
        self._indev_drv = indev_drv

        self._indevs.append(self)

    def get_width(self):
        return self._width

    def get_height(self):
        return self._height

    def get_rotation(self):
        return lv.display_get_rotation(self._disp_drv)

    def _set_type(self, type_):
        lv.indev_set_type(self._indev_drv, type_)

    def _read(self, drv, data):  # NOQA
        raise NotImplementedError

    def get_type(self):
        return lv.indev_get_type(self._indev_drv)

    def read(self):
        lv.indev_read(self._indev_drv)

    def send_event(self, code, param):
        return lv.indev_send_event(self._indev_drv, code, param)

    def remove_event(self, index):
        return lv.indev_remove_event(self._indev_drv, index)

    def get_event_dsc(self, index):
        return lv.indev_get_event_dsc(self._indev_drv, index)

    def get_event_count(self):
        return lv.indev_get_event_count(self._indev_drv)

    def add_event_cb(self, event_cb, filter, user_data):
        lv.indev_add_event_cb(self._indev_drv, event_cb, filter, user_data)

    def search_obj(self, point):
        return lv.indev_search_obj(self._indev_drv, point)

    def delete_read_timer(self):
        lv.indev_delete_read_timer(self._indev_drv)

    def get_read_timer(self):
        return lv.indev_get_read_timer(self._indev_drv)

    def get_active_obj(self):
        return lv.indev_get_active_obj(self._indev_drv)

    def wait_release(self):
        lv.indev_wait_release(self._indev_drv)

    def get_state(self):
        return lv.indev_get_state(self._indev_drv)

    def enable(self, en):
        lv.indev_enable(self._indev_drv, en)

    def get_group(self):
        return lv.indev_get_group(self._indev_drv)

    def set_group(self, group):
        lv.indev_set_group(self._indev_drv, group)

    def reset(self, obj):
        lv.indev_reset(self._indev_drv, obj)

    def get_disp(self):
        return self._py_disp_drv

    @staticmethod
    def active():
        indev = lv.indev_active()
        for i in IndevBase._indevs:
            if i._indev_drv == indev:
                return i
