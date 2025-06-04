# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import micropython  # NOQA
import time
import gc
import sys
import machine  # NOQA
from micropython import const  # NOQA

import lvgl as lv  # NOQA
import lcd_bus
import io_expander_framework


try:
    micropython.alloc_emergency_exception_buf(256)  # NOQA
except AttributeError:
    pass

# Constants

BYTE_ORDER_RGB = const(0x00)
BYTE_ORDER_BGR = const(0x08)

_RASET = const(0x2B)
_CASET = const(0x2A)
_RAMWR = const(0x2C)
_MADCTL = const(0x36)


_MADCTL_MH = const(0x04)  # Refresh 0=Left to Right, 1=Right to Left
_MADCTL_BGR = const(0x08)  # BGR color order
_MADCTL_ML = const(0x10)  # Refresh 0=Top to Bottom, 1=Bottom to Top
_MADCTL_MV = const(0x20)  # 0=Normal, 1=Row/column exchange
_MADCTL_MX = const(0x40)  # 0=Left to Right, 1=Right to Left
_MADCTL_MY = const(0x80)  # 0=Top to Bottom, 1=Bottom to Top


STATE_HIGH = 1
STATE_LOW = 0
STATE_PWM = -1


class DisplayDriver:
    _INVON = 0x21
    _INVOFF = 0x20

    _ORIENTATION_TABLE = (
        _MADCTL_MX,
        _MADCTL_MV,
        _MADCTL_MY,
        _MADCTL_MY | _MADCTL_MX | _MADCTL_MV
    )

    _displays = []

    @staticmethod
    def get_default():
        disp = lv.display_get_default()  # NOQA

        for d in DisplayDriver.get_displays():
            if d._disp_drv == disp:  # NOQA
                return disp

        return None

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
        color_space=lv.COLOR_FORMAT.RGB888,  # NOQA
        rgb565_byte_swap=False,
        _cmd_bits=8,
        _param_bits=8,
        _init_bus=True
    ):
        if power_on_state not in (STATE_HIGH, STATE_LOW):
            raise RuntimeError(
                'power on state must be either STATE_HIGH or STATE_LOW'
            )

        if reset_state not in (STATE_HIGH, STATE_LOW):
            raise RuntimeError(
                'reset state must be either STATE_HIGH or STATE_LOW'
            )

        if backlight_on_state not in (STATE_HIGH, STATE_LOW, STATE_PWM):
            raise RuntimeError(
                'backlight on state must be either '
                'STATE_HIGH, STATE_LOW or STATE_PWM'
            )

        if not lv.is_initialized():
            lv.init()

        self.display_width = display_width
        self.display_height = display_height

        self._reset_state = reset_state
        self._power_on_state = power_on_state
        self._backlight_on_state = backlight_on_state

        self._offset_x = offset_x
        self._offset_y = offset_y

        self._param_buf = bytearray(4)
        self._param_mv = memoryview(self._param_buf)

        self._color_byte_order = color_byte_order
        self._color_space = color_space

        self._physical_width = display_width
        self._physical_height = display_height

        self._initilized = False
        self._backup_set_memory_location = None

        self._rotation = lv.DISPLAY_ROTATION._0  # NOQA

        self._rgb565_byte_swap = rgb565_byte_swap
        self._cmd_bits = _cmd_bits
        self._param_bits = _param_bits

        if data_bus is None:
            self._reset_pin = None
            self._power_pin = None
            self._backlight_pin = None
            self._data_bus = None
            self._disp_drv = None
            self._frame_buffer1 = frame_buffer1
            self._frame_buffer2 = frame_buffer2
        else:
            if reset_pin is None:
                self._reset_pin = None
            elif not isinstance(reset_pin, int):
                self._reset_pin = reset_pin
            else:
                self._reset_pin = machine.Pin(reset_pin, machine.Pin.OUT)
                self._reset_pin.value(not reset_state)

            if power_pin is None:
                self._power_pin = None
            elif not isinstance(power_pin, int):
                self._power_pin = power_pin
            else:
                self._power_pin = machine.Pin(power_pin, machine.Pin.OUT)
                self._power_pin.value(not power_on_state)

            if backlight_pin is None:
                self._backlight_pin = None
            elif not isinstance(backlight_pin, int):
                self._backlight_pin = backlight_pin
            else:
                self._backlight_pin = machine.Pin(
                    backlight_pin,
                    machine.Pin.OUT
                )

            if backlight_on_state == STATE_PWM:
                if isinstance(self._backlight_pin, io_expander_framework.Pin):
                    backlight_on_state = STATE_HIGH
                else:
                    self._backlight_pin = machine.PWM(
                        self._backlight_pin, freq=38000)

            if (
                backlight_on_state != STATE_PWM and
                self._backlight_pin is not None
            ):
                self._backlight_pin.value(not backlight_on_state)

            self._backlight_on_state = backlight_on_state

            self._data_bus = data_bus
            self._disp_drv = lv.display_create(display_width, display_height)  # NOQA
            self._disp_drv.set_color_format(color_space)
            self._disp_drv.set_driver_data(self)

            if frame_buffer1 is None:
                buf_size = int(
                    display_width *
                    display_height *
                    lv.color_format_get_size(color_space)
                )
                buf_size = int(buf_size // 10)
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
                        frame_buffer1 = data_bus.free_framebuffer(frame_buffer1)

                if frame_buffer1 is None:
                    raise MemoryError(
                        f'Unable to allocate memory for frame buffer ({buf_size})'  # NOQA
                    )

            self._frame_buffer1 = frame_buffer1
            self._frame_buffer2 = frame_buffer2

            self._init_disp_bus = _init_bus

            if _init_bus:
                self._init_bus()

    def _init_bus(self):
        buffer_size = len(self._frame_buffer1)

        self._data_bus.init(
            self.display_width,
            self.display_height,
            lv.color_format_get_size(self._color_space) * 8,
            buffer_size,
            self._rgb565_byte_swap,
            self._cmd_bits,
            self._param_bits
        )

        self._disp_drv.set_flush_cb(self._flush_cb)

        full_screen_size = (
            self.display_width *
            self.display_height *
            lv.color_format_get_size(self._color_space)
        )
        if full_screen_size == len(self._frame_buffer1):
            render_mode = lv.DISPLAY_RENDER_MODE.FULL  # NOQA
        else:
            render_mode = lv.DISPLAY_RENDER_MODE.PARTIAL  # NOQA

        self._disp_drv.set_buffers(
            self._frame_buffer1,
            self._frame_buffer2,
            len(self._frame_buffer1),
            render_mode
        )

        if isinstance(self._data_bus, lcd_bus.RGBBus):
            # we don't need to set column and page addresses for the RGBBus.
            # The tx_params function in C code for the RGB Bus is a dummy
            # function that only has the purpose of keeping the API the same
            # across all of the busses. No point in using cpu time to make a
            # call to a do nothing function.
            setattr(
                self,
                '_set_memory_location',
                self._dummy_set_memory_location
            )

        self._data_bus.register_callback(self._flush_ready_cb)
        self.set_default()
        self._disp_drv.add_event_cb(
            self._on_size_change,
            lv.EVENT.RESOLUTION_CHANGED,  # NOQA
            None
        )

        self._displays.append(self)

    def _on_size_change(self, _):
        rotation = self._disp_drv.get_rotation()
        self._width = self._disp_drv.get_horizontal_resolution()
        self._height = self._disp_drv.get_vertical_resolution()

        if rotation == self._rotation:
            return

        self._rotation = rotation

        if not isinstance(self._data_bus, lcd_bus.RGBBus) and self._initilized:
            self._param_buf[0] = (self._madctl(
                self._color_byte_order, self._ORIENTATION_TABLE, ~rotation
            ))
            self._data_bus.tx_param(_MADCTL, self._param_mv[:1])

    @staticmethod
    def get_displays():
        return DisplayDriver._displays

    def set_physical_resolution(self, width, height):
        self._disp_drv.set_physical_resolution(width, height)

    def get_physical_horizontal_resolution(self):
        return self._disp_drv.get_physical_horizontal_resolution()

    def get_physical_vertical_resolution(self):
        return self._disp_drv.get_physical_vertical_resolution()

    def set_physical_horizontal_resolution(self, width):
        self._disp_drv.set_physical_horizontal_resolution(width)

    def set_physical_vertical_resolution(self, height):
        self._disp_drv.set_physical_vertical_resolution(height)

    def get_next(self):
        disp = self._disp_drv.get_next()
        return disp.get_driver_data()

    def set_offset(self, x, y):
        self._offset_x, self._offset_y = x, y

    def get_offset_x(self):
        return self._disp_drv.get_offset_x()

    def get_offset_y(self):
        return self._disp_drv.get_offset_y()

    def get_dpi(self):
        return self._disp_drv.get_dpi()

    def set_dpi(self, dpi):
        self._disp_drv.set_dpi(dpi)

    def set_color_format(self, color_space):
        self._disp_drv.set_color_format(color_space)
        self._color_space = color_space

    def get_color_format(self):
        return self._color_space

    def set_antialiasing(self, en):
        self._disp_drv.set_antialiasing(en)

    def get_antialiasing(self):
        return self._disp_drv.get_antialiasing()

    def is_double_buffered(self):
        return self._disp_drv.is_double_buffered()

    def get_screen_active(self):
        return self._disp_drv.get_screen_active()

    def get_screen_prev(self):
        return self._disp_drv.get_screen_prev()

    def get_layer_top(self):
        return self._disp_drv.get_layer_top()

    def get_layer_sys(self):
        return self._disp_drv.get_layer_sys()

    def get_layer_bottom(self):
        return self._disp_drv.get_layer_bottom()

    def add_event_cb(self, event_cb, filter, user_data):  # NOQA
        self._disp_drv.add_event_cb(event_cb, filter, user_data)

    def get_event_count(self):
        return self._disp_drv.get_event_count()

    def get_event_dsc(self, index):
        return self._disp_drv.get_event_dsc(index)

    def delete_event(self, index):
        return self._disp_drv.delete_event(index)

    def send_event(self, code, param):
        return self._disp_drv.send_event(code, param)

    def set_theme(self, th):
        self._disp_drv.set_theme(th)

    def get_theme(self):
        return self._disp_drv.get_theme()

    def get_inactive_time(self):
        return self._disp_drv.get_inactive_time()

    def trigger_activity(self):
        self._disp_drv.trigger_activity()

    def enable_invalidation(self, en):
        self._disp_drv.enable_invalidation(en)

    def is_invalidation_enabled(self):
        return self._disp_drv.is_invalidation_enabled()

    def get_refr_timer(self):
        return self._disp_drv.get_refr_timer()

    def delete_refr_timer(self):
        self._disp_drv.delete_refr_timer()

    def set_color_inversion(self, value):
        # If your white is showing up as black and your black
        # is showing up as white try setting this either True or False
        # and see if it corrects the problem.
        if None in (self._INVON, self._INVOFF):
            raise NotImplementedError

        if value:
            self.set_params(self._INVON)
        else:
            self.set_params(self._INVOFF)

    def set_default(self):
        self._disp_drv.set_default()

    def get_rotation(self):
        return self._rotation

    def set_rotation(self, value):
        self._disp_drv.set_rotation(value)

    def get_horizontal_resolution(self):
        return self._disp_drv.get_horizontal_resolution()

    def get_vertical_resolution(self):
        return self._disp_drv.get_vertical_resolution()

    def init(self, type=None):  # NOQA
        # the following code looks at the frame buffer size compared to what
        # the full frame size is. If they are the same then there is no reason
        # to set the memory address for where the data is to be written to every
        # time the buffer gets flushed. This sets the memory location a single
        # time and then overrides the function for setting the memory address
        # to a do nothing function that will get called by the flush function

        # if this behavior is not wanted then don't call this function at the
        # end of your init method, make sure to set self._initilized = True
        # at the end of your init method.

        # this next code block is to keep the memory consumptiomn low.
        # it allows for dynamically importing the initilization commands
        # and then deletimng them from memory since they are a run
        # once and done
        if type is None:
            mod_name = f'_{self.__class__.__name__.lower()}_init'
        else:
            mod_name = f'_{self.__class__.__name__.lower()}_init_type{type}'

        mod = __import__(mod_name)
        mod.init(self)
        del sys.modules[mod_name]
        # =======================================

        full_frame_size = (
            self.display_width *
            self.display_height *
            lv.color_format_get_size(self._color_space)
        )

        if full_frame_size == len(self._frame_buffer1):
            x1 = self._offset_x
            y1 = self._offset_y
            x2 = x1 + self.display_width
            y2 = y1 + self.display_height
            self._set_memory_location(x1, y1, x2, y2)

            self._backup_set_memory_location = self._set_memory_location
            setattr(
                self,
                '_set_memory_location',
                self._dummy_set_memory_location
            )

        self._initilized = True

    def set_params(self, cmd, params=None):
        self._data_bus.tx_param(cmd, params)

    def get_params(self, cmd, params):
        self._data_bus.rx_param(cmd, params)

    def get_power(self):
        if self._power_pin is None:
            return -1

        state = self._power_pin.value()
        if self._power_on_state:
            return state

        return not state

    def set_power(self, value):
        if self._power_pin is None:
            return

        if self._power_on_state:
            self._power_pin.value(value)
        else:
            self._power_pin.value(not value)

    def delete(self):
        raise NotImplementedError('You must delete the instance by using `del {instance}`')

    def __del__(self):
        if self in self._displays:
            self._displays.remove(self)
            self._disp_drv.delete()

        if not self._displays and lv.is_initialized():
            lv.deinit()

    def reset(self):
        if self._reset_pin is None:
            return

        self._reset_pin.value(self._reset_state)
        time.sleep_ms(120)  # NOQA
        self._reset_pin.value(not self._reset_state)

    def get_backlight(self):
        if self._backlight_pin is None:
            return -1

        if self._backlight_on_state == STATE_PWM:
            value = self._backlight_pin.duty_u16()  # NOQA
            return round(value / 65535 * 100.0, 2)

        value = self._backlight_pin.value()  # NOQA

        if self._power_on_state:
            return value

        return not value

    def set_backlight(self, value):
        if self._backlight_pin is None:
            return

        if self._backlight_on_state == STATE_PWM:
            self._backlight_pin.duty_u16(int(value / 100.0 * 65535.0))  # NOQA
        elif self._power_on_state:
            self._backlight_pin.value(int(bool(value)))  # NOQA
        else:
            self._backlight_pin.value(not int(bool(value)))  # NOQA

    def _dummy_set_memory_location(self, *_, **__):  # NOQA
        return _RAMWR

    def _set_memory_location(self, x1, y1, x2, y2):
        # Column addresses
        param_buf = self._param_buf  # NOQA

        param_buf[0] = (x1 >> 8) & 0xFF
        param_buf[1] = x1 & 0xFF
        param_buf[2] = (x2 >> 8) & 0xFF
        param_buf[3] = x2 & 0xFF

        self._data_bus.tx_param(_CASET, self._param_mv)

        # Page addresses
        param_buf[0] = (y1 >> 8) & 0xFF
        param_buf[1] = y1 & 0xFF
        param_buf[2] = (y2 >> 8) & 0xFF
        param_buf[3] = y2 & 0xFF

        self._data_bus.tx_param(_RASET, self._param_mv)

        return _RAMWR

    def _flush_cb(self, _, area, color_p):
        x1 = area.x1 + self._offset_x
        x2 = area.x2 + self._offset_x

        y1 = area.y1 + self._offset_y
        y2 = area.y2 + self._offset_y

        size = (
            (x2 - x1 + 1) *
            (y2 - y1 + 1) *
            lv.color_format_get_size(self._color_space)
        )

        cmd = self._set_memory_location(x1, y1, x2, y2)

        # we have to use the __dereference__ method because this method is
        # what converts from the C_Array object the binding passes into a
        # memoryview object that can be passed to the bus drivers
        data_view = color_p.__dereference__(size)
        self._data_bus.tx_color(cmd, data_view, x1, y1, x2, y2,
                                self._rotation, self._disp_drv.flush_is_last())

    # we always register this callback no matter what. This is what tells LVGL
    # that the buffer is able to be written to. If this callback doesn't get
    # registered then the flush function is going to block until the buffer
    # gets emptied. Everything is handeled internally in the bus driver if
    # using DMA and double buffer or a single buffer.

    def _flush_ready_cb(self, *_):
        self._disp_drv.flush_ready()

    def _madctl(self, colormode, rotations, rotation=None):
        if rotation is None:
            rotation = ~self._rotation

        # if rotation is 0 or positive use the value as is.
        if rotation >= 0:
            return rotation | colormode

        # otherwise use abs(rotation)-1 as index to
        # retrieve value from rotations set

        index = ~rotation
        if index > len(rotations):
            RuntimeError('Invalid display orientation value specified')

        return rotations[index] | colormode
