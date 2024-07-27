
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "spi_3wire.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_additions.h"


static mp_obj_t mp_lcd_spi_3wire_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum {
        ARG_mosi,
        ARG_sclk,
        ARG_freq,
        ARG_cs,
        ARG_cs_high_active,
        ARG_keep_cs_inactive,
        ARG_lsb_first,
        ARG_dc_zero_on_data,
        ARG_use_dc_bit,

        };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_mosi,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED                         },
        { MP_QSTR_sclk,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED                         },
        { MP_QSTR_freq,              MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = PANEL_IO_3WIRE_SPI_CLK_MAX } },
        { MP_QSTR_cs,                MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = -1                         } },
        { MP_QSTR_cs_high_active,    MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false                     } },
        { MP_QSTR_keep_cs_inactive,  MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = true                      } },
        { MP_QSTR_lsb_first,         MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false                     } },
        { MP_QSTR_dc_zero_on_data,   MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false                     } },
        { MP_QSTR_use_dc_bit,        MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false                     } },
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
    mp_lcd_spi_3wire_obj_t *self = m_new_obj(mp_lcd_spi_3wire_obj_t);
    self->base.type = &mp_lcd_spi_3wire_type;

    self->freq = (uint32_t)args[ARG_freq].u_int;
    self->cs_io_num = (int)args[ARG_cs].u_int;
    self->sclk_io_num = (int)args[ARG_sclk].u_int;
    self->mosi_io_num = (int)args[ARG_mosi].u_int;

    self->flags = (uint8_t)args[ARG_cs_high_active].u_bool;
    self->flags = self->flags | ((uint8_t)args[ARG_keep_cs_inactive].u_bool << 1);
    self->flags = self->flags | ((uint8_t)args[ARG_lsb_first].u_bool << 2);
    self->flags = self->flags | ((uint8_t)args[ARG_dc_zero_on_data].u_bool << 3);
    self->flags = self->flags | ((uint8_t)args[ARG_use_dc_bit].u_bool << 4);

    return MP_OBJ_FROM_PTR(self);
}


mp_obj_t mp_lcd_spi_3wire_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_cmd_bits, ARG_param_bits };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,             MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_cmd_bits,         MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_param_bits,       MP_ARG_INT  | MP_ARG_REQUIRED }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_lcd_spi_3wire_obj_t *self = (mp_lcd_spi_3wire_obj_t *)args[ARG_self].u_obj;

    if (self->freq <= PANEL_IO_3WIRE_SPI_CLK_MAX) {

        spi_line_config_t spi_line = {
            .cs_io_type=IO_TYPE_GPIO,
            .cs_gpio_num=self->cs_io_num,
            .scl_io_type=IO_TYPE_GPIO,
            .scl_gpio_num=self->sclk_io_num,
            .sda_io_type=IO_TYPE_GPIO,
            .sda_gpio_num=self->mosi_io_num,
            .io_expander=NULL,
        };

        esp_lcd_panel_io_3wire_spi_config_t io_config = {
            .line_config = spi_line,
            .expect_clk_speed = self->freq,
            .spi_mode = 0,
            .lcd_cmd_bytes = (uint32_t)args[ARG_cmd_bits].u_int / 8,
            .lcd_param_bytes = (uint32_t)args[ARG_param_bits].u_int / 8,
            .flags = {
                .use_dc_bit = (uint32_t)((self->flags >> 0) & 0x1),
                .dc_zero_on_data = (uint32_t)((self->flags >> 1) & 0x1),
                .lsb_first = (uint32_t)((self->flags >> 2) & 0x1),
                .cs_high_active = (uint32_t)((self->flags >> 3) & 0x1),
                .del_keep_cs_inactive = (uint32_t)((self->flags >> 4) & 0x1),
            },
        };

        self->panel_io = NULL;
        esp_err_t err = esp_lcd_new_panel_io_3wire_spi(&io_config, &self->panel_io);

        if (err != 0) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_3wire_spi)"), err);
        }
    } else {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("Frequency must be at or below %d"), PANEL_IO_3WIRE_SPI_CLK_MAX);
    }
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_spi_3wire_init_obj, 3, mp_lcd_spi_3wire_init);


mp_obj_t mp_lcd_spi_3wire_tx_param(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_cmd, ARG_params };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,    MP_ARG_OBJ | MP_ARG_REQUIRED                  },
        { MP_QSTR_cmd,     MP_ARG_INT | MP_ARG_REQUIRED, { .u_int = -1 } },
        { MP_QSTR_params,  MP_ARG_OBJ,          {.u_obj = mp_const_none} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_lcd_spi_3wire_obj_t *self = (mp_lcd_spi_3wire_obj_t *)args[ARG_self].u_obj;

    if (self->panel_io != NULL) {
        esp_err_t err;

        if (args[ARG_params].u_obj != mp_const_none) {
            mp_buffer_info_t bufinfo;
            mp_get_buffer_raise(args[ARG_params].u_obj, &bufinfo, MP_BUFFER_READ);

            err = esp_lcd_panel_io_tx_param(self->panel_io, (int)args[ARG_cmd].u_int, bufinfo.buf, (size_t)bufinfo.len);
        } else {
            err = esp_lcd_panel_io_tx_param(self->panel_io, (int)args[ARG_cmd].u_int, NULL, 0);
        }

        if (err != 0) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_tx_param)"), err);
        }
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_lcd_spi_3wire_tx_param_obj, 2, mp_lcd_spi_3wire_tx_param);


mp_obj_t mp_lcd_spi_3wire_deinit(mp_obj_t obj)
{
    mp_lcd_spi_3wire_obj_t *self = (mp_lcd_spi_3wire_obj_t *)obj;

    if (self->panel_io != NULL) {

        esp_err_t err = esp_lcd_panel_io_del(self->panel_io);
        if (err != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), err);
        } else {
            self->panel_io = NULL;
        }
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_lcd_spi_3wire_deinit_obj, mp_lcd_spi_3wire_deinit);


static const mp_rom_map_elem_t mp_lcd_spi_3wire_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init),      MP_ROM_PTR(&mp_lcd_spi_3wire_init_obj)     },
    { MP_ROM_QSTR(MP_QSTR_tx_param),  MP_ROM_PTR(&mp_lcd_spi_3wire_tx_param_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),    MP_ROM_PTR(&mp_lcd_spi_3wire_deinit_obj)   },
    { MP_ROM_QSTR(MP_QSTR___del__),   MP_ROM_PTR(&mp_lcd_spi_3wire_deinit_obj)   }
};

MP_DEFINE_CONST_DICT(mp_lcd_spi_3wire_locals_dict, mp_lcd_spi_3wire_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_spi_3wire_type,
    MP_QSTR_SPI3Wire,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_spi_3wire_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_spi_3wire_locals_dict
);
