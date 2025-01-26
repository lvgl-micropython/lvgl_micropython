// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"

#include "common/modlcd_bus.h"
#include "common/lcd_common_types.h"
#include "lcd_types.h"
#include "sdl_bus.h"

#include "SDL.h"

static mp_lcd_sdl_bus_obj_t *last_instance = NULL;


mp_lcd_err_t sdl_del(mp_obj_t obj);
mp_lcd_err_t sdl_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,  bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);


int flush_thread(void *self_in);
int process_event(mp_lcd_sdl_bus_obj_t *self, SDL_Event *event);


static bool sdl_init_cb(void *self_in)
{
    mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)self_in;


    self->prev_instance = last_instance;
    last_instance = self;

    self->sw_rot.init.err = LCD_OK;
    return true;
}


static mp_obj_t scheduled_flush_cb(mp_obj_t self_in)
{
    mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)MP_OBJ_TO_PTR(self_in);

    int pitch = self->panel_io_config.width * data->bytes_per_pixel;

    SDL_UpdateTexture(self->texture, NULL, self->sw_rot.buffers.active, pitch);
    SDL_RenderClear(self->renderer);
    SDL_RenderCopy(self->renderer, self->texture, NULL, NULL);
    SDL_RenderPresent(self->renderer);
    mp_lcd_event_set_from_isr(&self->sw_rot.handles.swap_bufs);
}


static void sdl_flush_cb(void *self_in, uint8_t last_update, int cmd, uint8_t *idle_fb)
{
    LCD_UNUSED(cmd);

    if (last_update) {
        mp_lcd_sdl_bus_obj_t *self = (mp_lcd_sdl_bus_obj_t *)self_in;
        mp_lcd_sw_rotation_data_t *data = &self->sw_rot.data;
        mp_lcd_sw_rotation_buffers_t *buffers = &self->sw_rot.buffers;

        buffers->idle = buffers->active;
        buffers->active = idle_fb;

        mp_sched_schedule(mp_obj_t function, MP_OBJ_FROM_PTR(self));

        mp_lcd_event_clear(&self->sw_rot.handles.swap_bufs);
        mp_lcd_event_wait(&self->sw_rot.handles.swap_bufs);

        memcpy(buffers->idle, buffers->active,
               self->width * self->height * data->bytes_per_pixel);
    }
}


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
    self->lanes = 1;

    self->flags = args[ARG_flags].u_int;

    self->panel_io_handle.del = sdl_del;
    self->panel_io_handle.init = sdl_init;

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


mp_lcd_err_t sdl_del(mp_obj_t obj)
{
    mp_lcd_sdl_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

    if (last_instance == self) {
        last_instance = self->prev_instance;
    } else {
        mp_lcd_sdl_bus_obj_t *instance = last_instance;
        while (instance != NULL) {
            if (instance->prev_instance == self) {
                instance->prev_instance = self->prev_instance;
                break;
            }
            instance = instance->prev_instance;
        }
    }

    SDL_DestroyTexture(self->texture);
    SDL_DestroyRenderer(self->renderer);
    SDL_DestroyWindow(self->window);

    m_free(self->sw_rot.buffers.active);
    m_free(self->sw_rot.buffers.idle);

    return LCD_OK;
}


mp_lcd_err_t sdl_init(mp_obj_t obj, uint8_t cmd_bits, uint8_t param_bits)
{
    LCD_UNUSED(cmd_bits);
    LCD_UNUSED(param_bits);

    mp_lcd_sdl_bus_obj_t *self = MP_OBJ_TO_PTR(obj);
    mp_lcd_sw_rotation_data_t *data = &self->sw_rot.data;

    self->sw_rot.data.rgb565_swap = false;

    (uint16_t) width = (uint16_t)data->dst_width;
    (uint16_t) height = (uint16_t)data->dst_height;

    self->panel_io_config.width = width;
    self->panel_io_config.height = height;
    self->sw_rotate = 1;

    self->sw_rot.buffers.active = (uint8_t *)malloc((size_t) (data->dst_width * data->dst_height * data->bytes_per_pixel));
    self->sw_rot.buffers.idle = (uint8_t *)malloc((size_t) (data->dst_width * data->dst_height * data->bytes_per_pixel));

    SDL_StartTextInput();

    self->window = SDL_CreateWindow(
        "LVGL MP\0",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width,
        height,
        self->flags
    );

    self->renderer = SDL_CreateRenderer(self->window, -1, SDL_RENDERER_SOFTWARE);

    self->texture = SDL_CreateTexture(self->renderer,
                (SDL_PixelFormatEnum)data->color_format,
                SDL_TEXTUREACCESS_STREAMING, width, height);

    SDL_SetTextureBlendMode(self->texture, SDL_BLENDMODE_BLEND);
    SDL_SetWindowSize(self->window, width, height);

    self->inited = true;
    return LCD_OK;
}


static mp_obj_t mp_lcd_sdl_poll_events(mp_obj_t self_in)
{
    LCD_UNUSED(self_in);
    //mp_printf(&mp_plat_print, "mp_lcd_sdl_poll_events\n");

    mp_lcd_sdl_bus_obj_t *self = NULL;
    SDL_Event event;

    while (SDL_PollEvent(&event) > 0) {
        mp_lcd_sdl_bus_obj_t *self = last_instance;

        while (self != NULL) {
            if (process_event(self, &event) == 1) break;
            self = self->prev_instance;
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

    mp_lcd_lock_acquire(&self->sw_rot.handles.tx_color_lock);

    uint16_t width = (uint16_t)args[ARG_width].u_int
    uint16_t height = (uint16_t)args[ARG_height].u_int

    self->panel_io_config.width = width;
    self->panel_io_config.height = height;

    self->sw_rot.data.dst_width = (uint32_t)width;
    self->sw_rot.data.dst_height = (uint32_t)height;

    self->sw_rot.data.color_format = (uint32_t)args[ARG_px_format].u_int;

    if(self->texture) {
        SDL_DestroyTexture(self->texture);
    }

    self->texture = SDL_CreateTexture(
        self->renderer,
        (SDL_PixelFormatEnum)self->sw_rot.data.color_format,
        SDL_TEXTUREACCESS_STATIC,
        width,
        height
    );

    SDL_SetTextureBlendMode(self->texture, SDL_BLENDMODE_BLEND);

    if ((bool)args[ARG_ignore_size_chg].u_int == false) {
        SDL_SetWindowSize(self->window, (int)width, (int)height);
    }

    mp_lcd_lock_release(&self->sw_rot.handles.tx_color_lock);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_sdl_set_window_size_obj, 5, mp_lcd_sdl_set_window_size);


static mp_obj_t mp_lcd_sdl_realloc_buffer(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_size, ARG_buf_num };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_size,         MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_buf_num,      MP_ARG_INT | MP_ARG_REQUIRED },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_lcd_sdl_bus_obj_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);

    size_t size = (size_t)args[ARG_size].u_int;

    mp_lcd_lock_acquire(&self->sw_rot.handles.tx_color_lock);

    if (args[ARG_buf_num].u_int == 1) {
        self->sw_rot.buffers.active = (uint8_t *)m_realloc(self->sw_rot.buffers.active, size);
        memset(self->sw_rot.buffers.active, 0x00, size);

        self->sw_rot.buffers.idle = (uint8_t *)m_realloc(self->sw_rot.buffers.idle, size);
        memset(self->sw_rot.buffers.idle, 0x00, size);
        mp_lcd_lock_release(&self->sw_rot.handles.tx_color_lock);
        return MP_OBJ_FROM_PTR(self->fb1);
    } else {
        mp_lcd_lock_release(&self->sw_rot.handles.tx_color_lock);
        return MP_OBJ_FROM_PTR(self->fb2);
    }
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
