// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "sdl_bus.h"
#include "lcd_types.h"
#include "py/obj.h"
#include "modlcd_bus.h"
#include <stdbool.h>
#include "sdl_bus.h"
#include "py/objarray.h"
#include "py/binary.h"

// mp_printf(&mp_plat_print, "incomming event %d\n", event->type);

#ifdef MP_PORT_UNIX
    #include "SDL.h"
    #include "SDL_thread.h"

    mp_lcd_sdl_bus_obj_t *instances[10] = { NULL };

    uint8_t instance_count = 0;

    mp_lcd_err_t sdl_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t sdl_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t sdl_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update);
    mp_lcd_err_t sdl_del(mp_obj_t obj);
    mp_lcd_err_t sdl_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,  bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
    mp_lcd_err_t sdl_get_lane_count(mp_obj_t obj, uint8_t *lane_count);

    int flush_thread(void *self_in);
    int process_event(mp_lcd_sdl_bus_obj_t *self, SDL_Event *event);


    static mp_obj_t mp_lcd_sdl_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        enum { ARG_flags };
        const mp_arg_t make_new_args[] = {{ MP_QSTR_flags, MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED, { .u_int = -1 } } };

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

        self->pointer_event = (pointer_event_t){
            .x=0,
            .y=0,
            .state=0,
        };

        self->quit_callback = mp_const_none;
        self->mouse_callback = mp_const_none;
        self->window_callback = mp_const_none;
        self->keypad_callback = mp_const_none;
        self->inited = false;

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

    mp_lcd_err_t sdl_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update)
    {
        LCD_UNUSED(x_start);
        LCD_UNUSED(y_start);
        LCD_UNUSED(x_end);
        LCD_UNUSED(y_end);
        LCD_UNUSED(color_size);
        LCD_UNUSED(rotation);
        LCD_UNUSED(last_update);

        mp_lcd_sdl_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

        int pitch = self->panel_io_config.width * self->panel_io_config.bytes_per_pixel;

        SDL_UpdateTexture(self->texture, NULL, color, pitch);
        SDL_RenderClear(self->renderer);
        SDL_RenderCopy(self->renderer, self->texture, NULL, NULL);
        SDL_RenderPresent(self->renderer);

        if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
            mp_call_function_n_kw(self->callback, 0, 0, NULL);
        }

        return LCD_OK;
    }

    mp_lcd_err_t sdl_del(mp_obj_t obj)
    {
        mp_lcd_sdl_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

        SDL_DestroyTexture(self->texture);
        SDL_DestroyRenderer(self->renderer);
        SDL_DestroyWindow(self->window);

        uint8_t i = 0;

        for (;i < instance_count;i++) {
            if (instances[i] == self) {
                instances[i] = NULL;
                break;
            }
        }

        if (instances[i] == NULL) {
            i++;
            for (;i < instance_count;i++) {
                instances[i - 1] = instances[i];
            }
            instance_count--;
        }

        if (instance_count == 0) {
            SDL_Quit();
        }

        return LCD_OK;
    }

    mp_lcd_err_t sdl_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits)
    {
        LCD_UNUSED(rgb565_byte_swap);
        LCD_UNUSED(cmd_bits);
        LCD_UNUSED(param_bits);

        mp_lcd_sdl_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

        self->panel_io_config.width = width;
        self->panel_io_config.height = height;

        SDL_StartTextInput();

        self->window = SDL_CreateWindow(
            "LVGL MP\0",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            width,
            height,
            self->panel_io_config.flags
        );

        self->renderer = SDL_CreateRenderer(self->window, -1, SDL_RENDERER_SOFTWARE);

        self->panel_io_config.bytes_per_pixel = bpp / 8;
        self->texture = SDL_CreateTexture(self->renderer, (SDL_PixelFormatEnum)buffer_size, SDL_TEXTUREACCESS_STREAMING, width, height);
        SDL_SetTextureBlendMode(self->texture, SDL_BLENDMODE_BLEND);
        SDL_SetWindowSize(self->window, width, height);

        self->rgb565_byte_swap = false;
        self->trans_done = true;

        instance_count += 1;
        if (instance_count > 10) {
            mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("Only 10 displays are supported"));
        }
        instances[instance_count - 1] = self;

        self->inited = true;

        return LCD_OK;
    }

    mp_lcd_err_t sdl_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
    {
        LCD_UNUSED(obj);
        *lane_count = 1;
        return LCD_OK;
    }

    static mp_obj_t mp_lcd_sdl_poll_events(mp_obj_t self_in)
    {
        LCD_UNUSED(self_in);
        //mp_printf(&mp_plat_print, "mp_lcd_sdl_poll_events\n");

        mp_lcd_sdl_bus_obj_t *self = NULL;
        SDL_Event event;

        while (SDL_PollEvent(&event) > 0) {
            for (uint8_t i=0;i < instance_count;i++) {
                self = instances[i];
                if (process_event(self, &event) == 1) {
                    break;
                }
            }
        }

        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_1(mp_lcd_sdl_poll_events_obj, mp_lcd_sdl_poll_events);

    static mp_obj_t mp_lcd_sdl_register_quit_callback(mp_obj_t self_in, mp_obj_t callback)
    {
        mp_lcd_sdl_bus_obj_t *self = MP_OBJ_TO_PTR(self_in);
        self->quit_callback = callback;
        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_2(mp_lcd_sdl_register_quit_callback_obj, mp_lcd_sdl_register_quit_callback);


    static mp_obj_t mp_lcd_sdl_register_mouse_callback(mp_obj_t self_in, mp_obj_t callback)
    {
        mp_lcd_sdl_bus_obj_t *self = MP_OBJ_TO_PTR(self_in);
        self->mouse_callback = callback;
        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_2(mp_lcd_sdl_register_mouse_callback_obj, mp_lcd_sdl_register_mouse_callback);


    static mp_obj_t mp_lcd_sdl_register_keypad_callback(mp_obj_t self_in, mp_obj_t callback)
    {
        mp_lcd_sdl_bus_obj_t *self = MP_OBJ_TO_PTR(self_in);
        self->keypad_callback = callback;
        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_2(mp_lcd_sdl_register_keypad_callback_obj, mp_lcd_sdl_register_keypad_callback);


    static mp_obj_t mp_lcd_sdl_register_window_callback(mp_obj_t self_in, mp_obj_t callback)
    {
        mp_lcd_sdl_bus_obj_t *self = MP_OBJ_TO_PTR(self_in);
        self->window_callback = callback;
        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_2(mp_lcd_sdl_register_window_callback_obj, mp_lcd_sdl_register_window_callback);


    static mp_obj_t mp_lcd_sdl_set_window_size(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_width, ARG_height, ARG_px_format, ARG_ignore_size_chg};
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,               MP_ARG_OBJ | MP_ARG_REQUIRED, { .u_obj = mp_const_none } },
            { MP_QSTR_width,              MP_ARG_INT | MP_ARG_REQUIRED, { .u_int = -1            } },
            { MP_QSTR_height,             MP_ARG_INT | MP_ARG_REQUIRED, { .u_int = -1            } },
            { MP_QSTR_px_format,          MP_ARG_INT | MP_ARG_REQUIRED, { .u_int = -1            } },
            { MP_QSTR_ignore_size_chg,    MP_ARG_INT | MP_ARG_REQUIRED, { .u_bool = false        } },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);

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


    static mp_obj_t mp_lcd_sdl_realloc_buffer(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_size, ARG_buf_num };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED, { .u_obj = mp_const_none } },
            { MP_QSTR_size,         MP_ARG_INT | MP_ARG_REQUIRED, { .u_int = -1            } },
            { MP_QSTR_buf_num,      MP_ARG_INT | MP_ARG_REQUIRED, { .u_int = -1            } },
        };

        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        mp_lcd_sdl_bus_obj_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);

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


    int process_event(mp_lcd_sdl_bus_obj_t *self, SDL_Event * event)
    {
        if (!self->inited) return 0;

        uint32_t window_id = SDL_GetWindowID(self->window);
        uint32_t window_flags = SDL_GetWindowFlags(self->window);

        //mp_printf(&mp_plat_print, "incomming event %d\n", event->type);

        switch(event->type) {
            case SDL_FINGERMOTION:
            case SDL_FINGERDOWN:
            case SDL_FINGERUP:
                if (event->tfinger.windowID != window_id) return 0;
                if (self->mouse_callback == mp_const_none) return 0;

                self->pointer_event.state = event->type == SDL_FINGERUP ? 0 : 1;
                self->pointer_event.x = (int32_t)event->tfinger.x;
                self->pointer_event.y = (int32_t)event->tfinger.y;

                mp_obj_t res1[6];
                res1[0] = mp_obj_new_int_from_uint(event->type);
                res1[1] = mp_obj_new_int_from_uint(self->pointer_event.state);
                res1[2] = mp_obj_new_int(self->pointer_event.x);
                res1[3] = mp_obj_new_int(self->pointer_event.y);
                res1[4] = mp_obj_new_int(0);
                res1[5] = mp_obj_new_int(0);

                mp_call_function_n_kw(self->mouse_callback, 6, 0, res1);

                return 1;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                if (event->key.windowID != window_id) return 0;
                if (self->keypad_callback == mp_const_none) return 0;

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

                mp_obj_t res2[4];
                res2[0] = mp_obj_new_int_from_uint(event->type);
                res2[1] = mp_obj_new_int_from_uint(event->key.state);
                res2[2] = mp_obj_new_int(key);
                res2[3] = mp_obj_new_int_from_uint(mod);

                mp_call_function_n_kw(self->keypad_callback, 4, 0, res2);

                return 1;

            case SDL_MOUSEMOTION:
                if (event->motion.windowID != window_id) return 0;
                if (self->mouse_callback == mp_const_none) return 0;

                if (event->motion.state == SDL_BUTTON(SDL_BUTTON_RIGHT) || event->motion.state == SDL_BUTTON(SDL_BUTTON_LEFT)) {
                    self->pointer_event.state = 1;
                } else {
                    self->pointer_event.state = 0;
                }

                self->pointer_event.x = (int32_t)event->motion.x;
                self->pointer_event.y = (int32_t)event->motion.y;

                mp_obj_t res3[6];
                res3[0] = mp_obj_new_int_from_uint(event->type);
                res3[1] = mp_obj_new_int_from_uint(self->pointer_event.state);
                res3[2] = mp_obj_new_int(self->pointer_event.x);
                res3[3] = mp_obj_new_int(self->pointer_event.y);
                res3[4] = mp_obj_new_int(0);
                res3[5] = mp_obj_new_int(0);

                mp_call_function_n_kw(self->mouse_callback, 6, 0, res3);

                return 1;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                if (event->button.windowID != window_id) return 0;
                if (self->mouse_callback == mp_const_none) return 0;

                self->pointer_event.state = event->type == SDL_MOUSEBUTTONUP ? 0 : 1;
                self->pointer_event.x = (int32_t)event->button.x;
                self->pointer_event.y = (int32_t)event->button.y;

                mp_obj_t res4[6];
                res4[0] = mp_obj_new_int_from_uint(event->type);
                res4[1] = mp_obj_new_int_from_uint(self->pointer_event.state);
                res4[2] = mp_obj_new_int(self->pointer_event.x);
                res4[3] = mp_obj_new_int(self->pointer_event.y);
                res4[4] = mp_obj_new_int(0);
                res4[5] = mp_obj_new_int(0);

                mp_call_function_n_kw(self->mouse_callback, 6, 0, res4);

                return 1;

            case SDL_MOUSEWHEEL:
                if (event->wheel.windowID != window_id) return 0;
                if (self->mouse_callback == mp_const_none) return 0;

                self->pointer_event.x += (int32_t)event->wheel.mouseX;
                self->pointer_event.y += (int32_t)event->wheel.mouseY;

                mp_obj_t res5[6];
                res5[0] = mp_obj_new_int_from_uint(event->type);
                res5[1] = mp_obj_new_int_from_uint(self->pointer_event.state);
                res5[2] = mp_obj_new_int(self->pointer_event.x);
                res5[3] = mp_obj_new_int(self->pointer_event.y);
                res5[4] = mp_obj_new_int(event->wheel.x);
                res5[5] = mp_obj_new_int(event->wheel.y);

                mp_call_function_n_kw(self->mouse_callback, 6, 0, res5);

                return 1;

            case SDL_QUIT:
                if (self->quit_callback == mp_const_none) return 0;
                mp_call_function_n_kw(self->quit_callback, 0, 0, NULL);
                return 0;

            case SDL_WINDOWEVENT:
                if (event->window.windowID != window_id) return 0;
                if (self->window_callback == mp_const_none) return 0;

                mp_obj_t res6[4];
                res6[0] = mp_obj_new_int_from_uint(event->type);
                res6[1] = mp_obj_new_int_from_uint(event->window.event);
                res6[2] = mp_obj_new_int(event->window.data1);
                res6[3] = mp_obj_new_int(event->window.data2);

                mp_call_function_n_kw(self->window_callback, 4, 0, res6);
                return 1;

            default:
                if (((window_flags | SDL_WINDOW_INPUT_FOCUS) != window_flags) && ((window_flags | SDL_WINDOW_MOUSE_FOCUS) != window_flags)) return 0;
                if (self->mouse_callback == mp_const_none) return 0;
                switch(event->type) {
                    case SDL_CONTROLLERAXISMOTION:
                        switch(event->caxis.axis) {
                            case 0:  // AXIS_LEFT_X
                            case 2:  // AXIS_RIGHT_X
                                self->pointer_event.x = (int32_t)(self->panel_io_config.width / 10 * (event->caxis.value / 32767));
                                break;
                            case 1:  // AXIS_LEFT_Y
                            case 3:  // AXIS_RIGHT_Y
                                self->pointer_event.y = (int32_t)(self->panel_io_config.height / 10 * (event->caxis.value / 32767));
                                break;
                            default:
                                return 0;
                        }
                        break;

                    case SDL_CONTROLLERBUTTONDOWN:
                    case SDL_CONTROLLERBUTTONUP:
                        self->pointer_event.state = event->type == SDL_CONTROLLERBUTTONUP ? 0 : 1;
                        break;

                    case SDL_CONTROLLERTOUCHPADMOTION:
                    case SDL_CONTROLLERTOUCHPADDOWN:
                    case SDL_CONTROLLERTOUCHPADUP:
                        self->pointer_event.state = event->type == SDL_CONTROLLERTOUCHPADUP ? 0 : 1;
                        self->pointer_event.x = (int32_t)event->ctouchpad.x;
                        self->pointer_event.y = (int32_t)event->ctouchpad.y;
                        break;

                    case SDL_JOYAXISMOTION:
                        switch(event->caxis.axis) {
                            case 0:  // AXIS_X
                                self->pointer_event.x = (int32_t)(self->panel_io_config.width / 10 * (event->jaxis.value / 32767));
                                break;
                            case 1:  // AXIS_Y
                                self->pointer_event.y = (int32_t)(self->panel_io_config.height / 10 * (event->jaxis.value / 32767));
                                break;
                            default:
                                return 0;
                        }
                        break;

                    case SDL_JOYBALLMOTION:
                        self->pointer_event.x += (int32_t)event->jball.xrel;
                        self->pointer_event.y += (int32_t)event->jball.yrel;
                        break;

                    case SDL_JOYHATMOTION:
                        switch(event->jhat.hat) {
                            case 0x00: //HAT_CENTERED
                                break;
                            case 0x01: //HAT_UP
                                self->pointer_event.y-= 1;
                                break;
                            case 0x02: //_HAT_RIGHT
                                self->pointer_event.x += 1;
                                break;
                            case 0x04: //_HAT_DOWN
                                self->pointer_event.y += 1;
                                break;
                            case 0x08: //_HAT_LEFT
                                self->pointer_event.x -= 1;
                                break;
                            case 0x09: //_HAT_LEFTUP
                                self->pointer_event.x -= 1;
                                self->pointer_event.y-= 1;
                                break;
                            case 0x03: //_HAT_RIGHTUP
                                self->pointer_event.x += 1;
                                self->pointer_event.y-= 1;
                                break;
                            case 0x0C: //_HAT_LEFTDOWN
                                self->pointer_event.x -= 1;
                                self->pointer_event.y += 1;
                                break;
                            case 0x06: //_HAT_RIGHTDOWN
                                self->pointer_event.x += 1;
                                self->pointer_event.y += 1;
                                break;
                            default:
                                return 0;
                        }
                        break;

                    case SDL_JOYBUTTONDOWN:
                    case SDL_JOYBUTTONUP:
                        self->pointer_event.state = event->type == SDL_JOYBUTTONUP ? 0 : 1;
                        break;
                    default:
                        return 0;
                }
                mp_obj_t res7[6];
                res7[0] = mp_obj_new_int_from_uint(event->type);
                res7[1] = mp_obj_new_int_from_uint(self->pointer_event.state);
                res7[2] = mp_obj_new_int(self->pointer_event.x);
                res7[3] = mp_obj_new_int(self->pointer_event.y);
                res7[4] = mp_obj_new_int(0);
                res7[5] = mp_obj_new_int(0);

                mp_call_function_n_kw(self->mouse_callback, 6, 0, res7);

                return 1;
        }
        return 0;
    }

    static const mp_rom_map_elem_t mp_lcd_sdl_bus_locals_dict_table[] = {
        { MP_ROM_QSTR(MP_QSTR_get_lane_count),       MP_ROM_PTR(&mp_lcd_bus_get_lane_count_obj)       },
        { MP_ROM_QSTR(MP_QSTR_register_callback),    MP_ROM_PTR(&mp_lcd_bus_register_callback_obj)    },
        { MP_ROM_QSTR(MP_QSTR_tx_color),             MP_ROM_PTR(&mp_lcd_bus_tx_color_obj)             },
        { MP_ROM_QSTR(MP_QSTR_rx_param),             MP_ROM_PTR(&mp_lcd_bus_rx_param_obj)             },
        { MP_ROM_QSTR(MP_QSTR_tx_param),             MP_ROM_PTR(&mp_lcd_bus_tx_param_obj)             },
        { MP_ROM_QSTR(MP_QSTR_free_framebuffer),     MP_ROM_PTR(&mp_lcd_bus_free_framebuffer_obj)     },
        { MP_ROM_QSTR(MP_QSTR_allocate_framebuffer), MP_ROM_PTR(&mp_lcd_bus_allocate_framebuffer_obj) },
        { MP_ROM_QSTR(MP_QSTR_init),                 MP_ROM_PTR(&mp_lcd_bus_init_obj)                 },
        { MP_ROM_QSTR(MP_QSTR_deinit),               MP_ROM_PTR(&mp_lcd_bus_deinit_obj)               },
        { MP_ROM_QSTR(MP_QSTR___del__),              MP_ROM_PTR(&mp_lcd_bus_deinit_obj)               },
        { MP_ROM_QSTR(MP_QSTR_set_window_size),      MP_ROM_PTR(&mp_lcd_sdl_set_window_size_obj)      },
        { MP_ROM_QSTR(MP_QSTR_realloc_buffer),       MP_ROM_PTR(&mp_lcd_sdl_realloc_buffer_obj)       },
        { MP_ROM_QSTR(MP_QSTR_register_quit_callback),    MP_ROM_PTR(&mp_lcd_sdl_register_quit_callback_obj)   },
        { MP_ROM_QSTR(MP_QSTR_register_mouse_callback),   MP_ROM_PTR(&mp_lcd_sdl_register_mouse_callback_obj)  },
        { MP_ROM_QSTR(MP_QSTR_register_keypad_callback),  MP_ROM_PTR(&mp_lcd_sdl_register_keypad_callback_obj) },
        { MP_ROM_QSTR(MP_QSTR_register_window_callback),  MP_ROM_PTR(&mp_lcd_sdl_register_window_callback_obj) },
        { MP_ROM_QSTR(MP_QSTR_poll_events),  MP_ROM_PTR(&mp_lcd_sdl_poll_events_obj) },
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


    static MP_DEFINE_CONST_DICT(mp_lcd_sdl_bus_locals_dict, mp_lcd_sdl_bus_locals_dict_table);

    MP_DEFINE_CONST_OBJ_TYPE(
        mp_lcd_sdl_bus_type,
        MP_QSTR_SDLBus,
        MP_TYPE_FLAG_NONE,
        make_new, mp_lcd_sdl_bus_make_new,
        locals_dict, (mp_obj_dict_t *)&mp_lcd_sdl_bus_locals_dict
    );
#endif
