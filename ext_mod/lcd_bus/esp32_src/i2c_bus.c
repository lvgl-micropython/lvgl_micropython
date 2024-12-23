
// local includes
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "i2c_bus.h"
#include "rotation.h"
#include "bus_task.h"

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
mp_lcd_err_t i2c_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits, bool sw_rotate);
mp_lcd_err_t i2c_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size, bool is_flush, bool last_flush_cmd);


static bool i2c_bus_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    LCD_UNUSED(panel_io);

    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)user_ctx;
    bus_event_set_from_isr(&self->rotation->task.swap_bufs);
    return false;
}


static mp_lcd_err_t i2c_rotation_init_func(void *self_in)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)self_in;

    self->panel_io_config->on_color_trans_done = &i2c_bus_trans_done_cb;

    rotation_init_err_t *init_err = &self->rotation->init_err;

    init_err->code = i2c_param_config(self->host, self->bus_config);
    if (init_err->code != LCD_OK) {
        init_err->msg = MP_ERROR_TEXT("%d(i2c_param_config)");
        return init_err->code;
    }

    init_err->code = i2c_driver_install(self->host, I2C_MODE_MASTER, 0, 0, 0);
    if (init_err->code != LCD_OK) {
        init_err->msg = MP_ERROR_TEXT("%d(i2c_driver_install)");
        return init_err->code;
    }

    init_err->code = esp_lcd_new_panel_io_i2c(self->bus_handle , self->panel_io_config, &self->panel_io_handle.panel_io);
    if (init_err->code != LCD_OK) {
        init_err->msg = MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_i2c)");
        return init_err->code;
    }

    free(self->bus_config);
    self->bus_config = NULL;

    free(self->panel_io_config);
    self->panel_io_config = NULL;

    return init_err->code;
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
    self->lane_count = 1;
    self->host = args[ARG_host].u_int;
    self->bus_handle = (esp_lcd_i2c_bus_handle_t)((uint32_t)self->host);

    self->bus_config = (i2c_config_t *)malloc(sizeof(i2c_config_t));
    self->bus_config->mode = I2C_MODE_MASTER;
    self->bus_config->sda_io_num = (int)args[ARG_sda].u_int;
    self->bus_config->scl_io_num = (int)args[ARG_scl].u_int;
    self->bus_config->sda_pullup_en = (bool)args[ARG_sda_pullup].u_bool;
    self->bus_config->scl_pullup_en = (bool)args[ARG_scl_pullup].u_bool;
    self->bus_config->master.clk_speed = (uint32_t)args[ARG_freq].u_int;
    self->bus_config->clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL;

    self->panel_io_config = (esp_lcd_panel_io_i2c_config_t *)malloc(sizeof(esp_lcd_panel_io_i2c_config_t));
    self->panel_io_config->dev_addr = (uint32_t)args[ARG_addr].u_int;
    self->panel_io_config->user_ctx = self;
    self->panel_io_config->control_phase_bytes = (size_t)args[ARG_control_phase_bytes].u_int;
    self->panel_io_config->dc_bit_offset = (unsigned int)args[ARG_dc_bit_offset].u_int;
    self->panel_io_config->flags.dc_low_on_data = (unsigned int)args[ARG_dc_low_on_data].u_bool;
    self->panel_io_config->flags.disable_control_phase = (unsigned int)args[ARG_disable_control_phase].u_bool;

    self->panel_io_handle.del = &i2c_del;
    self->panel_io_handle.init = &i2c_init;
    self->panel_io_handle.tx_param = &i2c_tx_param;

    return MP_OBJ_FROM_PTR(self);
}


mp_lcd_err_t i2c_del(mp_obj_t obj)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)obj;

    mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle.panel_io);
    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
    }
    ret = i2c_driver_delete(self->host);
    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(i2c_driver_delete)"), ret);
    }

    return ret;
}


mp_lcd_err_t i2c_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits, bool sw_rotate)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)obj;

    if (bpp == 16) {
        self->rgb565_byte_swap = rgb565_byte_swap;
    } else {
        self->rgb565_byte_swap = false;
    }

    self->panel_io_config->lcd_cmd_bits = (int)cmd_bits;
    self->panel_io_config->lcd_param_bits = (int)param_bits;

    esp_err_t ret;

    if (sw_rotate) {
        self->rotation = (rotation_t *)malloc(sizeof(rotation_t));
        self->rotation->init_func = &i2c_rotation_init_func;

        ret = rotation_set_buffers(self);
        if (ret != LCD_OK) mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("unable to allocate sw rotate fb"));

        rotation_task_start(self);
        ret = self->rotation->init_err.code;

        if (ret != LCD_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, self->rotation->init_err.msg, self->rotation->init_err.code);
        }
    } else {
        self->panel_io_config->on_color_trans_done = &bus_trans_done_cb;

        ret = i2c_param_config(self->host, self->bus_config);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(i2c_param_config)"), ret);
        }

        ret = i2c_driver_install(self->host, I2C_MODE_MASTER, 0, 0, 0);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(i2c_driver_install)"), ret);
        }

        ret = esp_lcd_new_panel_io_i2c(self->bus_handle , self->panel_io_config, &self->panel_io_handle.panel_io);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_i2c)"), ret);
        }

        free(self->bus_config);
        self->bus_config = NULL;

        free(self->panel_io_config);
        self->panel_io_config = NULL;
    }

    return ret;
}


mp_lcd_err_t i2c_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size, bool is_flush, bool last_flush_cmd)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)obj;

    mp_lcd_err_t ret;

    if (self->rotation == NULL || !is_flush) {
        LCD_UNUSED(last_flush_cmd);
        ret = esp_lcd_panel_io_tx_param(self->panel_io_handle.panel_io, lcd_cmd, param, param_size);
    } else {
        bus_lock_acquire(&self->rotation->task.tx_param_lock, -1);

        if (self->rotation->data.tx_param_count == 24) {
            bus_lock_release(&self->rotation->task.tx_param_lock);
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("tx_parameter overflow."));
        } else {
            uint8_t tx_param_count = self->rotation->data.tx_param_count;

            self->rotation->data.param_cmd[tx_param_count] = lcd_cmd;
            self->rotation->data.param[tx_param_count] = param;
            self->rotation->data.param_size[tx_param_count] = param_size;
            self->rotation->data.param_last_cmd[tx_param_count] = last_flush_cmd;
            self->rotation->data.tx_param_count++;

            bus_lock_release(&self->rotation->task.tx_param_lock);
        }
        ret = LCD_OK;
    }
    return ret;
}


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_i2c_bus_type,
    MP_QSTR_I2CBus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_i2c_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
);

