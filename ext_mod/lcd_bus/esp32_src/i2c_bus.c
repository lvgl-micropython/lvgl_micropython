// Copyright (c) 2024 - 2025 Kevin G. Schlosser

// local includes
#include "lcd_types.h"
#include "lcd_bus_task.h"
#include "modlcd_bus.h"
#include "i2c_bus.h"
#include "bus_trans_done.h"

// esp-idf includes
#include "esp_lcd_panel_io.h"
#include "driver/i2c.h"
#include "esp_task.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>



mp_lcd_err_t i2c_del(mp_lcd_bus_obj_t *self_in);
mp_lcd_err_t i2c_init(mp_lcd_bus_obj_t *self_in, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,
                      uint8_t cmd_bits, uint8_t param_bits);


static void i2c_send_func(mp_lcd_bus_obj_t *self_in, int cmd, uint8_t *params, size_t params_len)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)self_in;
    esp_lcd_panel_io_tx_param(self->panel_io_handle, cmd, params, params_len);

}


static void i2c_flush_func(mp_lcd_bus_obj_t *self_in, rotation_data_t *r_data, rotation_data_t *original_r_data,
                           uint8_t *idle_fb, uint8_t last_update)
{

    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)self_in;
    if (self->bufs.partial != NULL) {
        if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
            cb_isr(self->callback);
        }
        self->trans_done = 1;
    }

    if (last_update) {
        esp_err_t ret = esp_lcd_panel_io_tx_color(self->panel_io_handle, r_data->cmd, idle_fb, r_data->color_size);

        if (ret != 0) {
            mp_printf(&mp_plat_print, "esp_lcd_panel_io_tx_color error (%d)\n", ret);
        } else if (self->bufs.partial != NULL) {
            uint8_t *temp_buf = self->bufs.active;
            self->bufs.active = idle_fb;
            self->bufs.idle = temp_buf;
        }
    }
}


static bool i2c_init_func(mp_lcd_bus_obj_t *self_in)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *) self_in;

    self->init.err = i2c_param_config(self->host, &self->bus_config);
    if (self->init.err != 0) {
        self->init.err_msg = MP_ERROR_TEXT("%d(i2c_param_config)");
        return false;
    }

    self->init.err = i2c_driver_install(self->host, I2C_MODE_MASTER, 0, 0, 0);
    if (self->init.err != 0) {
        self->init.err_msg = MP_ERROR_TEXT("%d(i2c_driver_install)");
        return false;
    }

    self->init.err = esp_lcd_new_panel_io_i2c(self->bus_handle , &self->panel_io_config, &self->panel_io_handle);

    if (self->init.err != 0) {
        self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_i2c)");
        return false;
    }

    return true;
}


static mp_obj_t mp_lcd_i2c_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_sda, ARG_scl, ARG_addr, ARG_host, ARG_control_phase_bytes, ARG_dc_bit_offset, ARG_freq,
           ARG_dc_low_on_data, ARG_sda_pullup, ARG_scl_pullup, ARG_disable_control_phase };

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
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(make_new_args), make_new_args, args);

    // create new object
    mp_lcd_i2c_bus_obj_t *self = m_new_obj(mp_lcd_i2c_bus_obj_t);
    self->base.type = &mp_lcd_i2c_bus_type;

    self->callback = mp_const_none;

    self->host = args[ARG_host].u_int;
    self->bus_handle = (esp_lcd_i2c_bus_handle_t)((uint32_t)self->host);

    self->panel_io_handle = NULL;

    self->bus_config.mode = I2C_MODE_MASTER;
    self->bus_config.sda_io_num = (int)args[ARG_sda].u_int;
    self->bus_config.scl_io_num = (int)args[ARG_scl].u_int;
    self->bus_config.sda_pullup_en = (bool)args[ARG_sda_pullup].u_bool;
    self->bus_config.scl_pullup_en = (bool)args[ARG_scl_pullup].u_bool;
    self->bus_config.master.clk_speed = (uint32_t)args[ARG_freq].u_int;
    self->bus_config.clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;

    self->panel_io_config.dev_addr = (uint32_t)args[ARG_addr].u_int;
    self->panel_io_config.on_color_trans_done = &bus_trans_done_cb;
    self->panel_io_config.user_ctx = self;
    self->panel_io_config.control_phase_bytes = (size_t)args[ARG_control_phase_bytes].u_int;
    self->panel_io_config.dc_bit_offset = (unsigned int)args[ARG_dc_bit_offset].u_int;
    self->panel_io_config.flags.dc_low_on_data = (unsigned int)args[ARG_dc_low_on_data].u_bool;
    self->panel_io_config.flags.disable_control_phase = (unsigned int)args[ARG_disable_control_phase].u_bool;

    self->num_lanes = 1;

    self->internal_cb_funcs.deinit = &i2c_del;
    self->internal_cb_funcs.init = &i2c_init;

    self->tx_data.flush_func = &i2c_flush_func;
    self->init.init_func = &i2c_init_func;

    self->tx_cmds.send_func = &i2c_send_func;

    return MP_OBJ_FROM_PTR(self);
}


mp_lcd_err_t i2c_del(mp_lcd_bus_obj_t *self_in)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)self_in;

    if (self->panel_io_handle != NULL) {

        mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
            return ret;
        }

        ret = i2c_driver_delete(self->host);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(i2c_driver_delete)"), ret);
            return ret;
        }

        self->panel_io_handle = NULL;

        return ret;
    } else {
        return LCD_FAIL;
    }
}


mp_lcd_err_t i2c_init(mp_lcd_bus_obj_t *self_in, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,
                      uint8_t cmd_bits, uint8_t param_bits)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)self_in;

    self->panel_io_config.lcd_cmd_bits = (int)cmd_bits;
    self->panel_io_config.lcd_param_bits = (int)param_bits;


    xTaskCreatePinnedToCore(
                lcd_bus_task, "i2c_task", LCD_DEFAULT_STACK_SIZE / sizeof(StackType_t),
                self, ESP_TASK_PRIO_MAX - 1, &self->task.handle, 0);

        lcd_bus_lock_acquire(self->init.lock);
        lcd_bus_lock_release(self->init.lock);
        lcd_bus_lock_delete(self->init.lock);

        return self->init.err;
}


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_i2c_bus_type,
    MP_QSTR_I2CBus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_i2c_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
);

