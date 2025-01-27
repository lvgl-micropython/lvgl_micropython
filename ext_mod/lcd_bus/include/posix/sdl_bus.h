// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"

#ifndef _SDL_BUS_H
    #define _SDL_BUS_H

    #define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/

    #include "SDL.h"
    #include "SDL_thread.h"

    #include "common/lcd_common_types.h"
    #include "common/sw_rotate_task_common.h"
    #include "common/sw_rotate.h"
    #include "lcd_types.h"


    typedef struct {
        int32_t x;
        int32_t y;
        uint8_t state;
    } pointer_event_t;

    struct _mp_lcd_sdl_bus_obj_t {
        mp_obj_base_t base;
        lcd_panel_io_t panel_io_handle;

        mp_obj_t callback;

        mp_lcd_framebuf_t *fb1;
        mp_lcd_framebuf_t *fb2;

        uint8_t trans_done: 1;
        uint8_t sw_rotate: 1;
        uint8_t lanes: 5;

        mp_lcd_sw_rotation_t sw_rot;

        uint16_t width;
        uint16_t height;
        uint32_t win_id;
        int flags;

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

        mp_lcd_sdl_bus_obj_t* prev_instance;
    };

#endif /* _SDL_BUS_H */
