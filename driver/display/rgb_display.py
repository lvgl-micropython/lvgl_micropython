import display_driver_framework


class RGBDisplay(display_driver_framework.DisplayDriver):

    def _dummy_set_memory_location(self, *_, **__):
        return 0x00

    def set_rotation(self, value):
        self._disp_drv.set_orientation(value)
        self._rotation = value
