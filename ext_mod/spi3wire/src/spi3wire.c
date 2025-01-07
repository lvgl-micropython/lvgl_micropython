// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"
#include "py/runtime.h"
#include "esp_lcd_panel_io_additions.h"
#include "spi3wire.h"

#include <string.h>


static mp_obj_t spi3wire_deinit(mp_obj_t obj);


static uint8_t spi3wire_count = 0;
static mp_spi3wire_obj_t **spi3wire_objs;


void mp_spi3wire_deinit_all(void)
{
    // we need to copy the existing array to a new one so the order doesn't
    // get all mucked up when objects get removed.
    mp_spi3wire_obj_t *objs[spi3wire_count];

    for (uint8_t i=0;i<spi3wire_count;i++) {
        objs[i] = spi3wire_objs[i];
    }

    for (uint8_t i=0;i<spi3wire_count;i++) {
        spi3wire_deinit(MP_OBJ_FROM_PTR(objs[i]));
    }
}


static mp_obj_t spi3wire_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{

    enum {
        ARG_scl,
        ARG_sda,
        ARG_cs,
        ARG_freq,
        ARG_spi_mode,
        ARG_use_dc_bit,
        ARG_dc_zero_on_data,
        ARG_lsb_first,
        ARG_cs_high_active,
        ARG_del_keep_cs_inactive
    };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_scl,                  MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
        { MP_QSTR_sda,                  MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
        { MP_QSTR_cs,                   MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
        { MP_QSTR_freq,                 MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
        { MP_QSTR_spi_mode,             MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = 0      } },
        { MP_QSTR_use_dc_bit,           MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = true  } },
        { MP_QSTR_dc_zero_on_data,      MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false } },
        { MP_QSTR_lsb_first,            MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false } },
        { MP_QSTR_cs_high_active,       MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false } },
        { MP_QSTR_del_keep_cs_inactive, MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false } }
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
    mp_spi3wire_obj_t *self = m_new_obj(mp_spi3wire_obj_t);
    self->base.type = &mp_spi3wire_type;

    self->io_config = (esp_lcd_panel_io_3wire_spi_config_t *)malloc(sizeof(esp_lcd_panel_io_3wire_spi_config_t));
    esp_lcd_panel_io_3wire_spi_config_t *config = self->io_config;

    config->flags.use_dc_bit = (uint32_t)args[ARG_use_dc_bit].u_bool;
    config->flags.dc_zero_on_data = (uint32_t)args[ARG_dc_zero_on_data].u_bool;
    config->flags.lsb_first = (uint32_t)args[ARG_lsb_first].u_bool;
    config->flags.cs_high_active = (uint32_t)args[ARG_cs_high_active].u_bool;
    config->flags.del_keep_cs_inactive = (uint32_t)args[ARG_del_keep_cs_inactive].u_bool;
    config->spi_mode = (uint32_t)args[ARG_spi_mode].u_int;
    config->expect_clk_speed = (uint32_t)args[ARG_freq].u_int;
    config->line_config.cs_gpio_num = (int)args[ARG_cs].u_int;
    config->line_config.scl_gpio_num = (int)args[ARG_scl].u_int;
    config->line_config.sda_gpio_num = (int)args[ARG_sda].u_int;

    if (config->expect_clk_speed > PANEL_IO_3WIRE_SPI_CLK_MAX) {
       mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("frequency is greater than %d"), PANEL_IO_3WIRE_SPI_CLK_MAX);
    }

    SPI3WIRE_DEBUG_PRINT("use_dc_bit=%d\n", config->flags.use_dc_bit)
    SPI3WIRE_DEBUG_PRINT("dc_zero_on_data=%d\n", config->flags.dc_zero_on_data)
    SPI3WIRE_DEBUG_PRINT("lsb_first=%d\n", config->flags.lsb_first)
    SPI3WIRE_DEBUG_PRINT("cs_high_active=%d\n", config->flags.cs_high_active)
    SPI3WIRE_DEBUG_PRINT("del_keep_cs_inactive=%d\n", config->flags.del_keep_cs_inactive)
    SPI3WIRE_DEBUG_PRINT("spi_mode=%d\n", config->spi_mode)
    SPI3WIRE_DEBUG_PRINT("expect_clk_speed=%d\n", config->expect_clk_speed)
    SPI3WIRE_DEBUG_PRINT("cs_gpio_num=%d\n", config->line_config.cs_gpio_num)
    SPI3WIRE_DEBUG_PRINT("scl_gpio_num=%d\n", config->line_config.scl_gpio_num)
    SPI3WIRE_DEBUG_PRINT("sda_gpio_num=%d\n", config->line_config.sda_gpio_num)

    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t spi3wire_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_cmd_bits, ARG_param_bits };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,             MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_cmd_bits,         MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_param_bits,       MP_ARG_INT  | MP_ARG_REQUIRED }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_spi3wire_obj_t *self = (mp_spi3wire_obj_t *)args[ARG_self].u_obj;

    self->io_config->lcd_cmd_bytes = ((uint32_t)args[ARG_cmd_bits].u_int) / 8;
    self->io_config->lcd_param_bytes = ((uint32_t)args[ARG_param_bits].u_int) / 8;

    SPI3WIRE_DEBUG_PRINT("lcd_cmd_bytes=%d\n", self->io_config->lcd_cmd_bytes)
    SPI3WIRE_DEBUG_PRINT("lcd_param_bytes=%d\n", self->io_config->lcd_param_bytes)

    esp_err_t ret = esp_lcd_new_panel_io_3wire_spi(self->io_config, &self->panel_io_handle);

    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_3wire_spi)"), ret);
    }

    spi3wire_count++;
    spi3wire_objs = m_realloc(spi3wire_objs, spi3wire_count * sizeof(mp_spi3wire_obj_t *));
    spi3wire_objs[spi3wire_count - 1] = self;

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_KW(spi3wire_init_obj, 3, spi3wire_init);


static mp_obj_t spi3wire_tx_param(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_cmd, ARG_params };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,    MP_ARG_OBJ | MP_ARG_REQUIRED, { .u_obj = mp_const_none } },
        { MP_QSTR_cmd,     MP_ARG_INT | MP_ARG_REQUIRED, { .u_int = -1            } },
        { MP_QSTR_params,  MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_spi3wire_obj_t *self = (mp_spi3wire_obj_t *)args[ARG_self].u_obj;

    if (self->panel_io_handle != NULL) {
        int cmd = (int)args[ARG_cmd].u_int;

        void *buf = NULL;
        size_t buf_len = 0;

        if (args[ARG_params].u_obj != mp_const_none) {
            mp_buffer_info_t bufinfo;
            mp_get_buffer_raise(args[ARG_params].u_obj, &bufinfo, MP_BUFFER_READ);
            buf_len = bufinfo.len;
            buf = bufinfo.buf;
        }

        SPI3WIRE_DEBUG_PRINT("spi3wire_tx_param(self, lcd_cmd=%d, param, param_size=%d)\n", cmd, buf_len)

        esp_err_t ret = esp_lcd_panel_io_tx_param(self->panel_io_handle, cmd, buf, buf_len);

        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_tx_param)"), ret);
        }
    }

    return mp_const_none;
}


static MP_DEFINE_CONST_FUN_OBJ_KW(spi3wire_tx_param_obj, 2, spi3wire_tx_param);


mp_obj_t spi3wire_deinit(mp_obj_t obj)
{
    SPI3WIRE_DEBUG_PRINT("spi3wire_deinit(self)\n")

    mp_spi3wire_obj_t *self = (mp_spi3wire_obj_t *)obj;

    if (self->panel_io_handle != NULL) {
        esp_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
        }

        self->panel_io_handle = NULL;

        uint8_t i = 0;
        for (;i<spi3wire_count;i++) {
            if (spi3wire_objs[i] == self) {
                spi3wire_objs[i] = NULL;
                break;
            }
        }

        for (uint8_t j=i + 1;j<spi3wire_count;j++) {
            spi3wire_objs[j - i + 1] = spi3wire_objs[j];
        }

        spi3wire_count--;
        spi3wire_objs = m_realloc(spi3wire_objs, spi3wire_count * sizeof(mp_spi3wire_obj_t *));
    }

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_1(spi3wire_deinit_obj, spi3wire_deinit);


static const mp_rom_map_elem_t spi3wire_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_tx_param), MP_ROM_PTR(&spi3wire_tx_param_obj) },
    { MP_ROM_QSTR(MP_QSTR_init),     MP_ROM_PTR(&spi3wire_init_obj)     },
    { MP_ROM_QSTR(MP_QSTR_deinit),   MP_ROM_PTR(&spi3wire_deinit_obj)   },
    { MP_ROM_QSTR(MP_QSTR___del__),  MP_ROM_PTR(&spi3wire_deinit_obj)   }
};


static MP_DEFINE_CONST_DICT(spi3wire_locals_dict, spi3wire_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_spi3wire_type,
    MP_QSTR_Spi3Wire,
    MP_TYPE_FLAG_NONE,
    make_new, spi3wire_make_new,
    locals_dict, (mp_obj_dict_t *)&spi3wire_locals_dict
);


static const mp_rom_map_elem_t spi3wire_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_spi3wire) },
    { MP_ROM_QSTR(MP_QSTR_Spi3Wire), MP_ROM_PTR(&mp_spi3wire_type)     },
};

static MP_DEFINE_CONST_DICT(spi3wire_globals, spi3wire_globals_table);


const mp_obj_module_t module_spi3wire = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&spi3wire_globals,
};

MP_REGISTER_MODULE(MP_QSTR_spi3wire, module_spi3wire);