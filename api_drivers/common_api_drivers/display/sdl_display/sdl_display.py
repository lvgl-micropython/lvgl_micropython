# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import lvgl as lv  # NOQA
import display_driver_framework
from micropython import const  # NOQA
import micropython  # NOQA

# Window has been shown
_SDL_WINDOWEVENT_SHOWN = const(1)
# Window has been hidden
_SDL_WINDOWEVENT_HIDDEN = const(2)
# Window has been exposed and should be redrawn
_SDL_WINDOWEVENT_EXPOSED = const(3)
# Window has been moved to data1, data2
_SDL_WINDOWEVENT_MOVED = const(4)
# Window has been resized to data1xdata2
_SDL_WINDOWEVENT_RESIZED = const(5)
# The window size has changed, either as
# a result of an API call or through the
# system or user changing the window size.
_SDL_WINDOWEVENT_SIZE_CHANGED = const(6)
# Window has been minimized
_SDL_WINDOWEVENT_MINIMIZED = const(7)
# Window has been maximized
_SDL_WINDOWEVENT_MAXIMIZED = const(8)
# Window has been restored to normal size and position
_SDL_WINDOWEVENT_RESTORED = const(9)
# Window has gained mouse focus
_SDL_WINDOWEVENT_ENTER = const(10)
# Window has lost mouse focus
_SDL_WINDOWEVENT_LEAVE = const(11)
# Window has gained keyboard focus
_SDL_WINDOWEVENT_FOCUS_GAINED = const(12)
# Window has lost keyboard focus
_SDL_WINDOWEVENT_FOCUS_LOST = const(13)
# The window manager requests that the window be closed
_SDL_WINDOWEVENT_CLOSE = const(14)
# Window is being offered a focus (should SetWindowInputFocus()
# on itself or a subwindow, or ignore)
_SDL_WINDOWEVENT_TAKE_FOCUS = const(15)
# Window had a hit test that wasn't SDL_HITTEST_NORMAL
_SDL_WINDOWEVENT_HIT_TEST = const(16)
# The ICC profile of the window's display has changed.
_SDL_WINDOWEVENT_ICCPROF_CHANGED = const(17)
# Window has been moved to display data1.
_SDL_WINDOWEVENT_DISPLAY_CHANGED = const(18)


_SDL_PIXELFORMAT_RGB888 = const(0x16161804)
_SDL_PIXELFORMAT_BGR24 = const(0x17401803)
_SDL_PIXELFORMAT_RGB565 = const(0x15151002)
_SDL_PIXELFORMAT_INDEX1MSB = const(0x11200100)
_SDL_PIXELFORMAT_INDEX4MSB = const(0x12200400)
_SDL_PIXELFORMAT_INDEX8 = const(0x13000801)
_SDL_PIXELFORMAT_RGB24 = const(0x17101803)
_SDL_PIXELFORMAT_ARGB8888 = const(0x16362004)
_SDL_PIXELFORMAT_XRGB8888 = const(0x16161804)
_SDL_PIXELFORMAT_RGBA8888 = const(0x16462004)
_SDL_PIXELFORMAT_IYUV = const(0x56555949)
_SDL_PIXELFORMAT_NV21 = const(0x3132564e)
_SDL_PIXELFORMAT_NV12 = const(0x3231564e)
_SDL_PIXELFORMAT_YUY2 = const(0x32595559)
_SDL_PIXELFORMAT_UYVY = const(0x59565955)

_active_event_poll = False


BYTE_ORDER_RGB = display_driver_framework.BYTE_ORDER_RGB
BYTE_ORDER_BGR = display_driver_framework.BYTE_ORDER_BGR


class SDLDisplay(display_driver_framework.DisplayDriver):

    def __init__(
        self,
        data_bus,
        display_width,
        display_height,
        frame_buffer1=None,
        frame_buffer2=None,
        reset_pin=None,  # NOQA
        reset_state=display_driver_framework.STATE_HIGH,  # NOQA
        power_pin=None,  # NOQA
        power_on_state=display_driver_framework.STATE_HIGH,  # NOQA
        backlight_pin=None,  # NOQA
        backlight_on_state=display_driver_framework.STATE_HIGH,  # NOQA
        offset_x=0,
        offset_y=0,
        color_byte_order=BYTE_ORDER_RGB,  # NOQA
        color_space=lv.COLOR_FORMAT.RGB888,  # NOQA
        rgb565_byte_swap=False  # NOQA
    ):
        super().__init__(
            data_bus=None,
            display_width=display_width,
            display_height=display_height,
            frame_buffer1=frame_buffer1,
            frame_buffer2=frame_buffer2,
            reset_pin=None,
            reset_state=display_driver_framework.STATE_HIGH,
            power_pin=None,
            power_on_state=display_driver_framework.STATE_HIGH,
            backlight_pin=None,
            backlight_on_state=display_driver_framework.STATE_HIGH,
            offset_x=offset_x,
            offset_y=offset_y,
            color_byte_order=color_byte_order,
            color_space=color_space,
            rgb565_byte_swap=False,
            _init_bus=False
        )

        self._data_bus = data_bus

        buffer_size = lv.color_format_get_size(color_space)
        buffer_size *= display_width * display_height

        if frame_buffer1 is None:
            frame_buffer1 = data_bus.allocate_framebuffer(buffer_size, 0)

            if frame_buffer2 is None:
                frame_buffer2 = data_bus.allocate_framebuffer(buffer_size, 0)
        else:
            if buffer_size != len(frame_buffer1):
                raise RuntimeError('frame buffer is not large enough')

        if frame_buffer2 is not None:
            if len(frame_buffer1) != len(frame_buffer2):
                raise RuntimeError('Frame buffer sizes are not equal.')

        self._frame_buffer1 = frame_buffer1
        self._frame_buffer2 = frame_buffer2

        self._disp_drv = lv.display_create(display_width, display_height)  # NOQA

        self._disp_drv.set_color_format(color_space)
        self._disp_drv.set_driver_data(self)

        mapping = {
            lv.COLOR_FORMAT.I1: _SDL_PIXELFORMAT_INDEX1MSB,  # NOQA
            lv.COLOR_FORMAT.I4: _SDL_PIXELFORMAT_INDEX4MSB,  # NOQA
            lv.COLOR_FORMAT.I8: _SDL_PIXELFORMAT_INDEX8,  # NOQA
            lv.COLOR_FORMAT.RGB565: _SDL_PIXELFORMAT_RGB565,  # NOQA
            lv.COLOR_FORMAT.RGB888: _SDL_PIXELFORMAT_RGB24,  # NOQA
            lv.COLOR_FORMAT.ARGB8888: _SDL_PIXELFORMAT_ARGB8888,  # NOQA
            lv.COLOR_FORMAT.XRGB8888: _SDL_PIXELFORMAT_RGB888,  # NOQA
            lv.COLOR_FORMAT.I420: _SDL_PIXELFORMAT_IYUV,  # NOQA
            lv.COLOR_FORMAT.NV21: _SDL_PIXELFORMAT_NV21,  # NOQA
            lv.COLOR_FORMAT.NV12: _SDL_PIXELFORMAT_NV12,  # NOQA
            lv.COLOR_FORMAT.YUY2: _SDL_PIXELFORMAT_YUY2,  # NOQA
            lv.COLOR_FORMAT.UYVY: _SDL_PIXELFORMAT_UYVY,  # NOQA
            lv.COLOR_FORMAT.RAW: _SDL_PIXELFORMAT_RGB24,  # NOQA
            lv.COLOR_FORMAT.RAW_ALPHA: _SDL_PIXELFORMAT_RGBA8888  # NOQA
        }

        cf = mapping.get(color_space, None)

        if cf is None:
            raise RuntimeError('Color format is not supported')

        if cf == _SDL_PIXELFORMAT_RGB24 and self._color_byte_order == BYTE_ORDER_BGR:
            cf = _SDL_PIXELFORMAT_BGR24

        self._cf = cf

        data_bus.init(
            display_width,
            display_height,
            lv.color_format_get_size(color_space) * 8,
            cf,
            False,
            8,
            8
        )

        self._disp_drv.set_flush_cb(self._flush_cb)

        self._disp_drv.set_buffers(
            frame_buffer1,
            frame_buffer2,
            len(frame_buffer1),
            lv.DISPLAY_RENDER_MODE.DIRECT  # NOQA
        )

        self._ignore_size_chg = False

        data_bus.register_callback(self._flush_ready_cb)

        data_bus.register_quit_callback(self._quit_cb)
        data_bus.register_window_callback(self._windows_event_cb)

        self._disp_drv.add_event_cb(
            self._res_chg_event_cb, lv.EVENT.RESOLUTION_CHANGED, None)  # NOQA
        self._disp_drv.add_event_cb(
            self._release_disp_cb, lv.EVENT.DELETE, None)  # NOQA

        global _active_event_poll

        if not _active_event_poll:
            _active_event_poll = True
            self._timer = lv.timer_create(self._timer_cb, 5, None)  # NOQA
            self._timer.set_repeat_count(-1)  # NOQA

        self._displays.append(self)

    def _timer_cb(self, _):
        self._data_bus.poll_events()

    def _quit_cb(self):
        self._disp_drv.delete()

    def _res_chg_event_cb(self, _):
        bpp = lv.color_format_get_size(self._disp_drv.get_color_format())

        hor_res = self._disp_drv.get_horizontal_resolution()
        ver_res = self._disp_drv.get_vertical_resolution()

        buf_size = int(hor_res * ver_res * bpp)

        self._frame_buffer1 = self._data_bus.realloc_buffer(buf_size, 1)  # NOQA
        self._frame_buffer2 = self._data_bus.realloc_buffer(buf_size, 2)  # NOQA

        self._disp_drv.set_buffers(
            self._frame_buffer1,
            self._frame_buffer2,
            len(self._frame_buffer1),
            lv.DISPLAY_RENDER_MODE.DIRECT  # NOQA
        )

        self._data_bus.set_window_size(
            hor_res, ver_res, self._cf, self._ignore_size_chg)

    def _windows_event_cb(self, _, event, width, height):
        if event == _SDL_WINDOWEVENT_RESIZED:
            self._ignore_size_chg = True
            self._disp_drv.set_resolution(width, height)
            self._ignore_size_chg = False
            lv.refr_now(self._disp_drv)
        elif event in (_SDL_WINDOWEVENT_TAKE_FOCUS, _SDL_WINDOWEVENT_EXPOSED):
            lv.refr_now(self._disp_drv)
        elif event == _SDL_WINDOWEVENT_CLOSE:
            self._disp_drv.delete()

    def _release_disp_cb(self, _):
        self._remove_timer()
        self._data_bus.deinit()

    def _remove_timer(self):
        global _active_event_poll

        try:
            self._timer.pause()  # NOQA
            self._timer.delete()  # NOQA
            del self._timer
            displays = self.get_displays()

            if len(displays) == 1:
                _active_event_poll = False
            else:
                disp = displays[0]
                disp._timer = lv.timer_create(disp._timer_cb, 5, None)  # NOQA
                disp._timer.set_repeat_count(-1)  # NOQA
        except AttributeError:
            pass

    def __del__(self):
        self._remove_timer()
        self._disp_drv.remove_event_cb_with_user_data(self._release_disp_cb, None)
        display_driver_framework.DisplayDriver.__del__(self)

    def set_offset(self, x, y):
        rot90 = lv.DISPLAY_ROTATION._90  # NOQA
        rot270 = lv.DISPLAY_ROTATION._270  # NOQA

        if self._rotation in (rot90, rot270):
            x, y = y, x

        self._offset_x, self._offset_y = x, y
        self._disp_drv.set_offset(x, y)

    def invert_colors(self):
        pass

    def set_rotation(self, value):
        self._disp_drv.set_rotation(value)
        self._rotation = value

    def init(self):
        self._initilized = True

    def set_params(self, cmd, params=None):
        pass

    def get_params(self, cmd, params):
        pass

    def get_power(self):
        return True

    def delete(self):
        self._disp_drv.delete()

    def reset(self):
        pass

    def get_backlight(self):
        return 100.0

    def _dummy_set_memory_location(self, *_, **__):  # NOQA
        return 0

    def _set_memory_location(self, x1, y1, x2, y2):
        return 0

    def _madctl(self, colormode, rotations, rotation=None):
        return 0
