import display_driver_framework
import lvgl as lv

STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class RGBDisplay(display_driver_framework.DisplayDriver):

    def __init__(
        self,
        data_bus,
        display_width,
        display_height,
        frame_buffer1=None,
        frame_buffer2=None,
        reset_pin=None,
        reset_state=STATE_HIGH,
        power_pin=None,
        power_on_state=STATE_HIGH,
        backlight_pin=None,
        backlight_on_state=STATE_HIGH,
        offset_x=0,
        offset_y=0,
        color_byte_order=BYTE_ORDER_RGB,
        color_space=lv.COLOR_FORMAT.RGB888,
        rgb565_byte_swap=False
    ):

        super().__init__(
            data_bus=data_bus,
            display_width=display_width,
            display_height=display_height,
            frame_buffer1=frame_buffer1,
            frame_buffer2=frame_buffer2,
            reset_pin=reset_pin,
            reset_state=reset_state,
            power_pin=power_pin,
            power_on_state=power_on_state,
            backlight_pin=backlight_pin,
            backlight_on_state=backlight_on_state,
            offset_x=offset_x,
            offset_y=offset_y,
            color_byte_order=color_byte_order,
            color_space=color_space,
            rgb565_byte_swap=rgb565_byte_swap
        )

        self.__flushing_fb_index = None

    def _flush_ready_cb(self, buf_num):
        if (
            self.__flushing_fb_index is None or
            buf_num != self.__flushing_fb_index
        ):
            self._disp_drv.flush_ready()
            self.__flushing_fb_index = buf_num
            return True

        return False

    def _dummy_set_memory_location(self, *_, **__):
        return 0x00

    def set_rotation(self, value):
        self._disp_drv.set_orientation(value)
        self._rotation = value
