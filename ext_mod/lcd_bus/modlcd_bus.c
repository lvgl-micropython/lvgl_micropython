// Copyright (c) 2024 - 2025 Kevin G. Schlosser

// local includes
#include "modlcd_bus.h"
#include "lcd_types.h"
#include "allocate_buffers.h"
#include "spi_bus.h"
#include "i2c_bus.h"
#include "i80_bus.h"
#include "rgb_bus.h"
#include "led_bus.h"
#include "dsi_bus.h"

#ifdef MP_PORT_UNIX
    #include "sdl_bus.h"
#endif

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objarray.h"
#include "py/binary.h"


#if CONFIG_LCD_ENABLE_DEBUG_LOG
#define LCD_DEBUG  1
#else
#define LCD_DEBUG  0
#endif


mp_lcd_bus_obj_t **lcd_bus_objs;
uint8_t lcd_bus_count = 0;


static mp_obj_t mp_lcd_bus_get_lane_count(mp_obj_t obj)
{
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)MP_OBJ_TO_PTR(obj);
    return mp_obj_new_int(self->num_lanes);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_lcd_bus_get_lane_count_obj, mp_lcd_bus_get_lane_count);


static mp_obj_t mp_lcd_bus_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_width, ARG_height, ARG_bpp, ARG_buffer_size, ARG_rgb565_byte_swap, ARG_sw_rotation, ARG_cmd_bits, ARG_param_bits };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,             MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_width,            MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_height,           MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_bpp,              MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_buffer_size,      MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_rgb565_byte_swap, MP_ARG_BOOL | MP_ARG_REQUIRED },
        { MP_QSTR_sw_rotation,      MP_ARG_BOOL | MP_ARG_REQUIRED },
        { MP_QSTR_cmd_bits,         MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_param_bits,       MP_ARG_INT  | MP_ARG_REQUIRED },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)MP_OBJ_TO_PTR(args[ARG_self].u_obj);

    lcd_bus_lock_init(&self->init.lock);
    lcd_bus_lock_acquire(self->init.lock);

    lcd_bus_lock_init(&self->task.lock);
    lcd_bus_event_init(&self->task.exit);

    lcd_bus_lock_init(&self->tx_data.lock);
    lcd_bus_event_init(&self->tx_data.swap_bufs);
    lcd_bus_event_set(self->tx_data.swap_bufs);

    uint8_t bpp = (uint8_t)args[ARG_bpp].u_int;
    uint8_t swap = (uint8_t)args[ARG_rgb565_byte_swap].u_bool;
    uint8_t sw_rotation = (uint8_t)args[ARG_sw_rotation].u_bool;

    self->r_data.bytes_per_pixel = bpp / 8;
    self->r_data.sw_rotation = sw_rotation;

    if (bpp == 16) {
        self->r_data.swap = swap;
    } else {
        self->r_data.swap = 0;
    }


    mp_lcd_err_t ret = self->internal_cb_funcs.init(
        self,
        (uint16_t)args[ARG_width].u_int,
        (uint16_t)args[ARG_height].u_int,
        (uint8_t)args[ARG_bpp].u_int,
        (uint32_t)args[ARG_buffer_size].u_int,
        (uint8_t)args[ARG_cmd_bits].u_int,
        (uint8_t)args[ARG_param_bits].u_int
    );

    if (ret == 0) {
        // add the new bus ONLY after successfull initilization of the bus
        lcd_bus_count++;
        lcd_bus_objs = m_realloc(lcd_bus_objs, lcd_bus_count * sizeof(mp_lcd_bus_obj_t *));
        lcd_bus_objs[lcd_bus_count - 1] = self;
    } else {
        if (ret == LCD_ERR_NO_MEM) {
            mp_raise_msg_varg(&mp_type_MemoryError, MP_ERROR_TEXT("%d(mp_lcd_bus_init)"), ret);
        } else if (ret == LCD_ERR_INVALID_ARG) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(mp_lcd_bus_init)"), ret);
        } else if (ret == LCD_ERR_NOT_SUPPORTED) {
            mp_raise_msg_varg(&mp_type_NotImplementedError, MP_ERROR_TEXT("%d(mp_lcd_bus_init)"), ret);
        } else {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(mp_lcd_bus_init)"), ret);
        }
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_bus_init_obj, 8, mp_lcd_bus_init);


static mp_obj_t mp_lcd_bus_free_framebuffer(mp_obj_t self_in, mp_obj_t buf_in)
{
    if (buf_in != mp_const_none) {
        mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)MP_OBJ_TO_PTR(self_in);
        mp_obj_array_t *array_buf = (mp_obj_array_t *)MP_OBJ_TO_PTR(buf_in);

        if (array_buf == self->view1) {
            free_framebuffer(array_buf->items);
            self->view1 = NULL;
            LCD_DEBUG_PRINT("lcd_panel_io_free_framebuffer(self, buf=1)\n")
        } else if (array_buf == self->view2) {
            free_framebuffer(array_buf->items);
            self->view2 = NULL;
            LCD_DEBUG_PRINT("lcd_panel_io_free_framebuffer(self, buf=2)\n")
        } else {
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("No matching buffer found"));
        }
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_lcd_bus_free_framebuffer_obj, mp_lcd_bus_free_framebuffer);


static mp_obj_t mp_lcd_bus_allocate_framebuffer(mp_obj_t self_in, mp_obj_t size_in, mp_obj_t caps_in)
{
     mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)MP_OBJ_TO_PTR(self_in);

     size_t size = (size_t)mp_obj_get_int_truncated(size_in);
     uint32_t caps = (uint32_t)mp_obj_get_int_truncated(caps_in);

     void *buf = NULL;

     if (!allocate_framebuffer(buf, size, caps)) {
        mp_raise_msg_varg(&mp_type_MemoryError, MP_ERROR_TEXT("Not enough memory available (%d)"), size);
        return mp_const_none;
     }

     mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, buf));
     view->typecode |= 0x80; // used to indicate writable buffer

     if (self->view1 != NULL && self->view2 != NULL) {
         free_framebuffer(buf);
         mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("There is a maximum of 2 frame buffers allowed"));
         return mp_const_none;
     }

     if (self->view1 != NULL && self->buffer_flags != caps) {
         free_framebuffer(buf);
         mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("allocation flags must be the same for both buffers"));
         return mp_const_none;
     }

     self->buffer_flags = caps;

     if (self->view1 == NULL) self->view1 = view;
     else self->view2 = view;

     return MP_OBJ_FROM_PTR(view);
}

MP_DEFINE_CONST_FUN_OBJ_3(mp_lcd_bus_allocate_framebuffer_obj, mp_lcd_bus_allocate_framebuffer);


static mp_obj_t mp_lcd_bus_tx_param(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_cmd, ARG_params, ARG_flush_is_next };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,          MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_cmd,           MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_params,        MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_flush_is_next, MP_ARG_BOOL | MP_ARG_REQUIRED }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)args[ARG_self].u_obj;
    int cmd = (int)args[ARG_cmd].u_int;
    uint8_t flush_is_next = (uint8_t)args[ARG_flush_is_next].u_bool;

    uint8_t *params;
    size_t params_len;

    if (args[ARG_params].u_obj == mp_const_none) {
        params = NULL;
        params_len = 0;
    } else {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[ARG_params].u_obj, &bufinfo, MP_BUFFER_READ);
        params = (uint8_t *)bufinfo.buf;
        params_len = (size_t)bufinfo.len;
    }

    lcd_cmd_t *command = malloc(sizeof(lcd_cmd_t));

    command->cmd = cmd;
    command->params = NULL;
    command->params_len = params_len;
    command->flush_is_next = flush_is_next;

    if (params != NULL) {
        command->params = malloc(sizeof(uint8_t) * params_len);
        memcpy(command->params, params, params_len);
    }

    lcd_bus_lock_acquire(self->tx_cmds.lock);

    self->tx_cmds.cmds_len++;
    self->tx_cmds.cmds = realloc(self->tx_cmds.cmds, self->tx_cmds.cmds_len * sizeof(lcd_cmd_t *));
    self->tx_cmds.cmds[self->tx_cmds.cmds_len - 1] = command;
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_bus_tx_param_obj, 4, mp_lcd_bus_tx_param);


static mp_obj_t mp_lcd_bus_tx_color(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_cmd, ARG_data, ARG_x_start, ARG_y_start, ARG_x_end, ARG_y_end, ARG_dither, ARG_rotation, ARG_last_update };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,        MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_cmd,         MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_data,        MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_x_start,     MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_y_start,     MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_x_end,       MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_y_end,       MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_dither,      MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_rotation,    MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_last_update, MP_ARG_BOOL | MP_ARG_REQUIRED },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)args[ARG_self].u_obj;

    int cmd = (int)args[ARG_cmd].u_int;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_data].u_obj, &bufinfo, MP_BUFFER_RW);
    uint8_t * color = (uint8_t *)bufinfo.buf;
    size_t color_size = (size_t)bufinfo.len;

    int32_t x_start = (int32_t)args[ARG_x_start].u_int;
    int32_t y_start = (int32_t)args[ARG_y_start].u_int;
    int32_t x_end = (int32_t)args[ARG_x_end].u_int;
    int32_t y_end = (int32_t)args[ARG_y_end].u_int;
    uint8_t dither = (uint8_t)args[ARG_dither].u_int;
    uint8_t rotation = (uint8_t)args[ARG_rotation].u_int;
    uint8_t last_update = (uint8_t)args[ARG_last_update].u_bool;

    lcd_bus_lock_acquire(self->tx_data.lock);

    if (self->r_data.sw_rotation == 1) {
        self->bufs.partial = (uint8_t *)color;
    } else if (dither && self->bufs.partial == NULL) {
        int ret = 0;

        if (self->view2 != NULL) {
            ret = allocate_buffers(2, self->view1->len, self->bufs.idle, self->bufs.active);
        } else {
            ret = allocate_buffers(1, self->view1->len, self->bufs.idle, NULL);
        }

        if (ret == -1) {
            dither = false;
            self->bufs.idle = (uint8_t *)color;
        }
    } else if (!dither && !self->r_data.sw_rotation && self->bufs.partial != NULL) {
        free_buffers(self->bufs.idle, self->bufs.active);
        self->bufs.partial = NULL;
        self->bufs.idle = (uint8_t *)color;
    }

    self->r_data.last_update = (uint8_t)last_update;
    self->r_data.color_size = color_size;
    self->r_data.x_start = x_start;
    self->r_data.y_start = y_start;
    self->r_data.x_end = x_end;
    self->r_data.y_end = y_end;
    self->r_data.rotation = rotation;
    self->r_data.dither = dither;
    self->r_data.cmd = cmd;

    if (self->base.type != &mp_lcd_rgb_bus_type &&
        self->base.type != &mp_lcd_dsi_bus_type) {

        self->r_data.dst_width = x_end - x_start + 1;
        self->r_data.dst_height = y_end - y_start + 1;
    }

    lcd_bus_lock_release(self->task.lock);

    if (self->callback == mp_const_none) {
        while (self->trans_done == false) {}
        self->trans_done = false;
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_bus_tx_color_obj, 10, mp_lcd_bus_tx_color);


static mp_obj_t mp_lcd_bus_deinit(mp_obj_t self_in)
{

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)MP_OBJ_TO_PTR(self_in);

    lcd_bus_lock_acquire(self->tx_data.lock);
    self->bufs.partial = NULL;
    lcd_bus_event_set(self->task.exit);
    lcd_bus_lock_release(self->task.lock);
    lcd_bus_lock_release(self->tx_data.lock);

    mp_lcd_err_t ret = self->internal_cb_funcs.deinit(self);

    lcd_bus_lock_delete(self->task.lock);
    lcd_bus_lock_delete(self->tx_data.lock);

    lcd_bus_event_clear(self->tx_data.swap_bufs);
    lcd_bus_event_delete(&self->tx_data.swap_bufs);
    lcd_bus_event_delete(&self->task.exit);

    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(lcd_panel_io_del)"), ret);
    }

    if (self->view1 != NULL) {
        free_framebuffer(self->view1->items);
        self->view1->items = NULL;
        self->view1->len = 0;
        self->view1 = NULL;
        LCD_DEBUG_PRINT("spi_free_framebuffer(self, buf=1)\n")
    }

    if (self->view2 != NULL) {
        free_framebuffer(self->view2->items);
        self->view2->items = NULL;
        self->view2->len = 0;
        self->view2 = NULL;
        LCD_DEBUG_PRINT("spi_free_framebuffer(self, buf=1)\n")
    }

    uint8_t i = 0;
    for (;i<lcd_bus_count;i++) {
        if (lcd_bus_objs[i] == self) {
            lcd_bus_objs[i] = NULL;
            break;
        }
    }

    for (uint8_t j=i + 1;j<lcd_bus_count;j++) {
        lcd_bus_objs[j - 1] = lcd_bus_objs[j];
    }

    lcd_bus_count--;
    lcd_bus_objs = m_realloc(lcd_bus_objs, lcd_bus_count * sizeof(mp_lcd_bus_obj_t *));

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_lcd_bus_deinit_obj, mp_lcd_bus_deinit);


static mp_obj_t mp_lcd_bus_register_callback(mp_obj_t self_in, mp_obj_t callback)
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

MP_DEFINE_CONST_FUN_OBJ_0(mp_lcd_bus__pump_main_thread_obj, mp_lcd_bus__pump_main_thread);


void mp_lcd_bus_deinit_all()
{
    mp_lcd_bus_obj_t *self;
    mp_obj_t obj;

    while (lcd_bus_count != 0) {
        self = lcd_bus_objs[0];
        obj = (mp_obj_t)MP_OBJ_FROM_PTR(self);
        mp_lcd_bus_deinit(obj);
    }
}


static const mp_rom_map_elem_t mp_lcd_bus_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_lane_count),       MP_ROM_PTR(&mp_lcd_bus_get_lane_count_obj)       },
    { MP_ROM_QSTR(MP_QSTR_allocate_framebuffer), MP_ROM_PTR(&mp_lcd_bus_allocate_framebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_free_framebuffer),     MP_ROM_PTR(&mp_lcd_bus_free_framebuffer_obj)     },
    { MP_ROM_QSTR(MP_QSTR_register_callback),    MP_ROM_PTR(&mp_lcd_bus_register_callback_obj)    },
    { MP_ROM_QSTR(MP_QSTR_tx_param),             MP_ROM_PTR(&mp_lcd_bus_tx_param_obj)             },
    { MP_ROM_QSTR(MP_QSTR_tx_color),             MP_ROM_PTR(&mp_lcd_bus_tx_color_obj)             },
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
    { MP_ROM_QSTR(MP_QSTR_DSIBus),             MP_ROM_PTR(&mp_lcd_dsi_bus_type)        },
    { MP_ROM_QSTR(MP_QSTR_LEDBus),             MP_ROM_PTR(&mp_lcd_led_bus_type)        },
    { MP_ROM_QSTR(MP_QSTR__pump_main_thread),  MP_ROM_PTR(&mp_lcd_bus__pump_main_thread_obj)       },

    #ifdef MP_PORT_UNIX
        { MP_ROM_QSTR(MP_QSTR_SDLBus),         MP_ROM_PTR(&mp_lcd_sdl_bus_type)        },
    #endif
    { MP_ROM_QSTR(MP_QSTR_DEBUG_ENABLED),    MP_ROM_INT(LCD_DEBUG) },

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

MP_DEFINE_CONST_DICT(mp_module_lcd_bus_globals, mp_module_lcd_bus_globals_table);


const mp_obj_module_t mp_module_lcd_bus = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_lcd_bus_globals,
};

MP_REGISTER_MODULE(MP_QSTR_lcd_bus, mp_module_lcd_bus);
