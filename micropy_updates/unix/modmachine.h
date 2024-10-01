
#ifndef __MODMACHINE_H__
    #define __MODMACHINE_H__

    #define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/

    #include "SDL.h"

    void machine_timer_deinit_all(void);


    void lcd_bus_init_sdl(void) {
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
        SDL_InitSubSystem(SDL_INIT_JOYSTICK);
        SDL_InitSubSystem(SDL_INIT_EVENTS);
        SDL_InitSubSystem(SDL_INIT_TIMER);
    }

    void lcd_bus_deinit_sdl(void) {
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
        SDL_QuitSubSystem(SDL_INIT_EVENTS);
        SDL_QuitSubSystem(SDL_INIT_TIMER);
    }

#endif /* __MODMACHINE_H__ */