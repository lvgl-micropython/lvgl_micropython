from micropython import const
import pointer_framework

def SDL_BUTTON(X):
    return 1 << (X - 1)


SDL_BUTTON_LEFT = 1
SDL_BUTTON_MIDDLE = 2
SDL_BUTTON_RIGHT = 3
SDL_BUTTON_X1 = 4
SDL_BUTTON_X2 = 5
SDL_BUTTON_LMASK = SDL_BUTTON(SDL_BUTTON_LEFT)
SDL_BUTTON_MMASK = SDL_BUTTON(SDL_BUTTON_MIDDLE)
SDL_BUTTON_RMASK = SDL_BUTTON(SDL_BUTTON_RIGHT)
SDL_BUTTON_X1MASK = SDL_BUTTON(SDL_BUTTON_X1)
SDL_BUTTON_X2MASK = SDL_BUTTON(SDL_BUTTON_X2)


class SDLMouse(pointer_framework.PointerDriver):
    def __init__(self):
        super().__init__(None)
        self.__current_state = self.RELEASED
        self.__x = -1
        self.__y = -1

    def _motion_cb(self, kwargs):
        kwargs['state']
        kwargs['x']
        kwargs['y']

    def _wheel_cb(self, kwargs):
        kwargs['state']
        kwargs['x']
        kwargs['y']

register_mouse_motion_callback
state
id
x
y


register_mouse_button_callback
button
id
state
clicks
x
y


register_mouse_wheel_callback
idx
x
y
mousex
mouseY