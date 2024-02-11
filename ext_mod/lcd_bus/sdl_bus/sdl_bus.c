/**
 * @file lv_sdl_window.h
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "sdl_bus.h"

#ifdef SDL_INCLUDE_PATH
    #include <stdbool.h>

    #define SDL_MAIN_HANDLED /*To fix SDL's "undefined reference to WinMain" issue*/

    #include SDL_INCLUDE_PATH
    #include SDL_THREAD_INCLUDE_PATH
    #include SDL_IMAGE_INCLUDE_PATH

    mp_lcd_err_t sdl_tx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t sdl_rx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t sdl_tx_color(lcd_panel_io_t *io, int lcd_cmd, void *color, size_t color_size);
    mp_lcd_err_t sdl_del(lcd_panel_io_t *io);
    mp_lcd_err_t sdl_init(lcd_panel_io_t *io, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size);
    mp_lcd_err_t sdl_get_lane_count(lcd_panel_io_t *io, uint8_t *lane_count);

    int flush_cb(void *self_in);

    STATIC mp_obj_t mp_lcd_i2c_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        enum {
            ARG_flags
        };

        const mp_arg_t make_new_args[] = {
            { MP_QSTR_flags,                 MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       }
        };

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

        return MP_OBJ_FROM_PTR(self);
    }

    mp_lcd_err_t sdl_rx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size)
    {
        LCD_UNUSED(io);
        LCD_UNUSED(lcd_cmd);
        LCD_UNUSED(param);
        LCD_UNUSED(param_size);
        return LCD_OK;
    }


    mp_lcd_err_t sdl_tx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size)
    {
        LCD_UNUSED(io);
        LCD_UNUSED(lcd_cmd);
        LCD_UNUSED(param);
        LCD_UNUSED(param_size);
        return LCD_OK;
    }


    mp_lcd_err_t sdl_tx_color(lcd_panel_io_t *io, int lcd_cmd, void *color, size_t color_size)
    {
        mp_lcd_sdl_bus_obj_t *self = __containerof(io, mp_lcd_sdl_bus_obj_t, panel_io_handle);

        self->panel_io_config.buf_to_flush = color;
        SDL_SemPost(self->panel_io_config.sem);

        return LCD_OK;
    }


    mp_lcd_err_t sdl_del(lcd_panel_io_t *io)
    {
        mp_lcd_sdl_bus_obj_t *self = __containerof(io, mp_lcd_sdl_bus_obj_t, panel_io_handle);

        self->panel_io_config.buf_to_flush = NULL;
        self->panel_io_config.exit_thread = true;
        SDL_SemPost(self->panel_io_config.sem);
        SDL_WaitThread(self->panel_io_config.thread, NULL);
        SDL_DestroyTexture(self->texture);
        SDL_DestroyRenderer(self->renderer);
        SDL_DestroyWindow(self->window);
        SDL_DestroySemaphore(self->panel_io_config.sem);
        return LCD_OK;
    }

    mp_lcd_err_t sdl_init(lcd_panel_io_t *io, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size)
    {
        mp_lcd_sdl_bus_obj_t *self = __containerof(io, mp_lcd_sdl_bus_obj_t, panel_io_handle);
        LCD_UNUSED(buffer_size);

        self->panel_io_config.width = width;
        self->panel_io_config.height = height;

        SDL_Init(SDL_INIT_VIDEO);
        SDL_StartTextInput();

        if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
            mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("could not initialize sdl2_image: %s"), IMG_GetError());
            return LCD_ERR_NOT_SUPPORTED;
        }

        self->panel_io_config.sem = SDL_CreateSemaphore(0);
        SDL_SemWait(self->panel_io_config.sem);
        self->window = SDL_CreateWindow(
            "LVGL MP\0",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            width,
            height,
            self->panel_io_config.flags
        );       /*last param.  to hide borders*/

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
        self->panel_io_config.thread = SDL_CreateThread(flush_cb, "LVGL_MP\0", (void *)self);

        return LCD_OK;
    }

    mp_lcd_err_t sdl_get_lane_count(lcd_panel_io_t *io, uint8_t *lane_count)
    {
        *lane_count = 1;
        return LCD_OK;
    }

    int flush_cb(void *self_in) {
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
    }

    STATIC const mp_rom_map_elem_t mp_lcd_sdl_bus_locals_dict_table[] = {
        { MP_ROM_QSTR(MP_QSTR_get_lane_count),    MP_ROM_PTR(&mp_lcd_bus_get_lane_count_obj)    },
        { MP_ROM_QSTR(MP_QSTR_register_callback), MP_ROM_PTR(&mp_lcd_bus_register_callback_obj)     },
        { MP_ROM_QSTR(MP_QSTR_tx_color),          MP_ROM_PTR(&mp_lcd_rgb_bus_tx_color_obj)          },
        { MP_ROM_QSTR(MP_QSTR_rx_param),          MP_ROM_PTR(&mp_lcd_rgb_bus_rx_param_obj)          },
        { MP_ROM_QSTR(MP_QSTR_tx_param),          MP_ROM_PTR(&mp_lcd_rgb_bus_tx_param_obj)          },
        { MP_ROM_QSTR(MP_QSTR_free_framebuffer),     MP_ROM_PTR(&mp_lcd_bus_free_framebuffer_obj)     },
        { MP_ROM_QSTR(MP_QSTR_allocate_framebuffer), MP_ROM_PTR(&mp_lcd_bus_allocate_framebuffer_obj) },
        { MP_ROM_QSTR(MP_QSTR_init),              MP_ROM_PTR(&mp_lcd_bus_init_obj)              },
        { MP_ROM_QSTR(MP_QSTR_deinit),            MP_ROM_PTR(&mp_lcd_bus_deinit_obj)            },
        { MP_ROM_QSTR(MP_QSTR___del__),           MP_ROM_PTR(&mp_lcd_bus_deinit_obj)            }
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
