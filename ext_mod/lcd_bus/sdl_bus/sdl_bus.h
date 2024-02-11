
#ifndef _SDL_BUS_H
    #define _SDL_BUS_H

    #ifdef SDL_INCLUDE_PATH
        #include SDL_INCLUDE_PATH
        #include SDL_THREAD_INCLUDE_PATH

        typedef struct _panel_io_config_t {
            uint16_t width;
            uint16_t height;
            uint32_t win_id;
            bool exit_thread;
            void *buf_to_flush;
            SDL_Thread *thread;
            uint8_t bytes_per_pixel;
            SDL_sem *sem;
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
        } mp_lcd_sdl_bus_obj_t;

        extern const mp_obj_type_t mp_lcd_sdl_bus_type;
    #endif
#endif
