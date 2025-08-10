// Copyright (c) 2024 - 2025 Kevin G. Schlosser

// local includes
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "i2c_bus.h"
#include "../../../micropy_updates/common/mp_i2c_common.h"
#include "extmod/modmachine.h"


// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>


#define LCD_BYTESHIFT(VAR, IDX) (((VAR) >> ((IDX) * 8)) & 0xFF)


void i2c_deinit_callback(mp_machine_hw_i2c_device_obj_t *device)

mp_lcd_err_t i2c_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
mp_lcd_err_t i2c_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
mp_lcd_err_t i2c_del(mp_obj_t obj);
mp_lcd_err_t i2c_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
void i2c_transmit(mp_lcd_i2c_bus_obj_t *self, int lcd_cmd, const void *buffer,
                  size_t buffer_size, bool is_param);
mp_lcd_err_t i2c_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp,
                      uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits,
                      uint8_t param_bits);
mp_lcd_err_t i2c_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size,
                          int x_start, int y_start, int x_end, int y_end,
                          uint8_t rotation, bool last_update);


static uint8_t i2c_bus_count = 0;
static mp_lcd_i2c_bus_obj_t **i2c_bus_objs;


void mp_lcd_i2c_bus_deinit_all(void)
{
    // we need to copy the existing array to a new one so the order doesn't
    // get all mucked up when objects get removed.
}


static mp_obj_t mp_lcd_i2c_bus_make_new(const mp_obj_type_t *type, size_t n_args,
                                        size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_i2c_bus, ARG_addr, ARG_freq, ARG_control_phase_bytes,
           ARG_dc_bit_offset, ARG_dc_low_on_data, ARG_disable_control_phase };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_i2c_bus,               MP_ARG_OBJ  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
        { MP_QSTR_addr,                  MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
        { MP_QSTR_freq,                  MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
        { MP_QSTR_control_phase_bytes,   MP_ARG_INT  | MP_ARG_KW_ONLY,  {.u_int = 1         } },
        { MP_QSTR_dc_bit_offset,         MP_ARG_INT  | MP_ARG_KW_ONLY,  {.u_int = 6         } },
        { MP_QSTR_dc_low_on_data,        MP_ARG_BOOL | MP_ARG_KW_ONLY,  {.u_bool = true     } },
        { MP_QSTR_disable_control_phase, MP_ARG_BOOL | MP_ARG_KW_ONLY,  {.u_bool = false    } }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(make_new_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                              MP_ARRAY_SIZE(make_new_args), make_new_args, args);

    // create new object
    mp_lcd_i2c_bus_obj_t *self = m_new_obj(mp_lcd_i2c_bus_obj_t);
    self->base.type = &mp_lcd_i2c_bus_type;

    self->callback = mp_const_none;

    mp_machine_hw_i2c_bus_obj_t *i2c_bus = MP_OBJ_TO_PTR(args[ARG_i2c_bus].u_obj);

    self->i2c_device.base.type = &mp_machine_hw_i2c_device_type;
    self->i2c_device.timeout = 50000;
    self->i2c_device.freq = (uint32_t)args[ARG_freq].u_int;
    self->i2c_device.i2c_bus = i2c_bus;
    self->i2c_device.deinit = &i2c_deinit_callback;
    self->i2c_device.user_data = self;
    self->i2c_device.addr = (uint8_t)args[ARG_addr].u_int;

    size_t control_phase_bytes = (size_t)args[ARG_control_phase_bytes].u_int;
    uint32_t dc_low_on_data = (uint32_t)args[ARG_dc_low_on_data].u_bool;
    uint32_t dc_bit_offset = (uint32_t)args[ARG_dc_bit_offset].u_int;

    if (control_phase_bytes * 8 < dc_bit_offset) {
         mp_raise_msg(&mp_type_ValueError,
                      MP_ERROR_TEXT("`dc_bit_offset` exceeds `control_phase_bytes`"));
    }


    self->control_phase_data = (!dc_low_on_data) << dc_bit_offset;
    self->control_phase_cmd = dc_low_on_data << dc_bit_offset;
    self->control_phase_enabled = (uint8_t)args[ARG_disable_control_phase].u_bool) ? 0 : 1;
    self->port = i2c_bus->port;

    self->panel_io_handle.tx_color = &i2c_tx_color;
    self->panel_io_handle.rx_param = &i2c_rx_param;
    self->panel_io_handle.tx_param = &i2c_tx_param;
    self->panel_io_handle.del = &i2c_del;
    self->panel_io_handle.init = &i2c_init;
    self->panel_io_handle.get_lane_count = &i2c_get_lane_count;

    return MP_OBJ_FROM_PTR(self);
}


static mp_lcd_err_t i2c_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
{
    LCD_UNUSED(obj);
    LCD_UNUSED(lcd_cmd);
    LCD_UNUSED(param);
    LCD_UNUSED(param_size);

    return LCD_ERR_NOT_SUPPORTED;
}



static mp_lcd_err_t i2c_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
{
    mp_lcd_i2c_bus_obj_t *self = MP_OBJ_TO_PTR(obj);
    void i2c_transmit(self, lcd_cmd, param, param_size, true);
    return LCD_OK;
}


static mp_lcd_err_t i2c_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size,
                          int x_start, int y_start, int x_end, int y_end, uint8_t rotation,
                          bool last_update)
{
    LCD_UNUSED(x_start);
    LCD_UNUSED(y_start);
    LCD_UNUSED(x_end);
    LCD_UNUSED(y_end);
    LCD_UNUSED(rotation);
    LCD_UNUSED(last_update);
    LCD_UNUSED(last_update);

    mp_lcd_i2c_bus_obj_t *self = MP_OBJ_TO_PTR(obj);
    void i2c_transmit(self, lcd_cmd, color, color_size, false);
    return LCD_OK;
}


static void i2c_deinit_callback(mp_machine_hw_i2c_device_obj_t *device)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)device->user_data;
    i2c_del(MP_OBJ_FROM_PTR(self));
}


mp_lcd_err_t i2c_del(mp_obj_t obj)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)obj;

    if (self->i2c_device.i2c_bus != NULL) {
        mp_machine_hw_i2c_bus_remove_device(&self->i2c_device);

        self->i2c_device.i2c_bus = NULL;

        if (self->view1 != NULL) {
            m_free(self->view1->items);
            self->view1->items = NULL;
            self->view1->len = 0;
            self->view1 = NULL;
        }

        if (self->view2 != NULL) {
            m_free(self->view2->items);
            self->view2->items = NULL;
            self->view2->len = 0;
            self->view2 = NULL;
        }

        return LCD_OK;
    } else {
        return LCD_FAIL;
    }
}


mp_lcd_err_t i2c_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp,
                      uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits,
                      uint8_t param_bits)
{
    mp_lcd_i2c_bus_obj_t *self = (mp_lcd_i2c_bus_obj_t *)obj;

    if (bpp == 16) {
        self->rgb565_byte_swap = rgb565_byte_swap;
    } else {
        self->rgb565_byte_swap = false;
    }

    self->lcd_cmd_bits = cmd_bits;
    self->lcd_param_bits = param_bits;

    mp_machine_hw_i2c_bus_add_device(&self->i2c_device);

    return LCD_OK;
}


mp_lcd_err_t i2c_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
{
    LCD_UNUSED(obj);
    *lane_count = 1;
    return LCD_OK;
}


static void i2c_transmit(mp_lcd_i2c_bus_obj_t *self, int lcd_cmd, const void *buffer, size_t buffer_size, bool is_param)
{

    mp_machine_hw_i2c_device_obj_t *i2c_device = &self->i2c_device;

    size_t bufs_len = 0;

    bool send_param = (lcd_cmd >= 0);

    uint8_t control_phase_byte = 0;
    size_t control_phase_size = 0;
    if (self->control_phase_enabled) {
        control_phase_byte = is_param ? self->control_phase_cmd : self->control_phase_data;
        control_phase_size = 1;
        bufs_len++;
    }

    uint8_t *cmd_buffer = NULL;
    size_t cmd_buffer_size = 0;
    // some displays don't want any additional commands on data transfers
    uint8_t cmds[4] = { LCD_BYTESHIFT(lcd_cmd, 3), LCD_BYTESHIFT(lcd_cmd, 2), LCD_BYTESHIFT(lcd_cmd, 1), LCD_BYTESHIFT(lcd_cmd, 0) };
    if (send_param) {
        size_t cmds_size = self->lcd_cmd_bits / 8;
        if (cmds_size > 0 && cmds_size <= sizeof(cmds)) {
            cmd_buffer = cmds + (sizeof(cmds) - cmds_size);
            cmd_buffer_size = cmds_size;
            bufs_len++;
        }
    }

    uint8_t *lcd_buffer = NULL;
    size_t lcd_buffer_size = 0;
    if (buffer) {
        lcd_buffer = (uint8_t*)buffer;
        lcd_buffer_size = buffer_size;
        bufs_len++;
    }

    if (bufs_len) {
        mp_machine_i2c_buf_t *bufs = mp_local_alloc(bufs_len * sizeof(mp_machine_i2c_buf_t));

        if (control_phase_size) {
            bufs[0].buf = &control_phase_byte;
            bufs[0].len = control_phase_size;
        }

        if (cmd_buffer != NULL) {
            if (control_phase_size) {
                bufs[1].buf = cmd_buffer;
                bufs[1].len = cmd_buffer_size;
            } else {
                bufs[0].buf = cmd_buffer;
                bufs[0].len = cmd_buffer_size;
            }
        }

        if (lcd_buffer != NULL) {
            if (control_phase_size && cmd_buffer != NULL) {
                bufs[2].buf = lcd_buffer;
                bufs[2].len = lcd_buffer_size;
            } else if (control_phase_size || cmd_buffer != NULL) {
                bufs[1].buf = lcd_buffer;
                bufs[1].len = lcd_buffer_size;
            } else {
                bufs[0].buf = lcd_buffer;
                bufs[0].len = lcd_buffer_size;
            }
        }

        mp_machine_i2c_p_t *i2c_p = (mp_machine_i2c_p_t *)MP_OBJ_TYPE_GET_SLOT(i2c_device->base.type, protocol);
        i2c_p->transfer((mp_obj_base_t *)i2c_device, i2c_device->addr, bufs_len, bufs, MP_MACHINE_I2C_FLAG_STOP);
        mp_local_free(bufs);
    }

    if (!is_param) bus_trans_done_cb(&self->panel_io_handle, NULL, self);
}


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_i2c_bus_type,
    MP_QSTR_I2CBus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_i2c_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
);
