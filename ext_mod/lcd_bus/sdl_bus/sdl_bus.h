// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"
#include "modlcd_bus.h"
#include <stdbool.h>


#ifndef _SDL_BUS_H
    #define _SDL_BUS_H

    #define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/

    #ifdef MP_PORT_UNIX
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

            mp_obj_array_t *view1;
            mp_obj_array_t *view2;

            uint32_t buffer_flags;

            uint8_t trans_done: 1;
            uint8_t num_lanes: 5;

            lcd_task_t task;
            lcd_init_t init;
            lcd_bufs_t bufs;

            lcd_tx_data_t tx_data;
            lcd_tx_cmds_t tx_cmds;

            rotation_data_t r_data;

            internal_cb_funcs_t internal_cb_funcs;

            // bus specific
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

            uint8_t *buf1;

        } mp_lcd_sdl_bus_obj_t;

        extern const mp_obj_type_t mp_lcd_sdl_bus_type;
    #endif
#endif
