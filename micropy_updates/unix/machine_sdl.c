/* Copyright (C) 2024  Kevin G Schlosser
 * Code that is written by the above named is done under the GPL license
 * and that license is able to be viewed in the LICENSE file in the root
 * of this project.
 */

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
