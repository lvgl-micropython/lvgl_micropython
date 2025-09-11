// Copyright (c) 2024 - 2025 Kevin G. Schlosser

//local_includes
#include "modlcd_bus.h"

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <stdbool.h>


#ifndef _SDL_BUS_H
    #define _SDL_BUS_H

    #define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/

    // sdl includes
    #include "SDL.h"
    #include "SDL_thread.h"

    typedef struct {
        int32_t x;
        int32_t y;
        uint8_t state;
    } pointer_event_t;

    typedef struct _panel_io_config_t {
        uint16_t width;
        uint16_t height;
        uint32_t win_id;
        void *buf_to_flush;
        uint8_t bytes_per_pixel;
        int flags;
    } panel_io_config_t;

    typedef struct _mp_lcd_sdl_bus_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        void *buf1;
        void *buf2;

        bool trans_done;
        bool rgb565_byte_swap;

        lcd_panel_io_t panel_io_handle;

        panel_io_config_t panel_io_config;
        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_Texture *texture;

        pointer_event_t pointer_event;
        mp_obj_t keypad_callback;
        mp_obj_t window_callback;
        mp_obj_t mouse_callback;
        mp_obj_t quit_callback;

        bool ignore_size_chg;
        bool inited;

    } mp_lcd_sdl_bus_obj_t;

    extern const mp_obj_type_t mp_lcd_sdl_bus_type;
#endif
