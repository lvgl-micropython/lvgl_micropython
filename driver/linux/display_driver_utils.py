import usys as sys
sys.path.append('') # See: https://github.com/micropython/micropython/issues/6419

import lvgl as lv

try:
    import lv_utils
    lv_utils_available = True
except:
    lv_utils_available = False


ORIENT_LANDSCAPE = False
ORIENT_PORTRAIT = True


class driver:
    
    def __init__(
        self,
        width=420,
        height=320,
        orientation=ORIENT_PORTRAIT,
        asynchronous=False,
        exception_sink=None,
        defaultGroup=True
    ):

        if not lv.is_initialized():
            lv.init()

        self.group = lv.group_create()
        if defaultGroup:
            self.group.set_default()

        self.width = width
        self.height = height
        self.orientation = orientation
        self.asynchronous = asynchronous
        self.exception_sink = exception_sink
        self.disp = None
        self.touch = None
        self.type = None
        self.event_loop = None
        self.mouse = None
        self.keyboard = None

        if not (lv_utils_available and lv_utils.event_loop.is_running()):
            self.init_gui()

    def init_gui_SDL(self):
        if lv_utils_available:
            self.event_loop = lv_utils.event_loop(
                asynchronous=self.asynchronous,
                exception_sink=self.exception_sink
            )

        lv.sdl_window_create(self.width, self.height)
        self.mouse = lv.sdl_mouse_create()
        self.keyboard = lv.sdl_keyboard_create()
        self.keyboard.set_group(self.group)
        self.type = "SDL"
        print("Running the SDL lvgl version")

    def init_gui(self):
        
        # Identify platform and initialize it

        try:
            self.init_gui_SDL()
        except ImportError:
            raise RuntimeError("Could not find a suitable display driver!")
