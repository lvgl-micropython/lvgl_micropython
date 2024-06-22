import lvgl as lv  # NOQA
import display_driver_framework


class IndevBase:
    _instance_counter = 1
    _indevs = []

    PRESSED = lv.INDEV_STATE.PRESSED
    RELEASED = lv.INDEV_STATE.RELEASED

    def __init__(self, debug=False):  # NOQA
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

        self._width = self._disp_drv.get_horizontal_resolution()
        self._height = self._disp_drv.get_vertical_resolution()
        self._current_state = self.RELEASED
        self._debug = debug

        indev_drv = lv.indev_create()
        indev_drv.set_read_cb(self._read)
        indev_drv.set_display(disp)
        indev_drv.enable(False)
        self._indev_drv = indev_drv
        self._indevs.append(self)

        self._disp_drv.add_event_cb(self._on_size_change, lv.EVENT.RESOLUTION_CHANGED, None)  # NOQA

    def _on_size_change(self, _):
        self._width = self._disp_drv.get_horizontal_resolution()
        self._height = self._disp_drv.get_vertical_resolution()

    def _set_mode_event(self):
        self._indev_drv.set_mode(lv.INDEV_MODE.EVENT)

    def get_width(self):
        return self._width

    def get_height(self):
        return self._height

    def get_rotation(self):
        return self._disp_drv.get_rotation()

    def _set_type(self, type_):
        self._indev_drv.set_type(type_)

    def _read(self, drv, data):  # NOQA
        raise NotImplementedError

    def get_type(self):
        return self._indev_drv.get_type()

    def read(self):
        self._indev_drv.read()

    def send_event(self, code, param):
        return self._indev_drv.send_event(code, param)

    def remove_event(self, index):
        return self._indev_drv.remove_event(index)

    def get_event_dsc(self, index):
        return self._indev_drv.get_event_dsc(index)

    def get_event_count(self):
        return self._indev_drv.get_event_count()

    def add_event_cb(self, event_cb, filter, user_data):
        self._indev_drv.add_event_cb(event_cb, filter, user_data)

    def search_obj(self, point):
        return self._indev_drv.search_obj(point)

    def delete_read_timer(self):
        self._indev_drv.delete_read_timer()

    def get_read_timer(self):
        return self._indev_drv.get_read_timer()

    def get_active_obj(self):
        return self._indev_drv.get_active_obj()

    def wait_release(self):
        self._indev_drv.wait_release()

    def get_state(self):
        return self._indev_drv.get_state()

    def enable(self, en):
        self._indev_drv.enable(en)

    def get_group(self):
        return self._indev_drv.get_group()

    def set_group(self, group):
        self._indev_drv.set_group(group)

    def reset(self, obj):
        self._indev_drv.reset(obj)

    def get_disp(self):
        return self._py_disp_drv

    @staticmethod
    def active():
        indev = lv.indev_active()
        for i in IndevBase._indevs:
            if i._indev_drv == indev:
                return i
