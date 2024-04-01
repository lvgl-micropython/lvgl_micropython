import lvgl as lv  # NOQA
import display_driver_framework
from micropython import const

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
# Window has been moved to display data1.
_SDL_WINDOWEVENT_DISPLAY_CHANGED = const(16)

_SDL_PIXELFORMAT_RGB888 = const(0x16161804)
_SDL_PIXELFORMAT_BGR24 = const(0x17401803)
_SDL_PIXELFORMAT_RGB565 = const(0x15151002)



'''
SDL_WINDOWEVENT = 0x200

SDL_KEYDOWN = 0x300
SDL_KEYUP = 0x301


SDL_MOUSEMOTION = 0x400
SDL_MOUSEBUTTONDOWN = 0x401
SDL_MOUSEBUTTONUP = 0x402
SDL_MOUSEWHEEL = 0x403


SDL_JOYAXISMOTION = 0x600
SDL_JOYBALLMOTION = 0x601
SDL_JOYHATMOTION = 0x602
SDL_JOYBUTTONDOWN = 0x603
SDL_JOYBUTTONUP = 0x604
    

    

    
    
SDL_RELEASED = 0
SDL_PRESSED = 1

'''
class SDL(display_driver_framework.DisplayDriver):

    def __init__(
        self,
        data_bus,
        display_width,
        display_height,
        frame_buffer1=None,
        frame_buffer2=None,
        reset_pin=None,
        reset_state=display_driver_framework.STATE_HIGH,
        power_pin=None,
        power_on_state=display_driver_framework.STATE_HIGH,
        backlight_pin=None,
        backlight_on_state=display_driver_framework.STATE_HIGH,
        offset_x=0,
        offset_y=0,
        color_byte_order=display_driver_framework.BYTE_ORDER_RGB,
        color_space=lv.COLOR_FORMAT.RGB888,
        rgb565_byte_swap=False
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
            color_byte_order=display_driver_framework.BYTE_ORDER_RGB,
            color_space=color_space,
            rgb565_byte_swap=False
        )

        self._data_bus = data_bus

        try:
            import task_handler  # NOQA

            self._task_handler = task_handler.TaskHandler()

        except ImportError:
            self._task_handler = None

        buffer_size = lv.color_format_get_size(color_space)
        buffer_size *= display_width * display_height

        if frame_buffer1 is None:
            frame_buffer1 = data_bus.allocate_framebuffer(buffer_size, 0)
        if frame_buffer2 is None:
            frame_buffer2 = data_bus.allocate_framebuffer(buffer_size, 0)

        self._frame_buffer1 = frame_buffer1
        self._frame_buffer2 = frame_buffer2

        self._disp_drv = lv.display_create(display_width, display_height)

        self._disp_drv.set_color_format(color_space)
        self._disp_drv.set_driver_data(self)

        data_bus.init(
            display_width,
            display_height,
            lv.color_format_get_size(color_space) * 8,
            buffer_size,
            False
        )

        self._disp_drv.set_flush_cb(self._flush_cb)

        self._disp_drv.set_buffers(
            frame_buffer1,
            frame_buffer2,
            len(frame_buffer1),
            lv.DISPLAY_RENDER_MODE.DIRECT
        )

        self._ignore_size_chg = False

        data_bus.register_callback(self._flush_ready_cb)

        self._data_bus.register_window_callback(self._windows_event_cb)
        self._disp_drv.add_event_cb(self._res_chg_event_cb, lv.EVENT.RESOLUTION_CHANGED, None)
        self._disp_drv.add_event_cb(self._release_disp_cb, lv.EVENT.DELETE)

    def _res_chg_event_cb(self, e):
        bpp = lv.color_format_get_size(self._disp_drv.get_color_format()) * 8
        disp = e.get.current_target()

        hor_res = disp.get_horizontal_resolution()
        ver_res = disp.get_vertical_resolution()

        if bpp == 32:
            px_format = _SDL_PIXELFORMAT_RGB888
        elif bpp == 24:
            px_format = _SDL_PIXELFORMAT_BGR24
        elif bpp == 16:
            px_format = _SDL_PIXELFORMAT_RGB565
        else:
            return

        buf_size = int(hor_res * ver_res * bpp / 8)

        self._frame_buffer1 = self._data_bus.realloc_buffer(buf_size, 1)
        self._frame_buffer2 = self._data_bus.realloc_buffer(buf_size, 2)

        disp.set_buffers(
            self._frame_buffer1,
            self._frame_buffer2,
            len(self._frame_buffer1),
            lv.DISPLAY_RENDER_MODE.DIRECT
        )

        self._data_bus.set_window_size(hor_res, ver_res, px_format, self._ignore_size_chg)

    def _windows_event_cb(self, kwargs):
        event = kwargs['event']

        if event == _SDL_WINDOWEVENT_RESIZED:
            width = kwargs['data1']
            height = kwargs['data2']
            self._ignore_size_chg = True
            self._disp_drv.set_resolution(width, height)
            self._ignore_size_chg = False
            lv.refr_now(self._disp_drv)
        elif event in (_SDL_WINDOWEVENT_TAKE_FOCUS, _SDL_WINDOWEVENT_EXPOSED):
            lv.refr_now(self._disp_drv)
        elif event == _SDL_WINDOWEVENT_CLOSE:
            self._disp_drv.delete()

    def _release_disp_cb(self, _):
        self._data_bus.deinit()

    def set_offset(self, x, y):
        rot90 = lv.DISPLAY_ROTATION._90  # NOQA
        rot270 = lv.DISPLAY_ROTATION._270  # NOQA

        if self._rotation in (rot90, rot270):
            x, y = y, x

        self._offset_x, self._offset_y = x, y
        self._disp_drv.set_offset(x, y)

    def invert_colors(self, value):
        pass

    invert_colors = property(None, invert_colors)

    def set_rotation(self, value):
        self._disp_drv.set_orientation(value)
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

    def __del__(self):
        self._disp_drv.delete()

    def reset(self):
        pass

    def get_backlight(self):
        return 100.0

    def _dummy_set_memory_location(self, *_, **__):  # NOQA
        return 0

    def _set_memory_location(self, x1, y1, x2, y2):
        return 0

    def _flush_cb(self, _, area, color_p):
        pass

    def _flush_ready_cb(self):
        pass

    def _madctl(self, colormode, rotations, rotation=None):
        return 0




