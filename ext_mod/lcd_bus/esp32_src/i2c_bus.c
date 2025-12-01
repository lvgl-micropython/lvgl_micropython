// Copyright (c) 2024 - 2025 Kevin G. Schlosser

// local includes
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "i2c_bus.h"

// esp-idf includes
#include "esp_lcd_panel_io.h"
#include "driver/i2c.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>


mp_lcd_err_t i2c_del(mp_obj_t obj);
mp_lcd_err_t i2c_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
mp_lcd_err_t i2c_get_lane_count(mp_obj_t obj, uint8_t *lane_count);


typedef struct _i2c_obj_t {
    mp_obj_base_t base;
    i2c_port_t port : 8;
    gpio_num_t scl : 8;
    gpio_num_t sda : 8;
} _i2c_obj_t;


void mp_lcd_i2c_bus_deinit_all(void)
{
    // we need to copy the existing array to a new one so the order doesn't
    // get all mucked up when objects get removed.
}


static mp_obj_t mp_lcd_i2c_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum {
        ARG_i2c_bus,
        ARG_addr,
        ARG_control_phase_bytes,
        ARG_dc_bit_offset,
        ARG_dc_low_on_data,
        ARG_disable_control_phase
    };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_i2c_bus,               MP_ARG_OBJ  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
        { MP_QSTR_addr,                  MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
        { MP_QSTR_control_phase_bytes,   MP_ARG_INT  | MP_ARG_KW_ONLY,  {.u_int = 1         } },
        { MP_QSTR_dc_bit_offset,         MP_ARG_INT  | MP_ARG_KW_ONLY,  {.u_int = 6         } },
        { MP_QSTR_dc_low_on_data,        MP_ARG_BOOL | MP_ARG_KW_ONLY,  {.u_bool = true     } },
        { MP_QSTR_disable_control_phase, MP_ARG_BOOL | MP_ARG_KW_ONLY,  {.u_bool = false    } }
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
    mp_lcd_i2c_bus_obj_t *self = m_new_obj(mp_lcd_i2c_bus_obj_t);
    self->base.type = &mp_lcd_i2c_bus_type;

    self->callback = mp_const_none;

    _i2c_obj_t *bus = MP_OBJ_TO_PTR(args[ARG_i2c_bus].u_obj);

    self->port = bus->port;
    self->panel_io_config.dev_addr = (uint32_t)args[ARG_addr].u_int;
    self->panel_io_config.on_color_trans_done = bus_trans_done_cb;
    self->panel_io_config.user_ctx = self;
    self->panel_io_config.control_phase_bytes = (size_t)args[ARG_control_phase_bytes].u_int;
    self->panel_io_config.dc_bit_offset = (unsigned int)args[ARG_dc_bit_offset].u_int;
    self->panel_io_config.flags.dc_low_on_data = (unsigned int)args[ARG_dc_low_on_data].u_bool;
    self->panel_io_config.flags.disable_control_phase = (unsigned int)args[ARG_disable_control_phase].u_bool;

    self->panel_io_handle.del = &i2c_del;
    self->panel_io_handle.init = &i2c_init;
    self->panel_io_handle.get_lane_count = &i2c_get_lane_count;

    return MP_OBJ_FROM_PTR(self);
}


mp_lcd_err_t i2c_del(mp_obj_t obj)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)obj;

    if (self->panel_io_handle.panel_io != NULL) {

        mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle.panel_io);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
            return ret;
        }

        self->panel_io_handle.panel_io = NULL;

        if (self->view1 != NULL) {
            heap_caps_free(self->view1->items);
            self->view1->items = NULL;
            self->view1->len = 0;
            self->view1 = NULL;
            LCD_DEBUG_PRINT("i2c_free_framebuffer(self, buf=1)\n")
        }

        if (self->view2 != NULL) {
            heap_caps_free(self->view2->items);
            self->view2->items = NULL;
            self->view2->len = 0;
            self->view2 = NULL;
            LCD_DEBUG_PRINT("i2c_free_framebuffer(self, buf=1)\n")
        }
        
        return ret;
    } else {
        return LCD_FAIL;
    }
}


mp_lcd_err_t i2c_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)obj;

    if (bpp == 16) {
        self->rgb565_byte_swap = rgb565_byte_swap;
    } else {
        self->rgb565_byte_swap = false;
    }

    self->panel_io_config.lcd_cmd_bits = (int)cmd_bits;
    self->panel_io_config.lcd_param_bits = (int)param_bits;

    mp_lcd_err_t ret = esp_lcd_new_panel_io_i2c(self->port , &self->panel_io_config, &self->panel_io_handle.panel_io);

    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_i2c)"), ret);
        return ret;
    }
    
    return ret;
}


mp_lcd_err_t i2c_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
{
    LCD_UNUSED(obj);
    *lane_count = 1;
    return LCD_OK;
}


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_i2c_bus_type,
    MP_QSTR_I2CBus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_i2c_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
);

