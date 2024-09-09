import display_driver_framework

STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class RGBDisplay(display_driver_framework.DisplayDriver):

    def init(self):
        self._disp_drv.sw_rotate = 1
        display_driver_framework.DisplayDriver.init(self)

    def _dummy_set_memory_location(self, *_, **__):
        return 0x00
