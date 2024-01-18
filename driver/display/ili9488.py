
from micropython import const  # NOQA
import lvgl as lv
import lcd_bus  # NOQA
import display_driver_framework


_IFMODE = const(0xB0)
_FRMCTR1 = const(0xB1)
_DIC = const(0xB4)
_DFC = const(0xB6)
_EM = const(0xB7)
_PWR1 = const(0xC0)
_PWR2 = const(0xC1)
_VCMPCTL = const(0xC5)
_PGC = const(0xE0)
_NGC = const(0xE1)
_AC3 = const(0xF7)
_MADCTL = const(0x36)
_COLMOD = const(0x3A)
_NOP = const(0x00)

STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class ILI9488(display_driver_framework.DisplayDriver):

    # The st7795 display controller has an internal framebuffer
    # arranged in 320 x 480
    # configuration. Physical displays with pixel sizes less than
    # 320 x 480 must supply a start_x and
    # start_y argument to indicate where the physical display begins
    # relative to the start of the
    # display controllers internal framebuffer.

    # this display driver supports RGB565 and also RGB666. RGB666 is going to
    # use twice as much memory as the RGB565. It is also going to slow down the
    # frame rate by 1/3, This is becasue of the extra byte of data that needs
    # to get sent. To use RGB666 the color depth MUST be set to 32.
    # so when compiling
    # make sure to have LV_COLOR_DEPTH=32 set in LVFLAGS when you call make.
    # For RGB565 you need to have LV_COLOR_DEPTH=16

    # the reason why we use a 32 bit color depth is because of how the data gets
    # written. The entire 8 bits for each byte gets sent. The controller simply
    # ignores the lowest 2 bits in the byte to make it a 6 bit color channel
    # We just have to tell lvgl that we want to use

    display_name = 'ILI9488'

    def init(self):
        param_buf = bytearray(15)
        param_mv = memoryview(param_buf)

        param_buf[:15] = bytearray([
            0x00, 0x03, 0x09, 0x08, 0x16,
            0x0A, 0x3F, 0x78, 0x4C, 0x09,
            0x0A, 0x08, 0x16, 0x1A, 0x0F
        ])

        self.set_params(_PGC, param_mv[:15])

        param_buf[:15] = bytearray([
            0x00, 0x16, 0x19, 0x03, 0x0F,
            0x05, 0x32, 0x45, 0x46, 0x04,
            0x0E, 0x0D, 0x35, 0x37, 0x0F
        ])
        self.set_params(_NGC, param_mv[:15])

        param_buf[0] = 0x17
        param_buf[1] = 0x15

        self.set_params(_PWR1, param_mv[:2])

        param_buf[0] = 0x41
        self.set_params(_PWR2, param_mv[:1])

        param_buf[0] = 0x00
        param_buf[1] = 0x12
        param_buf[3] = 0x80
        self.set_params(_VCMPCTL, param_mv[:3])

        param_buf[0] = (
            self._madctl(
                self._color_byte_order,
                display_driver_framework._ORIENTATION_TABLE
            )
        )
        self.set_params(_MADCTL, param_mv[:1])

        color_size = lv.color_format_get_size(self._color_space)
        if color_size == 2:  # NOQA
            pixel_format = 0x55
        elif color_size == 3:
            pixel_format = 0x77
        else:
            raise RuntimeError(
                'ILI9488 IC only supports '
                'lv.COLOR_FORMAT.RGB565 or lv.COLOR_FORMAT.RGB888'
            )

        param_buf[0] = pixel_format
        self.set_params(_COLMOD, param_mv[:1])

        param_buf[0] = 0x00
        self.set_params(_IFMODE, param_mv[:1])

        param_buf[0] = 0xA0
        self.set_params(_FRMCTR1, param_mv[:1])

        param_buf[0] = 0x02
        self.set_params(_DIC, param_mv[:1])

        param_buf[0] = 0x02
        param_buf[1] = 0x02
        param_buf[2] = 0x3B
        self.set_params(_DFC, param_mv[:3])

        param_buf[0] = 0xC6
        self.set_params(_EM, param_mv[:1])

        param_buf[:4] = bytearray([
            0xA9, 0x51, 0x2C, 0x02
        ])
        self._data_bus.tx_param(_AC3, param_mv[:4])

        self.set_params(_NOP)

        display_driver_framework.DisplayDriver.init(self)
