/**
 * @file lv_sdl_window.h
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "sdl_bus.h"
#include "lcd_types.h"
#include "py/obj.h"
#include "modlcd_bus.h"
#include <stdbool.h>
#include "sdl_bus.h"
#include "py/objarray.h"
#include "py/binary.h"


#ifdef MP_PORT_UNIX
    #include <SDL2/SDL.h>
    #include <SDL2/SDL_thread.h>

    mp_lcd_err_t sdl_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t sdl_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t sdl_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end);
    mp_lcd_err_t sdl_del(mp_obj_t obj);
    mp_lcd_err_t sdl_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,  bool rgb565_byte_swap);
    mp_lcd_err_t sdl_get_lane_count(mp_obj_t obj, uint8_t *lane_count);

    int flush_thread(void *self_in);
    int event_filter_cb(void *userdata, SDL_Event *event);

    STATIC mp_obj_t mp_lcd_sdl_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        enum { ARG_flags };
        const mp_arg_t make_new_args[] = {{ MP_QSTR_flags, MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED }};

        mp_arg_val_t args[MP_ARRAY_SIZE(make_new_args)];
        mp_arg_parse_all_kw_array(
            n_args,
            n_kw,
            all_args,
            MP_ARRAY_SIZE(make_new_args),
            make_new_args,
            args
        );

        // create new object
        mp_lcd_sdl_bus_obj_t *self = m_new_obj(mp_lcd_sdl_bus_obj_t);
        self->base.type = &mp_lcd_sdl_bus_type;

        self->callback = mp_const_none;

        self->panel_io_config.flags = args[ARG_flags].u_int;

        self->panel_io_handle.del = sdl_del;
        self->panel_io_handle.init = sdl_init;
        self->panel_io_handle.tx_param = sdl_tx_param;
        self->panel_io_handle.rx_param = sdl_rx_param;
        self->panel_io_handle.tx_color = sdl_tx_color;
        self->panel_io_handle.get_lane_count = sdl_get_lane_count;

        self->controller_axis_motion_cb = mp_const_none;
        self->controller_button_cb = mp_const_none;
        self->touch_cb = mp_const_none;
        self->keypad_cb = mp_const_none;
        self->joystick_axis_motion_cb = mp_const_none;
        self->joystick_ball_motion_cb = mp_const_none;
        self->joystick_hat_motion_cb = mp_const_none;
        self->joystick_button_cb = mp_const_none;
        self->mouse_motion_cb = mp_const_none;
        self->mouse_button_cb = mp_const_none;
        self->mouse_wheel_cb = mp_const_none;
        self->window_cb = mp_const_none;

        return MP_OBJ_FROM_PTR(self);
    }


    mp_lcd_err_t sdl_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        LCD_UNUSED(obj);
        LCD_UNUSED(lcd_cmd);
        LCD_UNUSED(param);
        LCD_UNUSED(param_size);
        return LCD_OK;
    }


    mp_lcd_err_t sdl_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        LCD_UNUSED(obj);
        LCD_UNUSED(lcd_cmd);
        LCD_UNUSED(param);
        LCD_UNUSED(param_size);
        return LCD_OK;
    }

    mp_lcd_err_t sdl_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end)
    {
        LCD_UNUSED(x_start);
        LCD_UNUSED(y_start);
        LCD_UNUSED(x_end);
        LCD_UNUSED(y_end);
        LCD_UNUSED(color_size);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *) obj;

        self->panel_io_config.buf_to_flush = color;
        SDL_SemPost(self->panel_io_config.sem);

        return LCD_OK;
    }


    mp_lcd_err_t sdl_del(mp_obj_t obj)
    {
        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)obj;

        self->panel_io_config.buf_to_flush = NULL;
        self->panel_io_config.exit_thread = true;
        SDL_SemPost(self->panel_io_config.sem);
        SDL_WaitThread(self->panel_io_config.thread, NULL);
        SDL_DestroyTexture(self->texture);
        SDL_DestroyRenderer(self->renderer);
        SDL_DestroyWindow(self->window);
        SDL_DestroySemaphore(self->panel_io_config.sem);
        SDL_Quit();
        return LCD_OK;
    }

    mp_lcd_err_t sdl_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,  bool rgb565_byte_swap)
    {
        LCD_UNUSED(rgb565_byte_swap);
        LCD_UNUSED(buffer_size);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)obj;

        self->panel_io_config.width = width;
        self->panel_io_config.height = height;

        SDL_Init(SDL_INIT_VIDEO);
        SDL_StartTextInput();
        SDL_FilterEvents(event_filter_cb, self);

        self->panel_io_config.sem = SDL_CreateSemaphore(0);
        SDL_SemWait(self->panel_io_config.sem);
        self->window = SDL_CreateWindow(
            "LVGL MP\0",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            width,
            height,
            self->panel_io_config.flags
        );

        self->renderer = SDL_CreateRenderer(self->window, -1, SDL_RENDERER_SOFTWARE);

        SDL_PixelFormatEnum px_format = SDL_PIXELFORMAT_RGB565;
        self->panel_io_config.bytes_per_pixel = 2;

        switch(bpp) {
            case 32:
                px_format = SDL_PIXELFORMAT_ARGB8888;
                self->panel_io_config.bytes_per_pixel = 4;
                break;
            case 24:
                px_format = SDL_PIXELFORMAT_RGB888;
                self->panel_io_config.bytes_per_pixel = 3;
                break;
            case 16:
                px_format = SDL_PIXELFORMAT_RGB565;
                self->panel_io_config.bytes_per_pixel = 2;
                break;
        }

        self->texture = SDL_CreateTexture(self->renderer, px_format, SDL_TEXTUREACCESS_STREAMING, width, height);
        SDL_SetTextureBlendMode(self->texture, SDL_BLENDMODE_BLEND);

        SDL_SetWindowSize(self->window, width, height);

        self->rgb565_byte_swap = false;
        self->panel_io_config.thread = SDL_CreateThread(flush_thread, "LVGL_MP\0", (void *)self);

        return LCD_OK;
    }

    mp_lcd_err_t sdl_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
    {
        LCD_UNUSED(obj);
        *lane_count = 1;
        return LCD_OK;
    }

    int flush_thread(void *self_in) {
        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)self_in;

        void* buf;
        int pitch = self->panel_io_config.width * self->panel_io_config.bytes_per_pixel;

        while (!self->panel_io_config.exit_thread) {
            SDL_SemWait(self->panel_io_config.sem);

            if (self->panel_io_config.buf_to_flush != NULL) {
                buf = self->panel_io_config.buf_to_flush;
                SDL_UpdateTexture(self->texture, NULL, buf, pitch);
                SDL_RenderClear(self->renderer);
                SDL_RenderCopy(self->renderer, self->texture, NULL, NULL);
                SDL_RenderPresent(self->renderer);
                bus_trans_done_cb(&self->panel_io_handle, NULL, (void *)self);
            }
        }
        return 0;
    }


    mp_obj_t mp_lcd_sdl_register_controller_axis_motion_callback(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_callback };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED  },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        self->controller_axis_motion_cb = args[ARG_callback].u_obj;

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_register_controller_axis_motion_callback_obj, 2, mp_lcd_sdl_register_controller_axis_motion_callback);


    mp_obj_t mp_lcd_sdl_register_controller_button_callback(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_callback };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED  },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        self->controller_button_cb = args[ARG_callback].u_obj;

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_register_controller_button_callback_obj, 2, mp_lcd_sdl_register_controller_button_callback);


    mp_obj_t mp_lcd_sdl_register_touch_callback(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_callback };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED  },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        self->touch_cb = args[ARG_callback].u_obj;

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_register_touch_callback_obj, 2, mp_lcd_sdl_register_touch_callback);


    mp_obj_t mp_lcd_sdl_register_keypad_callback(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_callback };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED  },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        self->keypad_cb = args[ARG_callback].u_obj;

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_register_keypad_callback_obj, 2, mp_lcd_sdl_register_keypad_callback);


    mp_obj_t mp_lcd_sdl_register_joystick_axis_motion_callback(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_callback };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED  },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        self->joystick_axis_motion_cb = args[ARG_callback].u_obj;

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_register_joystick_axis_motion_callback_obj, 2, mp_lcd_sdl_register_joystick_axis_motion_callback);


    mp_obj_t mp_lcd_sdl_register_joystick_ball_motion_callback(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_callback };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED  },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        self->joystick_ball_motion_cb = args[ARG_callback].u_obj;

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_register_joystick_ball_motion_callback_obj, 2, mp_lcd_sdl_register_joystick_ball_motion_callback);


    mp_obj_t mp_lcd_sdl_register_joystick_hat_motion_callback(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_callback };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED  },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        self->joystick_hat_motion_cb = args[ARG_callback].u_obj;

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_register_joystick_hat_motion_callback_obj, 2, mp_lcd_sdl_register_joystick_hat_motion_callback);


    mp_obj_t mp_lcd_sdl_register_joystick_button_callback(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_callback };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED  },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        self->joystick_button_cb = args[ARG_callback].u_obj;

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_register_joystick_button_callback_obj, 2, mp_lcd_sdl_register_joystick_button_callback);


    mp_obj_t mp_lcd_sdl_register_mouse_motion_callback(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_callback };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED  },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        self->mouse_motion_cb = args[ARG_callback].u_obj;

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_register_mouse_motion_callback_obj, 2, mp_lcd_sdl_register_mouse_motion_callback);


    mp_obj_t mp_lcd_sdl_register_mouse_button_callback(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_callback };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED  },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        self->mouse_button_cb = args[ARG_callback].u_obj;

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_register_mouse_button_callback_obj, 2, mp_lcd_sdl_register_mouse_button_callback);


    mp_obj_t mp_lcd_sdl_register_mouse_wheel_callback(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_callback };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED  },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        self->mouse_wheel_cb = args[ARG_callback].u_obj;

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_register_mouse_wheel_callback_obj, 2, mp_lcd_sdl_register_mouse_wheel_callback);


    mp_obj_t mp_lcd_sdl_register_window_callback(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_callback };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED  },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        self->window_cb = args[ARG_callback].u_obj;

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_register_window_callback_obj, 2, mp_lcd_sdl_register_window_callback);


    mp_obj_t mp_lcd_sdl_set_window_size(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_width, ARG_height, ARG_px_format, ARG_ignore_size_chg};
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,               MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_width,              MP_ARG_INT | MP_ARG_REQUIRED  },
            { MP_QSTR_height,             MP_ARG_INT | MP_ARG_REQUIRED  },
            { MP_QSTR_px_format,          MP_ARG_INT | MP_ARG_REQUIRED  },
            { MP_QSTR_ignore_size_chg,    MP_ARG_INT | MP_ARG_REQUIRED  }
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        self->panel_io_config.width = (uint16_t)args[ARG_width].u_int;
        self->panel_io_config.height = (uint16_t)args[ARG_height].u_int;

        if(self->texture) {
            SDL_DestroyTexture(self->texture);
        }

        self->texture = SDL_CreateTexture(
            self->renderer,
            (SDL_PixelFormatEnum)args[ARG_px_format].u_int,
            SDL_TEXTUREACCESS_STATIC,
            self->panel_io_config.width,
            self->panel_io_config.height
        );

        SDL_SetTextureBlendMode(self->texture, SDL_BLENDMODE_BLEND);

        if ((bool)args[ARG_ignore_size_chg].u_int == false) {
            SDL_SetWindowSize(self->window, (int)self->panel_io_config.width, (int)self->panel_io_config.height);
        }

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_set_window_size_obj, 5, mp_lcd_sdl_set_window_size);


    mp_obj_t mp_lcd_sdl_realloc_buffer(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_size, ARG_buf_num };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED  },
            { MP_QSTR_size,         MP_ARG_INT | MP_ARG_REQUIRED  },
            { MP_QSTR_buf_num,      MP_ARG_INT | MP_ARG_REQUIRED  },
        };

        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)args[ARG_self].u_obj;

        void *buf;
        size_t size = (size_t)args[ARG_size].u_int;

        if (args[ARG_buf_num].u_int == 1) {
            self->buf1 = m_realloc(self->buf1, size);
            buf = self->buf1;
            memset(self->buf1, 0x00, size);
        } else {
            self->buf2 = m_realloc(self->buf2, size);
            buf = self->buf2;
            memset(self->buf2, 0x00, size);
        }

        mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, buf));
        view->typecode |= 0x80; // used to indicate writable buffer
        return MP_OBJ_FROM_PTR(view);
    }

    MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_realloc_buffer_obj, 3, mp_lcd_sdl_realloc_buffer);


    int event_filter_cb(void *userdata, SDL_Event *event)
    {
        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)userdata;
        uint32_t window_id = SDL_GetWindowID(self->window);
        mp_obj_t dict;

        switch(event->type) {
            case SDL_CONTROLLERAXISMOTION:
                if (self->controller_axis_motion_cb == mp_const_none) {
                    return 0;
                }

                dict = mp_obj_new_dict(4);
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("type"), mp_obj_new_int_from_uint(event->type));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("axis"), mp_obj_new_int_from_uint(event->caxis.axis));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("id"), mp_obj_new_int(event->caxis.which));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("value"), mp_obj_new_int(event->caxis.value));
                mp_sched_schedule(self->controller_axis_motion_cb, dict);
                break;

            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
                if (self->controller_button_cb == mp_const_none) {
                    return 0;
                }
                dict = mp_obj_new_dict(4);
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("type"), mp_obj_new_int_from_uint(event->type));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("state"), mp_obj_new_int_from_uint(event->cbutton.state));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("id"), mp_obj_new_int(event->cbutton.which));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("button"), mp_obj_new_int_from_uint(event->cbutton.button));

                mp_sched_schedule(self->controller_button_cb, dict);
                break;

            case SDL_FINGERMOTION:
            case SDL_FINGERDOWN:
            case SDL_FINGERUP:
                if (event->key.windowID != window_id) {
                    return 0;
                }
                if (self->touch_cb == mp_const_none) {
                    return 0;
                }

                dict = mp_obj_new_dict(4);
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("type"), mp_obj_new_int_from_uint(event->type));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("id"), mp_obj_new_int(event->tfinger.which));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("finger_id"), mp_obj_new_int(event->tfinger.fingerId));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("x"), mp_obj_new_float(event->tfinger.x));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("y"), mp_obj_new_float(event->tfinger.y));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("pressure"), mp_obj_new_float(event->tfinger.pressure));
                mp_sched_schedule(self->touch_cb, dict);
                break;

            case SDL_CONTROLLERTOUCHPADMOTION:
            case SDL_CONTROLLERTOUCHPADDOWN:
            case SDL_CONTROLLERTOUCHPADUP:
                if (self->touch_cb == mp_const_none) {
                    return 0;
                }

                dict = mp_obj_new_dict(5);
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("type"), mp_obj_new_int_from_uint(event->type));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("id"), mp_obj_new_int(event->ctouchpad.which));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("finger_id"), mp_obj_new_int(event->ctouchpad.fingerId));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("x"), mp_obj_new_float(event->ctouchpad.x));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("y"), mp_obj_new_float(event->ctouchpad.y));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("pressure"), mp_obj_new_float(event->ctouchpad.pressure));
                mp_sched_schedule(self->touch_cb, dict);
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                if (event->key.windowID != window_id) {
                    return 0;
                }
                if (self->keypad_cb == mp_const_none) {
                    return 0;
                }

                int key =  event->key.keysym.sym;
                uint16_t mod = event->key.keysym.mod;

                if (
                    ((mod ==  KMOD_CAPS) ||
                    (mod == KMOD_LSHIFT) ||
                    (mod == KMOD_RSHIFT)) &&
                    (key >= 91 && key <= 122)
                ) {
                    key -= 32;
                }

                dict = mp_obj_new_dict(5);
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("type"), mp_obj_new_int_from_uint(event->type));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("state"), mp_obj_new_int_from_uint(event->key.state));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("repeat"), mp_obj_new_int_from_uint(event->key.repeat));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("mod"), mp_obj_new_int_from_uint(mod));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("key"), mp_obj_new_int(key));

                mp_sched_schedule(self->keypad_cb, dict);
                break;

            case SDL_JOYAXISMOTION:
                if (self->joystick_axis_motion_cb == mp_const_none) {
                    return 0;
                }

                dict = mp_obj_new_dict(4);
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("type"), mp_obj_new_int_from_uint(event->type));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("axis"), mp_obj_new_int_from_uint(event->jaxis.axis));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("id"), mp_obj_new_int(event->jaxis.which));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("value"), mp_obj_new_int(event->jaxis.value));

                mp_sched_schedule(self->joystick_axis_motion_cb, dict);
                break;

            case SDL_JOYBALLMOTION:
                if (self->joystick_ball_motion_cb == mp_const_none) {
                    return 0;
                }

                dict = mp_obj_new_dict(5);
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("type"), mp_obj_new_int_from_uint(event->type));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("ball"), mp_obj_new_int_from_uint(event->jball.ball));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("id"), mp_obj_new_int(event->jball.which));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("xrel"), mp_obj_new_int(event->jball.xrel));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("yrel"), mp_obj_new_int(event->jball.yrel));

                mp_sched_schedule(self->joystick_ball_motion_cb, dict);
                break;

            case SDL_JOYHATMOTION:
                if (self->joystick_hat_motion_cb == mp_const_none) {
                    return 0;
                }

                dict = mp_obj_new_dict(4);
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("type"), mp_obj_new_int_from_uint(event->type));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("hat"), mp_obj_new_int_from_uint(event->jhat.hat));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("id"), mp_obj_new_int(event->jhat.which));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("value"), mp_obj_new_int_from_uint(event->jhat.value));

                mp_sched_schedule(self->joystick_hat_motion_cb, dict);

                break;

            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
                if (self->joystick_button_cb == mp_const_none) {
                    return 0;
                }

                dict = mp_obj_new_dict(4);
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("type"), mp_obj_new_int_from_uint(event->type));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("button"), mp_obj_new_int_from_uint(event->jbutton.button));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("id"), mp_obj_new_int(event->jbutton.which));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("state"), mp_obj_new_int_from_uint(event->jbutton.state));

                mp_sched_schedule(self->joystick_button_cb, dict);
                break;

            case SDL_MOUSEMOTION:
                if (event->key.windowID != window_id) {
                    return 0;
                }
                if (self->mouse_motion_cb == mp_const_none) {
                    return 0;
                }

                dict = mp_obj_new_dict(5);
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("type"), mp_obj_new_int_from_uint(event->type));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("state"), mp_obj_new_int_from_uint(event->motion.state));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("id"), mp_obj_new_int(event->motion.which));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("x"), mp_obj_new_int(event->motion.x));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("y"), mp_obj_new_int(event->motion.y));

                mp_sched_schedule(self->mouse_motion_cb, dict);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (event->key.windowID != window_id) {
                    return 0;
                }
                if (self->mouse_button_cb == mp_const_none) {
                    return 0;
                }

                dict = mp_obj_new_dict(7);
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("type"), mp_obj_new_int_from_uint(event->type));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("button"), mp_obj_new_int_from_uint(event->button.button));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("id"), mp_obj_new_int(event->button.which));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("state"), mp_obj_new_int_from_uint(event->button.state));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("clicks"), mp_obj_new_int_from_uint(event->button.clicks));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("x"), mp_obj_new_int(event->button.x));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("y"), mp_obj_new_int(event->button.y));
                mp_sched_schedule(self->mouse_motion_cb, dict);
                break;

            case SDL_MOUSEWHEEL:
                if (event->key.windowID != window_id) {
                    return 0;
                }
                if (self->mouse_wheel_cb == mp_const_none) {
                    return 0;
                }

                dict = mp_obj_new_dict(4);
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("type"), mp_obj_new_int_from_uint(event->type));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("id"), mp_obj_new_int(event->wheel.which));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("x"), mp_obj_new_int(event->wheel.x));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("y"), mp_obj_new_int(event->wheel.y));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("mouseX"), mp_obj_new_int(event->wheel.mouseX));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("mouseY"), mp_obj_new_int(event->wheel.mouseY));

                mp_sched_schedule(self->mouse_wheel_cb, dict);
                break;

            case SDL_QUIT:
                SDL_Quit();
                break;

            case SDL_WINDOWEVENT:
                if (event->key.windowID != window_id) {
                    return 0;
                }
                if (self->window_cb == mp_const_none) {
                    return 0;
                }

                dict = mp_obj_new_dict(4);
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("type"), mp_obj_new_int_from_uint(event->type));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("event"), mp_obj_new_int_from_uint(event->window.event));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("data1"), mp_obj_new_int(event->window.data1));
                mp_obj_dict_store(dict, MP_OBJ_NEW_QSTR("data2"), mp_obj_new_int(event->window.data2));

                mp_sched_schedule(self->mouse_motion_cb, dict);
                break;
        }
        return 0;
    }

    STATIC const mp_rom_map_elem_t mp_lcd_sdl_bus_locals_dict_table[] = {
        { MP_ROM_QSTR(MP_QSTR_get_lane_count),       MP_ROM_PTR(&mp_lcd_bus_get_lane_count_obj)       },
        { MP_ROM_QSTR(MP_QSTR_register_callback),    MP_ROM_PTR(&mp_lcd_bus_register_callback_obj)    },
        { MP_ROM_QSTR(MP_QSTR_tx_color),             MP_ROM_PTR(&mp_lcd_bus_tx_color_obj)         },
        { MP_ROM_QSTR(MP_QSTR_rx_param),             MP_ROM_PTR(&mp_lcd_bus_rx_param_obj)         },
        { MP_ROM_QSTR(MP_QSTR_tx_param),             MP_ROM_PTR(&mp_lcd_bus_tx_param_obj)         },
        { MP_ROM_QSTR(MP_QSTR_free_framebuffer),     MP_ROM_PTR(&mp_lcd_bus_free_framebuffer_obj)     },
        { MP_ROM_QSTR(MP_QSTR_allocate_framebuffer), MP_ROM_PTR(&mp_lcd_bus_allocate_framebuffer_obj) },
        { MP_ROM_QSTR(MP_QSTR_init),                 MP_ROM_PTR(&mp_lcd_bus_init_obj)                 },
        { MP_ROM_QSTR(MP_QSTR_deinit),               MP_ROM_PTR(&mp_lcd_bus_deinit_obj)               },
        { MP_ROM_QSTR(MP_QSTR___del__),              MP_ROM_PTR(&mp_lcd_bus_deinit_obj)               },
        { MP_ROM_QSTR(MP_QSTR_set_window_size),      MP_ROM_PTR(&mp_lcd_sdl_set_window_size_obj)      },
        { MP_ROM_QSTR(MP_QSTR_realloc_buffer),       MP_ROM_PTR(&mp_lcd_sdl_realloc_buffer_obj)       },
        { MP_ROM_QSTR(MP_QSTR_register_controller_axis_motion_callback), MP_ROM_PTR(&mp_lcd_sdl_register_controller_axis_motion_callback_obj) },
        { MP_ROM_QSTR(MP_QSTR_register_controller_button_callback),      MP_ROM_PTR(&mp_lcd_sdl_register_controller_button_callback_obj)      },
        { MP_ROM_QSTR(MP_QSTR_register_touch_callback),                  MP_ROM_PTR(&mp_lcd_sdl_register_touch_callback_obj)                  },
        { MP_ROM_QSTR(MP_QSTR_register_keypad_callback),                 MP_ROM_PTR(&mp_lcd_sdl_register_keypad_callback_obj)                 },
        { MP_ROM_QSTR(MP_QSTR_register_joystick_axis_motion_callback),   MP_ROM_PTR(&mp_lcd_sdl_register_joystick_axis_motion_callback_obj)   },
        { MP_ROM_QSTR(MP_QSTR_register_joystick_ball_motion_callback),   MP_ROM_PTR(&mp_lcd_sdl_register_joystick_ball_motion_callback_obj)   },
        { MP_ROM_QSTR(MP_QSTR_register_joystick_hat_motion_callback),    MP_ROM_PTR(&mp_lcd_sdl_register_joystick_hat_motion_callback_obj)    },
        { MP_ROM_QSTR(MP_QSTR_register_joystick_button_callback),        MP_ROM_PTR(&mp_lcd_sdl_register_joystick_button_callback_obj)        },
        { MP_ROM_QSTR(MP_QSTR_register_mouse_motion_callback),           MP_ROM_PTR(&mp_lcd_sdl_register_mouse_motion_callback_obj)           },
        { MP_ROM_QSTR(MP_QSTR_register_mouse_button_callback),           MP_ROM_PTR(&mp_lcd_sdl_register_mouse_button_callback_obj)           },
        { MP_ROM_QSTR(MP_QSTR_register_mouse_wheel_callback),            MP_ROM_PTR(&mp_lcd_sdl_register_mouse_wheel_callback_obj)            },
        { MP_ROM_QSTR(MP_QSTR_register_window_callback),                 MP_ROM_PTR(&mp_lcd_sdl_register_window_callback_obj)                 },
        { MP_ROM_QSTR(MP_QSTR_WINDOW_FULLSCREEN),         MP_ROM_INT(SDL_WINDOW_FULLSCREEN)         },
        { MP_ROM_QSTR(MP_QSTR_WINDOW_FULLSCREEN_DESKTOP), MP_ROM_INT(SDL_WINDOW_FULLSCREEN_DESKTOP) },
        { MP_ROM_QSTR(MP_QSTR_WINDOW_BORDERLESS),         MP_ROM_INT(SDL_WINDOW_BORDERLESS)         },
        { MP_ROM_QSTR(MP_QSTR_WINDOW_MINIMIZED),          MP_ROM_INT(SDL_WINDOW_MINIMIZED)          },
        { MP_ROM_QSTR(MP_QSTR_WINDOW_MAXIMIZED),          MP_ROM_INT(SDL_WINDOW_MAXIMIZED)          },
        { MP_ROM_QSTR(MP_QSTR_WINDOW_ALLOW_HIGHDPI),      MP_ROM_INT(SDL_WINDOW_ALLOW_HIGHDPI)      },
        { MP_ROM_QSTR(MP_QSTR_WINDOW_ALWAYS_ON_TOP),      MP_ROM_INT(SDL_WINDOW_ALWAYS_ON_TOP)      },
        { MP_ROM_QSTR(MP_QSTR_WINDOW_SKIP_TASKBAR),       MP_ROM_INT(SDL_WINDOW_SKIP_TASKBAR)       },
        { MP_ROM_QSTR(MP_QSTR_WINDOW_UTILITY),            MP_ROM_INT(SDL_WINDOW_UTILITY)            },
        { MP_ROM_QSTR(MP_QSTR_WINDOW_TOOLTIP),            MP_ROM_INT(SDL_WINDOW_TOOLTIP)            },
        { MP_ROM_QSTR(MP_QSTR_WINDOW_POPUP_MENU),         MP_ROM_INT(SDL_WINDOW_POPUP_MENU)         }
    };


    STATIC MP_DEFINE_CONST_DICT(mp_lcd_sdl_bus_locals_dict, mp_lcd_sdl_bus_locals_dict_table);

    MP_DEFINE_CONST_OBJ_TYPE(
        mp_lcd_sdl_bus_type,
        MP_QSTR_SDLBus,
        MP_TYPE_FLAG_NONE,
        make_new, mp_lcd_sdl_bus_make_new,
        locals_dict, (mp_obj_dict_t *)&mp_lcd_sdl_bus_locals_dict
    );
#endif
