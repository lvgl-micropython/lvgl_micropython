# Copyright (c) 2024 - 2025 Kevin G. Schlosser

from micropython import const  # NOQA
import display_driver_framework

import lcd_bus
import gc
import lvgl as lv


STATE_HIGH = display_driver_framework.STATE_HIGH
STATE_LOW = display_driver_framework.STATE_LOW
STATE_PWM = display_driver_framework.STATE_PWM

_SET_CONTRAST = const(0x81)
_SET_NORM_INV = const(0xA6)
_DISP_OFF = const(0xAE)
_DISP_ON = const(0xAF)
_SET_COL_ADDR = const(0x21)
_SET_PAGE_ADDR = const(0x22)


class SSD1306(display_driver_framework.DisplayDriver):

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
        backlight_pin=None,
        backlight_on_state=STATE_HIGH,
        offset_x=0,
        offset_y=0,
        color_space=lv.COLOR_FORMAT.I1,  # NOQA
        rgb565_byte_swap=False
    ):

        if not isinstance(data_bus, (lcd_bus.SPIBus, lcd_bus.I2CBus)):
            raise ValueError('Only SPI and I2C lcd busses allowed')

        buf_size = int(display_width * display_height // 8)

        if frame_buffer1 is None:

            gc.collect()

            for flags in (
                lcd_bus.MEMORY_INTERNAL | lcd_bus.MEMORY_DMA,
                lcd_bus.MEMORY_SPIRAM | lcd_bus.MEMORY_DMA,
                lcd_bus.MEMORY_INTERNAL,
                lcd_bus.MEMORY_SPIRAM
            ):
                try:
                    frame_buffer1 = (
                        data_bus.allocate_framebuffer(buf_size, flags)
                    )

                    if (flags | lcd_bus.MEMORY_DMA) == flags:
                        frame_buffer2 = (
                            data_bus.allocate_framebuffer(buf_size, flags)
                        )

                    break
                except MemoryError:
                    frame_buffer1 = data_bus.free_framebuffer(frame_buffer1)  # NOQA

            if frame_buffer1 is None:
                raise MemoryError(
                    f'Unable to allocate memory for frame buffer ({buf_size})'
                    # NOQA
                )

        self._pages = int(display_height // 8)

        if len(frame_buffer1) != buf_size:
            raise ValueError(f'Framebuffer is too small ({buf_size}')

        super().__init__(
            data_bus=data_bus,
            display_width=display_width,
            display_height=display_height,
            frame_buffer1=frame_buffer1,
            frame_buffer2=frame_buffer2,
            reset_pin=reset_pin,
            reset_state=reset_state,
            power_pin=None,
            backlight_pin=backlight_pin,
            backlight_on_state=backlight_on_state,
            offset_x=offset_x,
            offset_y=offset_y,
            color_byte_order=display_driver_framework.BYTE_ORDER_RGB,
            color_space=color_space,  # NOQA
            rgb565_byte_swap=rgb565_byte_swap,
            _cmd_bits=8,
            _param_bits=8,
            _init_bus=True
        )

        self._power_pin = power_pin

    def set_constrast(self, value):
        self._param_buf[0] = value & 0xFF
        self.set_params(_SET_CONTRAST, self._param_mv[:1])

    def set_color_inversion(self, value):
        self.set_params(_SET_NORM_INV | (int(bool(value)) & 1))

    def get_power(self):
        return self._power

    def set_power(self, value):
        self._power = bool(value)  # NOQA

        if self._power:
            self.set_params(_DISP_ON)
        else:
            self.set_params(_DISP_OFF)

    def _flush_cb(self, _, area, color_p):
        # display ram is 8192 bits in size. the is evenly divided into 8 pages.
        # each page holds
        # 8 pages 1024 bits (128 bytes)
        x1 = 0
        x2 = self.display_width - 1

        if self.display_width == 64:
            # displays with width of 64 pixels are shifted by 32
            x1 += 32
            x2 += 32

        y1 = 0
        y2 = self._pages - 1

        self._param_buf[0] = x1
        self._param_buf[1] = x2
        self.set_params(_SET_COL_ADDR, self._param_mv[:2])

        self._param_buf[0] = y1
        self._param_buf[1] = y2
        self.set_params(_SET_PAGE_ADDR, self._param_mv[:2])

        size = int((area.x2 - area.x1 + 1) * (area.y2 - area.y1 + 1) / 8)

        # we have to use the __dereference__ method because this method is
        # what converts from the C_Array object the binding passes into a
        # memoryview object that can be passed to the bus drivers
        data_view = color_p.__dereference__(size)
        self._data_bus.tx_color(-1, data_view, x1, y1, x2, y2, self._rotation, self._disp_drv.flush_is_last())
