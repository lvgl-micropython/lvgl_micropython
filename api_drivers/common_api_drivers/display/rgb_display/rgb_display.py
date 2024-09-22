import rgb_display_framework

STATE_HIGH = rgb_display_framework.STATE_HIGH
STATE_LOW = rgb_display_framework.STATE_LOW
STATE_PWM = rgb_display_framework.STATE_PWM

BYTE_ORDER_RGB = rgb_display_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = rgb_display_framework.BYTE_ORDER_BGR


class RGBDisplay(rgb_display_framework.RGBDisplayDriver):

    def init(self):
        rgb_display_framework.RGBDisplayDriver.init(self, None)
