// Copyright (c) 2024 - 2025 Kevin G. Schlosser

// local includes
#include "lcd_types.h"
#include "lcd_bus_task.h"
#include "modlcd_bus.h"
#include "spi_bus.h"
#include "bus_trans_done.h"
#include "../../../micropy_updates/common/mp_spi_common.h"

// esp-idf includes
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "soc/spi_pins.h"
#include "esp_lcd_panel_io.h"
#include "hal/spi_types.h"
#include "esp_task.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>


/*
typedef enum _mp_spi_state_t {
    MP_SPI_STATE_STOPPED,
    MP_SPI_STATE_STARTED,
    MP_SPI_STATE_SENDING
} mp_spi_state_t;

typedef struct _machine_hw_spi_bus_obj_t {
    uint8_t host;
    mp_obj_t sck;
    mp_obj_t mosi;
    mp_obj_t miso;
    int16_t active_devices;
    mp_spi_state_t state;
    void *user_data;
} machine_hw_spi_bus_obj_t;


typedef struct _machine_hw_spi_obj_t {
    mp_obj_base_t base;
    uint32_t baudrate;
    uint8_t polarity;
    uint8_t phase;
    uint8_t bits;
    uint8_t firstbit;
    mp_obj_t cs;
    machine_hw_spi_bus_obj_t *spi_bus;
    void *user_data;
} machine_hw_spi_obj_t;

*/


mp_lcd_err_t spi_del(mp_lcd_bus_obj_t *self_in);
mp_lcd_err_t spi_init(mp_lcd_bus_obj_t *self_in, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,
                      uint8_t cmd_bits, uint8_t param_bits);
void spi_deinit_callback(mp_machine_hw_spi_device_obj_t *device);


static void spi_send_func(mp_lcd_bus_obj_t *self_in, int cmd, uint8_t *params, size_t params_len)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)self_in;
    esp_lcd_panel_io_tx_param(self->panel_io_handle, cmd, params, params_len);

}


static void spi_flush_func(mp_lcd_bus_obj_t *self_in, rotation_data_t *r_data, rotation_data_t *original_r_data,
                           uint8_t *idle_fb, uint8_t last_update)
{

    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)self_in;
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

static bool spi_init_func(mp_lcd_bus_obj_t *self_in)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *) self_in;

    self->init.err = esp_lcd_new_panel_io_spi(self->bus_handle, &self->panel_io_config, &self->panel_io_handle);
    if (self->init.err != 0) {
        self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_spi)");
        return false;
    }

    return true;
}

static mp_obj_t mp_lcd_spi_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
     enum { ARG_spi_bus, ARG_dc, ARG_freq, ARG_cs, ARG_dc_low_on_data, ARG_lsb_first,
            ARG_cs_high_active, ARG_spi_mode, ARG_dual, ARG_quad, ARG_octal };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_spi_bus,          MP_ARG_OBJ  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_dc,               MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_freq,             MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_cs,               MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1       } },
        { MP_QSTR_dc_low_on_data,   MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_lsb_first,        MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_cs_high_active,   MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_spi_mode,         MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0        } },
        { MP_QSTR_dual,             MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_quad,             MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_octal,            MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(make_new_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(make_new_args), make_new_args, args);

    // create new object
    mp_lcd_spi_bus_obj_t *self = m_new_obj(mp_lcd_spi_bus_obj_t);
    self->base.type = &mp_lcd_spi_bus_type;

    mp_machine_hw_spi_bus_obj_t *spi_bus = MP_OBJ_TO_PTR(args[ARG_spi_bus].u_obj);

    self->callback = mp_const_none;

    self->host = (spi_host_device_t)spi_bus->host;
    self->panel_io_handle = NULL;
    self->bus_handle = (esp_lcd_spi_bus_handle_t)self->host;

    self->panel_io_config.cs_gpio_num = (int)args[ARG_cs].u_int;
    self->panel_io_config.dc_gpio_num = (int)args[ARG_dc].u_int;
    self->panel_io_config.spi_mode = (int)args[ARG_spi_mode].u_int;
    self->panel_io_config.pclk_hz = (unsigned int)args[ARG_freq].u_int;
    self->panel_io_config.on_color_trans_done = &bus_trans_done_cb;
    self->panel_io_config.user_ctx = self;
    self->panel_io_config.flags.dc_low_on_data = (unsigned int)args[ARG_dc_low_on_data].u_bool;
    self->panel_io_config.flags.lsb_first = (unsigned int)args[ARG_lsb_first].u_bool;
    self->panel_io_config.flags.cs_high_active = (unsigned int)args[ARG_cs_high_active].u_bool;
    self->panel_io_config.flags.sio_mode = (unsigned int)args[ARG_dual].u_bool;
    self->panel_io_config.flags.quad_mode = (unsigned int)args[ARG_quad].u_bool;
    self->panel_io_config.flags.octal_mode = (unsigned int)args[ARG_octal].u_bool;

    if (!spi_bus->dual) self->panel_io_config.flags.sio_mode = 0;
    if (!spi_bus->quad) self->panel_io_config.flags.quad_mode = 0;
    if (!spi_bus->octal) self->panel_io_config.flags.octal_mode = 0;

    if (self->panel_io_config.flags.sio_mode) self->num_lanes = 2;
    else if (self->panel_io_config.flags.quad_mode) self->num_lanes = 4;
    else if (self->panel_io_config.flags.octal_mode) self->num_lanes = 8;
    else self->num_lanes = 1;

    self->spi_device.active = true;
    self->spi_device.base.type = &mp_machine_hw_spi_device_type;
    self->spi_device.spi_bus = spi_bus;
    self->spi_device.deinit = &spi_deinit_callback;
    self->spi_device.user_data = self;

    self->tx_cmds.send_func = &spi_send_func;
    self->tx_data.flush_func = &spi_flush_func;
    self->init.init_func = &spi_init_func;

    self->internal_cb_funcs.init = &spi_init;
    self->internal_cb_funcs.deinit = &spi_del;

    LCD_DEBUG_PRINT("host=%d\n", self->host)
    LCD_DEBUG_PRINT("cs_gpio_num=%d\n", self->panel_io_config.cs_gpio_num)
    LCD_DEBUG_PRINT("dc_gpio_num=%d\n", self->panel_io_config.dc_gpio_num)
    LCD_DEBUG_PRINT("spi_mode=%d\n", self->panel_io_config.spi_mode)
    LCD_DEBUG_PRINT("pclk_hz=%i\n", self->panel_io_config.pclk_hz)
    LCD_DEBUG_PRINT("dc_low_on_data=%d\n", self->panel_io_config.flags.dc_low_on_data)
    LCD_DEBUG_PRINT("lsb_first=%d\n", self->panel_io_config.flags.lsb_first)
    LCD_DEBUG_PRINT("cs_high_active=%d\n", self->panel_io_config.flags.cs_high_active)
    LCD_DEBUG_PRINT("dual=%d\n", self->panel_io_config.flags.sio_mode)
    LCD_DEBUG_PRINT("quad=%d\n", self->panel_io_config.flags.quad_mode)
    LCD_DEBUG_PRINT("octal=%d\n", self->panel_io_config.flags.octal_mode)

    return MP_OBJ_FROM_PTR(self);
}




mp_lcd_err_t spi_del(mp_lcd_bus_obj_t *self_in)
{
    LCD_DEBUG_PRINT("spi_del(self)\n")

    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)self_in;

    if (self->panel_io_handle != NULL) {
        mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle);
        if (ret != ESP_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
            return ret;
        }

        self->panel_io_handle = NULL;

        mp_machine_hw_spi_bus_remove_device(&self->spi_device);
        self->spi_device.active = false;

        if (self->spi_device.spi_bus->device_count == 0) {
            self->spi_device.spi_bus->deinit(self->spi_device.spi_bus);
        }

        return ret;
    } else {
        return LCD_FAIL;
    }
}

void spi_deinit_callback(mp_machine_hw_spi_device_obj_t *device)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)device->user_data;
    spi_del(MP_OBJ_FROM_PTR(self));
}


mp_lcd_err_t spi_init(mp_lcd_bus_obj_t *self_in, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,
                      uint8_t cmd_bits, uint8_t param_bits)
{
    LCD_DEBUG_PRINT("spi_init(self, width=%i, height=%i, bpp=%i, buffer_size=%lu, cmd_bits=%i, param_bits=%i)\n",
                    width, height, bpp, buffer_size, cmd_bits, param_bits)
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)self_in;

    if (self->panel_io_handle != NULL) {
        return LCD_FAIL;
    }

    if (self->spi_device.spi_bus->state == MP_SPI_STATE_STOPPED) {
        mp_machine_hw_spi_bus_initilize(self->spi_device.spi_bus);
    }

    self->panel_io_config.trans_queue_depth = 10;
    self->panel_io_config.lcd_cmd_bits = (int)cmd_bits;
    self->panel_io_config.lcd_param_bits = (int)param_bits;

    LCD_DEBUG_PRINT("lcd_cmd_bits=%d\n", self->panel_io_config.lcd_cmd_bits)
    LCD_DEBUG_PRINT("lcd_param_bits=%d\n", self->panel_io_config.lcd_param_bits)
    LCD_DEBUG_PRINT("rgb565_byte_swap=%i\n",  (uint8_t)self->r_data.swap)
    LCD_DEBUG_PRINT("trans_queue_depth=%i\n", (uint8_t)self->panel_io_config.trans_queue_depth)

    xTaskCreatePinnedToCore(
                lcd_bus_task, "spi_task", LCD_DEFAULT_STACK_SIZE / sizeof(StackType_t),
                self, ESP_TASK_PRIO_MAX - 1, &self->task.handle, 0);

    lcd_bus_lock_acquire(self->init.lock);
    lcd_bus_lock_release(self->init.lock);
    lcd_bus_lock_delete(self->init.lock);

    if (self->init.err == LCD_OK) {
        mp_machine_hw_spi_bus_add_device(&self->spi_device);
    }
    return self->init.err;
}


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_spi_bus_type,
    MP_QSTR_SPI_Bus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_spi_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
);