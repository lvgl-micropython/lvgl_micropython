// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"
#include "py/runtime.h"

#include "esp_lcd_panel_io.h"
#include "driver/i2c.h"


#include "common/lcd_commmon_types.h"
#include "common/modlcd_bus.h"
#include "lcd_types.h"
#include "i2c_bus.h"

mp_lcd_err_t i2c_del(mp_obj_t obj);
mp_lcd_err_t i2c_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);


static bool i2c_trans_done_cb(esp_lcd_panel_handle_t panel,
                        const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)user_ctx;
    
    if (self->trans_done == 0) {
        if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
            mp_lcd_flush_ready_cb(self->callback);
        }
        self->trans_done = 1;
    }
    
    return false;
}


static void i2c_tx_param_cb(void* self_in, int cmd, uint8_t *params, size_t params_len)
{
    esp_lcd_panel_io_tx_param(self->panel_io_handle.panel_io, cmd, params, params_len);
}
    
    
static bool i2c_init_cb(void *self_in)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)self_in;
    mp_lcd_sw_rotation_init_t *init = &self->sw_rot.init;
    
    init->err = i2c_param_config(self->host, self->bus_config);
    if (init->err != LCD_OK) {
        init->err_msg = MP_ERROR_TEXT("%d(i2c_param_config)");
        return false;
    }

    init->err = i2c_driver_install(self->host, I2C_MODE_MASTER, 0, 0, 0);
    
    if (init->err != LCD_OK) {
        init->err_msg = MP_ERROR_TEXT("%d(i2c_driver_install)");
        return false;
    }

    init->err = esp_lcd_new_panel_io_i2c(self->bus_handle , self->panel_io_config, &self->panel_io_handle.panel_io);

    if (init->err != LCD_OK) {
        init->err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_i2c)");
        return false;
    }
    
    free(self->panel_io_config);
    free(self->bus_config);
    self->panel_io_config = NULL;
    self->bus_config = NULL;

    return true;
}
    
    
static void i2c_flush_cb(void *self_in, uint8_t last_update, int cmd, uint8_t *idle_fb)
{
    LCD_UNUSED(last_update);
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)self_in;
    mp_lcd_sw_rotation_buffers_t *buffers = &self->sw_rot.buffers;

    
    if (idle_fb == buffers->idle) {
        buffers->idle = buffers->active;
        buffers->active = idle_fb;
    }
    
    mp_lcd_err_t ret = esp_lcd_panel_io_tx_color(self->panel_io_handle.panel_io, cmd, idle_fb, self->fb1->len);
    
    if (ret != LCD_OK) {
        mp_printf(&mp_plat_print, "esp_lcd_panel_draw_bitmap error (%d)\n", ret);
    }
}


static mp_obj_t mp_lcd_i2c_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum {
        ARG_sda,
        ARG_scl,
        ARG_addr,
        ARG_host,
        ARG_control_phase_bytes,
        ARG_dc_bit_offset,
        ARG_freq,
        ARG_dc_low_on_data,
        ARG_sda_pullup,
        ARG_scl_pullup,
        ARG_disable_control_phase
    };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_sda,                   MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
        { MP_QSTR_scl,                   MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
        { MP_QSTR_addr,                  MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
        { MP_QSTR_host,                  MP_ARG_INT  | MP_ARG_KW_ONLY,  {.u_int = 0         } },
        { MP_QSTR_control_phase_bytes,   MP_ARG_INT  | MP_ARG_KW_ONLY,  {.u_int = 1         } },
        { MP_QSTR_dc_bit_offset,         MP_ARG_INT  | MP_ARG_KW_ONLY,  {.u_int = 6         } },
        { MP_QSTR_freq,                  MP_ARG_INT  | MP_ARG_KW_ONLY,  {.u_int = 10000000  } },
        { MP_QSTR_dc_low_on_data,        MP_ARG_BOOL | MP_ARG_KW_ONLY,  {.u_bool = false    } },
        { MP_QSTR_sda_pullup,            MP_ARG_BOOL | MP_ARG_KW_ONLY,  {.u_bool = true     } },
        { MP_QSTR_scl_pullup,            MP_ARG_BOOL | MP_ARG_KW_ONLY,  {.u_bool = true     } },
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

    self->panel_io_config = malloc(sizeof(esp_lcd_panel_io_i2c_config_t));
    esp_lcd_panel_io_i2c_config_t *panel_io_config = self->panel_io_config;

    self->bus_config = malloc(sizeof(i2c_config_t));
    i2c_config_t *bus_config = self->bus_config


    self->host = args[ARG_host].u_int;
    self->bus_handle = (esp_lcd_i2c_bus_handle_t)((uint32_t)self->host);

    bus_config->mode = I2C_MODE_MASTER;
    bus_config->sda_io_num = (int)args[ARG_sda].u_int;
    bus_config->scl_io_num = (int)args[ARG_scl].u_int;
    bus_config->sda_pullup_en = (bool)args[ARG_sda_pullup].u_bool;
    bus_config->scl_pullup_en = (bool)args[ARG_scl_pullup].u_bool;
    bus_config->master.clk_speed = (uint32_t)args[ARG_freq].u_int;
    bus_config->clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;

    panel_io_config->dev_addr = (uint32_t)args[ARG_addr].u_int;
    panel_io_config->on_color_trans_done = bus_trans_done_cb;
    panel_io_config->user_ctx = self;
    panel_io_config->control_phase_bytes = (size_t)args[ARG_control_phase_bytes].u_int;
    panel_io_config->dc_bit_offset = (unsigned int)args[ARG_dc_bit_offset].u_int;
    panel_io_config->flags.dc_low_on_data = (unsigned int)args[ARG_dc_low_on_data].u_bool;
    panel_io_config->flags.disable_control_phase = (unsigned int)args[ARG_disable_control_phase].u_bool;

    self->lanes = 1;

    self->panel_io_handle.del = &i2c_del;
    self->panel_io_handle.init = &i2c_init;

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

        ret = i2c_driver_delete(self->host);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(i2c_driver_delete)"), ret);
            return ret;
        }

        self->panel_io_handle.panel_io = NULL;

        return ret;
    } else {
        return LCD_FAIL;
    }
}


mp_lcd_err_t i2c_init(mp_obj_t obj, uint8_t cmd_bits, uint8_t param_bits)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)obj;

    if (self->sw_rot.data.bytes_per_pixel == 2) {
        self->rgb565_byte_swap = false;
   
    self->panel_io_config->lcd_cmd_bits = (int)cmd_bits;
    self->panel_io_config->lcd_param_bits = (int)param_bits;
    
    self->sw_rot.data.dst_width = 0;
    self->sw_rot.data.dst_height = 0;
    self->sw_rot.init.cb = &i2c_init_cb;
    self->sw_rot.flush_cb = &i2c_flush_cb;
    self->sw_rot.tx_params.cb = &i2c_tx_param_cb;

    return LCD_OK;
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

