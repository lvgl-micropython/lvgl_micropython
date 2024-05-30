
// local includes
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "spi_bus.h"
#include "esp32_spi_bus.h"

// esp-idf includes
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "soc/gpio_sig_map.h"
#include "soc/spi_pins.h"
#include "soc/soc_caps.h"
#include "rom/gpio.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_heap_caps.h"
#include "hal/spi_types.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>


mp_lcd_err_t spi_del(mp_obj_t obj);
mp_lcd_err_t spi_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap);
mp_lcd_err_t spi_get_lane_count(mp_obj_t obj, uint8_t *lane_count);


STATIC mp_obj_t mp_lcd_spi_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
     enum {
        ARG_spi_bus,
        ARG_freq,
        ARG_dc,
        ARG_cs,
        ARG_polarity,
        ARG_phase,
        ARG_firstbit,
        ARG_cmd_bits,
        ARG_param_bits,
        ARG_cs_high_active,
        ARG_dc_low_on_data,
    };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_spi_bus,        MP_ARG_OBJ  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED    },
        { MP_QSTR_freq,           MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED    },
        { MP_QSTR_dc,             MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED    },
        { MP_QSTR_cs,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int  = -1    } },
        { MP_QSTR_polarity,       MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int  =  0    } },
        { MP_QSTR_phase,          MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int  =  0    } },
        { MP_QSTR_firstbit,       MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int  = MICROPY_PY_MACHINE_SPI_MSB } },
        { MP_QSTR_cmd_bits,       MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int  =  8    } },
        { MP_QSTR_param_bits,     MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int  =  8    } },
        { MP_QSTR_cs_high_active, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false } },
        { MP_QSTR_dc_low_on_data, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false } },
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
    mp_lcd_spi_bus_obj_t *self = m_new_obj(mp_lcd_spi_bus_obj_t);
    self->base.type = &mp_lcd_spi_bus_type;

    self->callback = mp_const_none;
    self->spi_bus = (esp32_hw_spi_bus_obj_t *)MP_OBJ_TO_PTR(args[ARG_spi_bus].u_obj);
    self->panel_io_handle.panel_io = NULL;
    self->bus_handle = (esp_lcd_spi_bus_handle_t)self->spi_bus->host;

    self->panel_io_config.cs_gpio_num = (int)args[ARG_cs].u_int;
    self->panel_io_config.dc_gpio_num = (int)args[ARG_dc].u_int;
    self->panel_io_config.spi_mode = (int)args[ARG_phase].u_int | ((int)args[ARG_polarity].u_int << 1);
    self->panel_io_config.pclk_hz = (unsigned int)args[ARG_freq].u_int;
    self->panel_io_config.on_color_trans_done = &bus_trans_done_cb;
    self->panel_io_config.user_ctx = self;
    self->panel_io_config.lcd_cmd_bits = (int)args[ARG_cmd_bits].u_int;
    self->panel_io_config.lcd_param_bits = (int)args[ARG_param_bits].u_int;
    self->panel_io_config.flags.dc_low_on_data = (unsigned int)args[ARG_dc_low_on_data].u_bool;
    self->panel_io_config.flags.sio_mode = (unsigned int)self->spi_bus->dual_mode;
    self->panel_io_config.flags.lsb_first = (unsigned int)args[ARG_firstbit].u_int == MICROPY_PY_MACHINE_SPI_LSB ? 1 : 0;
    self->panel_io_config.flags.cs_high_active = (unsigned int)args[ARG_cs_high_active].u_bool;
    self->panel_io_config.flags.octal_mode = (unsigned int)self->spi_bus->octal_mode;

    self->panel_io_handle.del = &spi_del;
    self->panel_io_handle.init = &spi_init;
    self->panel_io_handle.get_lane_count = &spi_get_lane_count;

#if CONFIG_LCD_ENABLE_DEBUG_LOG
    printf("host=%d\n", self->spi_bus->host);
    printf("sclk_io_num=%d\n", self->spi_bus->buscfg.sclk_io_num);
    printf("mosi_io_num=%d\n", self->spi_bus->buscfg.mosi_io_num);
    printf("miso_io_num=%d\n", self->spi_bus->buscfg.miso_io_num);
    printf("quadwp_io_num=%d\n", self->spi_bus->buscfg.quadwp_io_num);
    printf("quadhd_io_num=%d\n", self->spi_bus->buscfg.quadhd_io_num);
    printf("cs_gpio_num=%d\n", self->panel_io_config.cs_gpio_num);
    printf("dc_gpio_num=%d\n", self->panel_io_config.dc_gpio_num);
    printf("spi_mode=%d\n", self->panel_io_config.spi_mode);
    printf("pclk_hz=%lu\n", self->panel_io_config.pclk_hz);
    printf("lcd_cmd_bits=%d\n", self->panel_io_config.lcd_cmd_bits);
    printf("lcd_param_bits=%d\n", self->panel_io_config.lcd_param_bits);
    printf("dc_low_on_data=%lu\n", self->panel_io_config.flags.dc_low_on_data);
    printf("sio_mode=%lu\n", self->panel_io_config.flags.sio_mode);
    printf("lsb_first=%lu\n", self->panel_io_config.flags.lsb_first);
    printf("cs_high_active=%lu\n", self->panel_io_config.flags.cs_high_active);
    printf("octal_mode=%lu\n", self->panel_io_config.flags.octal_mode);

#endif

    return MP_OBJ_FROM_PTR(self);
}


mp_lcd_err_t spi_del(mp_obj_t obj)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;

    mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle.panel_io);
    if (ret != ESP_OK) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
    }

    ret = spi_bus_free(self->spi_bus->host);
    if (ret != ESP_OK) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(spi_bus_free)"), ret);
    }
    return ret;
}


mp_lcd_err_t spi_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap)
{
#if CONFIG_LCD_ENABLE_DEBUG_LOG
    printf("spi_init(self, width=%i, height=%i, bpp=%i, buffer_size=%lu, rgb565_byte_swap=%i)\n", width, height, bpp, buffer_size, (uint8_t)rgb565_byte_swap);
#endif
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;

    if (bpp == 16) {
        self->rgb565_byte_swap = rgb565_byte_swap;
    } else {
        self->rgb565_byte_swap = false;
    }

    self->panel_io_config.trans_queue_depth = 10;

#if CONFIG_LCD_ENABLE_DEBUG_LOG
    printf("rgb565_byte_swap=%i\n",  (uint8_t)self->rgb565_byte_swap);
    printf("trans_queue_depth=%i\n", (uint8_t)self->panel_io_config.trans_queue_depth);
#endif

    mp_lcd_err_t ret = esp_lcd_new_panel_io_spi(self->bus_handle, &self->panel_io_config, &self->panel_io_handle.panel_io);
    if (ret != ESP_OK) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_spi)"), ret);
    }

    return ret;
}


mp_lcd_err_t spi_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;

    *lane_count = 1;

    if (self->panel_io_config.flags.sio_mode) {
        *lane_count = 2;
    } else if (self->spi_bus->octal_mode) {
        *lane_count = 8;
    } else {
        *lane_count = 0;
    }

#if CONFIG_LCD_ENABLE_DEBUG_LOG
    printf("spi_get_lane_count(self) -> %i\n", (uint8_t)(*lane_count));
#endif

    return LCD_OK;
}


mp_obj_t mp_spi_bus_get_host(mp_obj_t obj)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;

#if CONFIG_LCD_ENABLE_DEBUG_LOG
    printf("mp_spi_bus_get_host(self) -> %i\n", (uint8_t)self->spi_bus->host);
#endif

    return mp_obj_new_int((uint8_t)self->spi_bus->host);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_spi_bus_get_host_obj, mp_spi_bus_get_host);


STATIC const mp_rom_map_elem_t mp_lcd_spi_bus_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_host),             MP_ROM_PTR(&mp_spi_bus_get_host_obj)             },
    { MP_ROM_QSTR(MP_QSTR_get_lane_count),       MP_ROM_PTR(&mp_lcd_bus_get_lane_count_obj)       },
    { MP_ROM_QSTR(MP_QSTR_allocate_framebuffer), MP_ROM_PTR(&mp_lcd_bus_allocate_framebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_free_framebuffer),     MP_ROM_PTR(&mp_lcd_bus_free_framebuffer_obj)     },
    { MP_ROM_QSTR(MP_QSTR_register_callback),    MP_ROM_PTR(&mp_lcd_bus_register_callback_obj)    },
    { MP_ROM_QSTR(MP_QSTR_tx_param),             MP_ROM_PTR(&mp_lcd_bus_tx_param_obj)             },
    { MP_ROM_QSTR(MP_QSTR_tx_color),             MP_ROM_PTR(&mp_lcd_bus_tx_color_obj)             },
    { MP_ROM_QSTR(MP_QSTR_rx_param),             MP_ROM_PTR(&mp_lcd_bus_rx_param_obj)             },
    { MP_ROM_QSTR(MP_QSTR_init),                 MP_ROM_PTR(&mp_lcd_bus_init_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_deinit),               MP_ROM_PTR(&mp_lcd_bus_deinit_obj)               },
    { MP_ROM_QSTR(MP_QSTR___del__),              MP_ROM_PTR(&mp_lcd_bus_deinit_obj)               },
};

STATIC MP_DEFINE_CONST_DICT(mp_lcd_spi_bus_locals_dict, mp_lcd_spi_bus_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_spi_bus_type,
    MP_QSTR_SPI_Bus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_spi_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_spi_bus_locals_dict
);
