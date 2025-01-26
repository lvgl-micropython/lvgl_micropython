// Copyright (c) 2024 - 2025 Kevin G. Schlosser

// local includes
#include "common/modlcd_bus.h"
#include "spi_bus.h"
#include "i2c_bus.h"
#include "i80_bus.h"
#include "rgb_bus.h"

#ifdef MP_PORT_UNIX
    #include "sdl_bus.h"
#endif

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objarray.h"
#include "py/binary.h"


#ifdef ESP_IDF_VERSION
    #include "esp_heap_caps.h"
#endif



static mp_lcd_bus_obj_t **running_busses;
static uint8_t running_bus_count = 0;


void mp_lcd_bus_shutdown(void)
{
    mp_lcd_bus_obj_t *bus;
    uint8_t bus_count = running_bus_count;
    while (bus_count != 0) {
        bus_count--;
        bus = running_busses[bus_count];
        mp_lcd_bus_deinit(MP_OBJ_FROM_PTR(bus));
    }
}


mp_obj_t mp_lcd_bus_get_lane_count(mp_obj_t self_in)
{

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)MP_OBJ_TO_PTR(self_in);
    mp_lcd_err_t ret = lcd_panel_io_get_lane_count(self_in, &lane_count);

    return mp_obj_new_int_from_uint(self->lane_count);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_lcd_bus_get_lane_count_obj, mp_lcd_bus_get_lane_count);


mp_obj_t mp_lcd_bus_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_width, ARG_height, ARG_bpp, ARG_color_format, ARG_cmd_bits,
        ARG_param_bits, ARG_fb1, ,ARG_fb2, ARG_sw_rotate, ARG_rgb565_byte_swap };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,             MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_width,            MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_height,           MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_bpp,              MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_color_format,     MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_cmd_bits,         MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_param_bits,       MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_fb1,              MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_fb2,              MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_sw_rotate,        MP_ARG_BOOL | MP_ARG_REQUIRED },
        { MP_QSTR_rgb565_byte_swap, MP_ARG_BOOL | MP_ARG_REQUIRED },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)args[ARG_self].u_obj;

    self->fb1 = (mp_lcd_framebuf_t *)MP_OBJ_TO_PTR(args[ARG_fb1].u_obj);

    if (args[ARG_fb2].u_obj != mp_const_none) {
        self->fb2 = (mp_lcd_framebuf_t *)MP_OBJ_TO_PTR(args[ARG_fb2].u_obj);
    } else {
        self->fb2 = NULL;
    }

    mp_lcd_error_t ret = mp_lcd_verify_frame_buffers(self->fb1, self->fb2);
    switch (ret) {
        case LCD_ERR_INVALID_ARG:
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Frame buffers don't have matching caps"));
            return mp_const_none;

        case LCD_ERR_INVALID_SIZE:
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Frame buffers are not the same size"));
            return mp_const_none;

        default:
            break;
    }

    uint16_t width = (uint16_t)args[ARG_width].u_int;
    uint16_t height = (uint16_t)args[ARG_height].u_int;
    uint8_t bpp = (uint8_t)args[ARG_bpp].u_int;

    self->sw_rot.data->dst_width = (uint32_t)width;
    self->sw_rot.data->dst_height = (uint32_t)height;
    self->sw_rot.data->bytes_per_pixel = bpp / 8;
    self->sw_rot.data.color_format = (uint32_t)args[ARG_color_format].u_int;

    self->sw_rot.data.rgb565_swap = (bool)args[ARG_rgb565_byte_swap].u_bool;

    self->sw_rotate = (uint8_t)args[ARG_sw_rotate].u_bool;

    mp_lcd_lock_init(&self->sw_rot.handles.copy_lock);
    mp_lcd_lock_init(&self->sw_rot.handles.tx_color_lock);
    mp_lcd_event_init(&self->sw_rot.handles.copy_task_exit);
    mp_lcd_event_init(&self->sw_rot.handles.swap_bufs);
    mp_lcd_event_set(&self->sw_rot.handles.swap_bufs);
    mp_lcd_lock_init(&self->sw_rot.handles.init_lock);
    mp_lcd_lock_acquire(&self->sw_rot.handles.init_lock);
    mp_lcd_lock_init(&self->sw_rot.tx_params.lock);

    ret = self->panel_io_handle.init(
        args[ARG_self].u_obj,
        (uint8_t)args[ARG_cmd_bits].u_int,
        (uint8_t)args[ARG_param_bits].u_int
    );

    if (ret != LCD_OK) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(lcd_panel_io_init)"), ret);
        return mp_const_none;
    }

    if (mp_lcd_start_rotate_task(self)) {
        if ((self->sw_rotate || self->sw_rot.data.rgb565_swap) && self->sw_rot.buffers.active == NULL) {
            ret = mp_lcd_allocate_rotation_buffers(self);
            if (ret == LCD_ERR_NO_MEM) {
                mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Not enough memory to allocate frame buffers"));
                return mp_const_none;
            }
        }
    }

    running_bus_count += 1;
    running_busses = realloc(running_busses, running_bus_count * (sizeof(mp_lcd_bus_obj_t *)));
    running_busses[running_bus_count - 1] = self;

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_bus_init_obj, 10, mp_lcd_bus_init);


mp_obj_t mp_lcd_bus_tx_param(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_cmd, ARG_flush_next, ARG_params };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,       MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_cmd,        MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_params,     MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_flush_next, MP_ARG_BOOL | MP_ARG_REQUIRED },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (
        mp_obj_is_type(args[ARG_self].u_obj, &mp_lcd_rgb_bus_type) ||
        mp_obj_is_type(args[ARG_self].u_obj, &mp_lcd_sdl_bus_type)) {

        mp_printf(&mp_plat_print, "Transmitting parameters is not supported\n");
        return mp_const_none;
    }

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)args[ARG_self].u_obj;

    mp_lcd_sw_rotate_tx_params_t *tx_params = &self->sw_rot.tx_params;

    mp_lcd_lock_acquire(&self->sw_rot.handles.tx_color_lock);
    mp_lcd_lock_acquire(&tx_params->lock);

    tx_params->len++;

    tx_params->params = (mp_lcd_sw_rotate_tx_param_t *)realloc(
        tx_params->params, tx_params->len * sizeof(mp_lcd_sw_rotate_tx_param_t));

    if (args[ARG_params].u_obj != mp_const_none) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[ARG_params].u_obj, &bufinfo, MP_BUFFER_READ);

        tx_params->params[tx_params->len - 1].cmd = (int)args[ARG_cmd].u_int;
        tx_params->params[tx_params->len - 1].params = (uint8_t *)bufinfo.buf;
        tx_params->params[tx_params->len - 1].params_len = (size_t)bufinfo.len;
        tx_params->params[tx_params->len - 1].flush_next = (bool)args[ARG_flush_next].u_bool;
    } else {
        tx_params->params[tx_params->len - 1].cmd = (int)args[ARG_cmd].u_int;
        tx_params->params[tx_params->len - 1].params = NULL;
        tx_params->params[tx_params->len - 1].params_len = 0;
        tx_params->params[tx_params->len - 1].flush_next = (bool)args[ARG_flush_next].u_bool;
    }

    mp_lcd_lock_release(&tx_params->lock);
    mp_lcd_lock_acquire(&self->sw_rot.handles.tx_color_lock);
    self->sw_rot.buffers.partial = NULL;
    mp_lcd_lock_release(&self->sw_rot.handles.copy_lock);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_bus_tx_param_obj, 4, mp_lcd_bus_tx_param);


mp_obj_t mp_lcd_bus_tx_color(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_cmd, ARG_data, ARG_x_start, ARG_y_start, ARG_x_end, ARG_y_end, ARG_rotation, ARG_last_update, ARG_rgb565_dither };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,          MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_cmd,           MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_data,          MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_x_start,       MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_y_start,       MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_x_end,         MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_y_end,         MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_rotation,      MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_last_update,   MP_ARG_BOOL | MP_ARG_REQUIRED },
        { MP_QSTR_rgb565_dither, MP_ARG_BOOL | MP_ARG_REQUIRED },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)args[ARG_self].u_obj;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_data].u_obj, &bufinfo, MP_BUFFER_READ);

    mp_lcd_sw_rotation_data_t *data = &self->sw_rot.data;

    mp_lcd_lock_acquire(&self->sw_rot.handles.tx_color_lock);

    data->cmd = (int)args[ARG_cmd].u_int;
    self->sw_rot.buffers.partial = (uint8_t *)bufinfo.buf;
    data->x_start = (uint32_t)args[ARG_x_start].u_int;
    data->y_start = (uint32_t)args[ARG_y_start].u_int;
    data->x_end = (uint32_t)args[ARG_x_end].u_int;
    data->y_end = (uint32_t)args[ARG_y_end].u_int;
    data->rotation = (uint8_t)args[ARG_rotation].u_int;
    data->last_update = (uint8_t)((bool)args[ARG_last_update].u_bool);
    data->rgb565_dither = (uint8_t)((bool)args[ARG_rgb565_dither].u_bool);

    if (
        !mp_obj_is_type(args[ARG_self].u_obj, &mp_lcd_rgb_bus_type) &&
        !mp_obj_is_type(args[ARG_self].u_obj, &mp_lcd_sdl_bus_type)) {

        data->dst_width = data->x_end - data->x_start;
        data->dst_height = data->y_end - data->y_start;
    }

    mp_lcd_lock_release(&self->sw_rot.handles.copy_lock);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_bus_tx_color_obj, 10, mp_lcd_bus_tx_color);


mp_obj_t mp_lcd_bus_deinit(mp_obj_t obj)
{
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)args[ARG_self].u_obj;

    mp_lcd_sw_rotation_handles_t *handles = &self->sw_rot.handles;

    mp_lcd_lock_acquire(&handles->tx_color_lock);
    self->sw_rot.buffers.partial = NULL;

    mp_lcd_event_set(&handles->copy_task_exit);
    mp_lcd_lock_release(&handles->copy_lock);
    mp_lcd_lock_release(&handles->tx_color_lock);

    mp_lcd_err_t ret = esp_lcd_panel_del(self->panel_handle);

    mp_lcd_lock_delete(&handles->copy_lock);
    mp_lcd_lock_delete(&handles->tx_color_lock);

    mp_lcd_event_clear(&handles->swap_bufs);
    mp_lcd_event_delete(&handles->swap_bufs);
    mp_lcd_event_delete(&handles->copy_task_exit);

    if (!mp_obj_is_type(self_in, &mp_lcd_rgb_bus_type)) {
        mp_lcd_lock_delete(&self->sw_rot.tx_params.lock);
    }

    mp_lcd_err_t ret = self->panel_io_handle.deinit(obj);

    if (ret != LCD_OK) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(lcd_panel_io_del)"), ret);
        return mp_const_none;
    }

    uint8_t i = 0;
    for (;i<running_bus_count;i++) if (running_busses[i] == self) break;

    running_bus_count--;

    memmove(running_busses + i, running_busses + (i + 1),
           (running_bus_count - i) * sizeof(mp_lcd_bus_obj_t *));

    running_busses = (mp_lcd_bus_obj_t *)realloc(running_busses,
                                    running_bus_count * sizeof(mp_lcd_bus_obj_t *));
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_lcd_bus_deinit_obj, mp_lcd_bus_deinit);


mp_obj_t mp_lcd_bus_rx_param(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    /*
    Currently Unsupported

    enum { ARG_self, ARG_cmd, ARG_data };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,  MP_ARG_OBJ | MP_ARG_REQUIRED, { .u_obj = mp_const_none } },
        { MP_QSTR_cmd,   MP_ARG_INT | MP_ARG_REQUIRED, { .u_int = -1            } },
        { MP_QSTR_data,  MP_ARG_OBJ | MP_ARG_REQUIRED, { .u_obj = mp_const_none } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_data].u_obj, &bufinfo, MP_BUFFER_WRITE);

    mp_lcd_err_t ret = lcd_panel_io_rx_param(args[ARG_self].u_obj, (int)args[ARG_cmd].u_int, bufinfo.buf, (size_t)bufinfo.len);

    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(lcd_panel_io_rx_param)"), ret);
    }
    */
    LCD_UNUSED(n_args);
    LCD_UNUSED(pos_args);
    LCD_UNUSED(kw_args);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_bus_rx_param_obj, 3, mp_lcd_bus_rx_param);


mp_obj_t mp_lcd_bus_register_callback(mp_obj_t self_in, mp_obj_t callback)
{
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)MP_OBJ_TO_PTR(self_in);
    self->callback = callback;

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_lcd_bus_register_callback_obj, mp_lcd_bus_register_callback);


static mp_obj_t mp_lcd_bus__pump_main_thread(void)
{
    mp_handle_pending(true);
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_0(mp_lcd_bus__pump_main_thread_obj, mp_lcd_bus__pump_main_thread);



static const mp_rom_map_elem_t mp_lcd_bus_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_lane_count),       MP_ROM_PTR(&mp_lcd_bus_get_lane_count_obj)       },
    { MP_ROM_QSTR(MP_QSTR_allocate_framebuffer), MP_ROM_PTR(&mp_lcd_bus_allocate_framebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_free_framebuffer),     MP_ROM_PTR(&mp_lcd_bus_free_framebuffer_obj)     },
    { MP_ROM_QSTR(MP_QSTR_register_callback),    MP_ROM_PTR(&mp_lcd_bus_register_callback_obj)    },
    { MP_ROM_QSTR(MP_QSTR_tx_param),             MP_ROM_PTR(&mp_lcd_bus_tx_param_obj)             },
    { MP_ROM_QSTR(MP_QSTR_tx_color),             MP_ROM_PTR(&mp_lcd_bus_tx_color_obj)             },
    { MP_ROM_QSTR(MP_QSTR_rx_param),             MP_ROM_PTR(&mp_lcd_bus_rx_param_obj)             },
    { MP_ROM_QSTR(MP_QSTR_init),                 MP_ROM_PTR(&mp_lcd_bus_init_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_deinit),               MP_ROM_PTR(&mp_lcd_bus_deinit_obj)               },
    { MP_ROM_QSTR(MP_QSTR___del__),              MP_ROM_PTR(&mp_lcd_bus_deinit_obj)               }
};

MP_DEFINE_CONST_DICT(mp_lcd_bus_locals_dict, mp_lcd_bus_locals_dict_table);


static const mp_rom_map_elem_t mp_module_lcd_bus_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),           MP_OBJ_NEW_QSTR(MP_QSTR_lcd_bus)        },
    { MP_ROM_QSTR(MP_QSTR_RGBBus),             MP_ROM_PTR(&mp_lcd_rgb_bus_type)        },
    { MP_ROM_QSTR(MP_QSTR_SPIBus),             MP_ROM_PTR(&mp_lcd_spi_bus_type)        },
    { MP_ROM_QSTR(MP_QSTR_I2CBus),             MP_ROM_PTR(&mp_lcd_i2c_bus_type)        },
    { MP_ROM_QSTR(MP_QSTR_I80Bus),             MP_ROM_PTR(&mp_lcd_i80_bus_type)        },
    { MP_ROM_QSTR(MP_QSTR__pump_main_thread),  MP_ROM_PTR(&mp_lcd_bus__pump_main_thread_obj)       },

    #ifdef MP_PORT_UNIX
        { MP_ROM_QSTR(MP_QSTR_SDLBus),         MP_ROM_PTR(&mp_lcd_sdl_bus_type)        },
    #endif

    #ifdef ESP_IDF_VERSION
        { MP_ROM_QSTR(MP_QSTR_MEMORY_32BIT),    MP_ROM_INT(MALLOC_CAP_32BIT)     },
        { MP_ROM_QSTR(MP_QSTR_MEMORY_8BIT),     MP_ROM_INT(MALLOC_CAP_8BIT)      },
        { MP_ROM_QSTR(MP_QSTR_MEMORY_DMA),      MP_ROM_INT(MALLOC_CAP_DMA)       },
        { MP_ROM_QSTR(MP_QSTR_MEMORY_SPIRAM),   MP_ROM_INT(MALLOC_CAP_SPIRAM)    },
        { MP_ROM_QSTR(MP_QSTR_MEMORY_INTERNAL), MP_ROM_INT(MALLOC_CAP_INTERNAL)  },
        { MP_ROM_QSTR(MP_QSTR_MEMORY_DEFAULT),  MP_ROM_INT(MALLOC_CAP_DEFAULT)   },
    #else
        { MP_ROM_QSTR(MP_QSTR_MEMORY_32BIT),    MP_ROM_INT(0) },
        { MP_ROM_QSTR(MP_QSTR_MEMORY_8BIT),     MP_ROM_INT(0) },
        { MP_ROM_QSTR(MP_QSTR_MEMORY_DMA),      MP_ROM_INT(0) },
        { MP_ROM_QSTR(MP_QSTR_MEMORY_SPIRAM),   MP_ROM_INT(0) },
        { MP_ROM_QSTR(MP_QSTR_MEMORY_INTERNAL), MP_ROM_INT(0) },
        { MP_ROM_QSTR(MP_QSTR_MEMORY_DEFAULT),  MP_ROM_INT(0) },
    #endif /* ESP_IDF_VERSION */
};

static MP_DEFINE_CONST_DICT(mp_module_lcd_bus_globals, mp_module_lcd_bus_globals_table);


const mp_obj_module_t mp_module_lcd_bus = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_lcd_bus_globals,
};

MP_REGISTER_MODULE(MP_QSTR_lcd_bus, mp_module_lcd_bus);
