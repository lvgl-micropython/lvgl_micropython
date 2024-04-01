import display_driver_framework


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class RGBDisplay(display_driver_framework.DisplayDriver):

    def _dummy_set_memory_location(self, *_, **__):
        return 0x00

    def set_rotation(self, value):
        self._disp_drv.set_orientation(value)
        self._rotation = value
