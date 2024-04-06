#include "py/obj.h"
#include "modlcd_bus.h"
#include <stdbool.h>


#ifndef _SDL_BUS_H
    #define _SDL_BUS_H

    #define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/

    #ifdef MP_PORT_UNIX
        #include <SDL2/SDL.h>
        #include <SDL2/SDL_thread.h>

        typedef struct _panel_io_config_t {
            uint16_t width;
            uint16_t height;
            uint32_t win_id;
            bool exit_thread;
            void *buf_to_flush;
            SDL_Thread *thread;
            uint8_t bytes_per_pixel;
            SDL_sem *sem;
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

            mp_obj_t controller_axis_motion_cb;
            mp_obj_t controller_button_cb;
            mp_obj_t touch_cb;
            mp_obj_t keypad_cb;
            mp_obj_t joystick_axis_motion_cb;
            mp_obj_t joystick_ball_motion_cb;
            mp_obj_t joystick_hat_motion_cb;
            mp_obj_t joystick_button_cb;
            mp_obj_t mouse_motion_cb;
            mp_obj_t mouse_button_cb;
            mp_obj_t mouse_wheel_cb;
            mp_obj_t window_cb;

            bool ignore_size_chg;

        } mp_lcd_sdl_bus_obj_t;

        extern const mp_obj_type_t mp_lcd_sdl_bus_type;
    #endif
#endif
