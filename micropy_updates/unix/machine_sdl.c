// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "machine_sdl.h"
#include "SDL.h"

void init_sdl(void)
{
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    SDL_InitSubSystem(SDL_INIT_EVENTS);
}


void deinit_sdl(void)
{
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
}
