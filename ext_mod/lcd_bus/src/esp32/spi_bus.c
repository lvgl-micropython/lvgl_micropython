// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"
#include "py/runtime.h"

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


#include "common/lcd_common_types.h"
#include "common/sw_rotate.h"
#include "common/sw_rotate_task_common.h"
#include "common/modlcd_bus.h"

#include "lcd_types.h"
#include "sw_rotate_task.h"
#include "spi_bus.h"

#include "../../../../micropy_updates/common/mp_spi_common.h"


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


mp_lcd_err_t spi_del(mp_obj_t obj);
mp_lcd_err_t spi_init(mp_obj_t obj, uint8_t cmd_bits, uint8_t param_bits);
void spi_deinit_callback(mp_machine_hw_spi_device_obj_t *device);



static void spi_tx_param_cb(void* self_in, int cmd, uint8_t *params, size_t params_len)
{
    esp_lcd_panel_io_tx_param(self->panel_io_handle.panel_io, cmd, params, params_len);
}


static bool spi_trans_done_cb(esp_lcd_panel_handle_t panel,
                        const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)user_ctx;

    if (self->trans_done == 0) {
        if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
            mp_lcd_flush_ready_cb(self->callback);
        }
        self->trans_done = 1;
    }

    return false;
}


static bool spi_init_cb(void *self_in)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)self_in;

    self->sw_rot.init.err = esp_lcd_new_panel_io_spi(self->bus_handle, self->panel_io_config, &self->panel_io_handle.panel_io);

    if (self->sw_rot.init.err != 0) {
        self->sw_rot.init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_spi)");
        return false;
    }

    free(self->panel_io_config);
    self->panel_io_config = NULL;

    return true;
}


static void spi_flush_cb(void *self_in, uint8_t last_update, int cmd, uint8_t *idle_fb)
{
    LCD_UNUSED(last_update);
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)self_in;

    if (idle_fb == self->sw_rot.buffers.idle) {
        self->sw_rot.buffers.idle = self->sw_rot.buffers.active;
        self->sw_rot.buffers.active = idle_fb;
    }

    mp_lcd_err_t ret = esp_lcd_panel_io_tx_color(self->panel_io_handle.panel_io, cmd, idle_fb, self->fb1->len);

    if (ret != 0) {
        mp_printf(&mp_plat_print, "esp_lcd_panel_draw_bitmap error (%d)\n", ret);
    }
}



static mp_obj_t mp_lcd_spi_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
     enum {
        ARG_spi_bus,
        ARG_dc,
        ARG_freq,
        ARG_cs,
        ARG_dc_low_on_data,
        ARG_lsb_first,
        ARG_cs_high_active,
        ARG_spi_mode,
        ARG_dual,
        ARG_quad,
        ARG_octal
    };

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

    mp_machine_hw_spi_bus_obj_t *spi_bus = MP_OBJ_TO_PTR(args[ARG_spi_bus].u_obj);

    self->panel_io_config = malloc(sizeof(esp_lcd_panel_io_spi_config_t));
    esp_lcd_panel_io_spi_config_t *panel_io_config = self->panel_io_config;

    self->bus_config = malloc(sizeof(spi_bus_config_t));
    spi_bus_config_t *bus_config = self->bus_config;

    self->callback = mp_const_none;

    self->host = (spi_host_device_t)spi_bus->host;
    self->panel_io_handle.panel_io = NULL;
    self->bus_handle = (esp_lcd_spi_bus_handle_t)self->host;

    panel_io_config->cs_gpio_num = (int)args[ARG_cs].u_int;
    panel_io_config->dc_gpio_num = (int)args[ARG_dc].u_int;
    panel_io_config->spi_mode = (int)args[ARG_spi_mode].u_int;
    panel_io_config->pclk_hz = (unsigned int)args[ARG_freq].u_int;
    panel_io_config->on_color_trans_done = &bus_trans_done_cb;
    panel_io_config->user_ctx = self;
    panel_io_config->flags.dc_low_on_data = (unsigned int)args[ARG_dc_low_on_data].u_bool;
    panel_io_config->flags.lsb_first = (unsigned int)args[ARG_lsb_first].u_bool;
    panel_io_config->flags.cs_high_active = (unsigned int)args[ARG_cs_high_active].u_bool;
    panel_io_config->flags.sio_mode = (unsigned int)args[ARG_dual].u_bool;
    panel_io_config->flags.quad_mode = (unsigned int)args[ARG_quad].u_bool;
    panel_io_config->flags.octal_mode = (unsigned int)args[ARG_octal].u_bool;

    if (!spi_bus->dual) panel_io_config->flags.sio_mode = 0;
    if (!spi_bus->quad) panel_io_config->flags.quad_mode = 0;
    if (!spi_bus->octal) panel_io_config->flags.octal_mode = 0;

    if (panel_io_config->flags.sio_mode) self->lanes = 2;
    else if (panel_io_config->flags.quad_mode) self->lanes = 4;
    else if (panel_io_config->flags.octal_mode) self->lanes = 8;
    else self->lanes = 1;

    self->spi_device.active = true;
    self->spi_device.base.type = &mp_machine_hw_spi_device_type;
    self->spi_device.spi_bus = spi_bus;
    self->spi_device.deinit = &spi_deinit_callback;
    self->spi_device.user_data = self;

    LCD_DEBUG_PRINT("host=%d\n", self->host)
    LCD_DEBUG_PRINT("cs_gpio_num=%d\n", panel_io_config->cs_gpio_num)
    LCD_DEBUG_PRINT("dc_gpio_num=%d\n", panel_io_config->dc_gpio_num)
    LCD_DEBUG_PRINT("spi_mode=%d\n", panel_io_config->spi_mode)
    LCD_DEBUG_PRINT("pclk_hz=%i\n", panel_io_config->pclk_hz)
    LCD_DEBUG_PRINT("dc_low_on_data=%d\n", panel_io_config->flags.dc_low_on_data)
    LCD_DEBUG_PRINT("lsb_first=%d\n", panel_io_config->flags.lsb_first)
    LCD_DEBUG_PRINT("cs_high_active=%d\n", panel_io_config->flags.cs_high_active)
    LCD_DEBUG_PRINT("dual=%d\n", panel_io_config->flags.sio_mode)
    LCD_DEBUG_PRINT("quad=%d\n", panel_io_config->flags.quad_mode)
    LCD_DEBUG_PRINT("octal=%d\n", panel_io_config->flags.octal_mode)

    self->panel_io_handle.del = &spi_del;
    self->panel_io_handle.init = &spi_init;

    return MP_OBJ_FROM_PTR(self);
}


void spi_deinit_callback(mp_machine_hw_spi_device_obj_t *device)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)device->user_data;
    spi_del(MP_OBJ_FROM_PTR(self));
}


mp_lcd_err_t spi_del(mp_obj_t obj)
{
    LCD_DEBUG_PRINT("spi_del(self)\n")

    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;

    if (self->panel_io_handle.panel_io != NULL) {
        mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle.panel_io);
        if (ret != ESP_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
            return ret;
        }

        self->panel_io_handle.panel_io = NULL;

        uint8_t i= 0;
        for (;i<spi_bus_count;i++) {
            if (spi_bus_objs[i] == self) {
                spi_bus_objs[i] = NULL;
                break;
            }
        }

        for (uint8_t j=i + 1;j<spi_bus_count;j++) {
            spi_bus_objs[j - i + 1] = spi_bus_objs[j];
        }

        spi_bus_count--;
        spi_bus_objs = m_realloc(spi_bus_objs, spi_bus_count * sizeof(mp_lcd_spi_bus_obj_t *));

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



mp_lcd_err_t spi_init(mp_obj_t obj, uint8_t cmd_bits, uint8_t param_bits)
{
    LCD_DEBUG_PRINT("spi_init(self, cmd_bits=%i, param_bits=%i)\n", cmd_bits, param_bits)
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)MP_OBJ_TO_PTR(obj);

    if (self->panel_io_handle.panel_io != NULL) {
        return LCD_FAIL;
    }

    if (self->spi_device.spi_bus->state == MP_SPI_STATE_STOPPED) {
        mp_machine_hw_spi_bus_initilize(self->spi_device.spi_bus);
    }

    if (self->sw_rot.data.bytes_per_pixel != 2) {
        self->sw_rot.data.rgb565_swap = false;
    }

    self->panel_io_config->trans_queue_depth = 10;
    self->panel_io_config->lcd_cmd_bits = (int)cmd_bits;
    self->panel_io_config->lcd_param_bits = (int)param_bits;

    LCD_DEBUG_PRINT("lcd_cmd_bits=%d\n", self->panel_io_config->lcd_cmd_bits)
    LCD_DEBUG_PRINT("lcd_param_bits=%d\n", self->panel_io_config->lcd_param_bits)
    LCD_DEBUG_PRINT("trans_queue_depth=%i\n", (uint8_t)self->panel_io_config->trans_queue_depth)

    self->sw_rot.data.dst_width = 0;
    self->sw_rot.data.dst_height = 0;
    self->sw_rot.init.cb = &spi_init_cb;
    self->sw_rot.flush_cb = &spi_flush_cb;
    self->sw_rot.tx_params.cb = &spi_tx_param_cb;

    mp_lcd_lock_init(&self->sw_rot.handles.copy_lock);
    mp_lcd_lock_init(&self->sw_rot.handles.tx_color_lock);
    mp_lcd_event_init(&self->sw_rot.handles.copy_task_exit);
    mp_lcd_event_init(&self->sw_rot.handles.swap_bufs);
    mp_lcd_event_set(&self->sw_rot.handles.swap_bufs);
    mp_lcd_lock_init(&self->sw_rot.handles.init_lock);
    mp_lcd_lock_acquire(&self->sw_rot.handles.init_lock);
    mp_lcd_lock_init(&self->sw_rot.tx_params.lock);
    self->sw_rot.tx_params.len = 0;

    mp_machine_hw_spi_bus_add_device(&self->spi_device);

    return LCD_OK;
}


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_spi_bus_type,
    MP_QSTR_SPI_Bus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_spi_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
);
