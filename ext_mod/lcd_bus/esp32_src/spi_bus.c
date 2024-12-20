
// local includes
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "spi_bus.h"
#include "rotation.h"
#include "bus_task.h"
#include "../../../micropy_updates/common/mp_spi_common.h"

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
mp_lcd_err_t spi_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits, bool sw_rotate);
mp_lcd_err_t spi_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
mp_lcd_err_t spi_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size, bool is_flush, bool last_flush_cmd);
void spi_deinit_callback(machine_hw_spi_device_obj_t *device);


static bool spi_bus_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    LCD_UNUSED(panel_io);

    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)user_ctx;
    bus_event_set_from_isr(&self->rotation->task.swap_bufs);
    return false;
}


static mp_lcd_err_t spi_rotation_init_func(void *self_in)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)self_in;

    self->panel_io_config.on_color_trans_done = &spi_bus_trans_done_cb;

    rotation_t *rotation = self->rotation;
    rotation_init_err_t *init_err = &rotation->init_err;
    rotation_task_t *task = &rotation->task;

    init_err->code = esp_lcd_new_panel_io_spi(self->bus_handle, &self->panel_io_config, &self->panel_io_handle.panel_io);
    if (init_err->code != LCD_OK) {
        init_err->err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_spi)");
        bus_lock_release(&task->init_lock);
    } else {
        machine_hw_spi_bus_add_device(&self->spi_device);
    }

    return init_err->code;
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

    machine_hw_spi_bus_obj_t *spi_bus = MP_OBJ_TO_PTR(args[ARG_spi_bus].u_obj);

    self->callback = mp_const_none;

    self->host = (spi_host_device_t)spi_bus->host;
    self->panel_io_handle.panel_io = NULL;
    self->bus_handle = (esp_lcd_spi_bus_handle_t)self->host;

    self->panel_io_config.cs_gpio_num = (int)args[ARG_cs].u_int;
    self->panel_io_config.dc_gpio_num = (int)args[ARG_dc].u_int;
    self->panel_io_config.spi_mode = (int)args[ARG_spi_mode].u_int;
    self->panel_io_config.pclk_hz = (unsigned int)args[ARG_freq].u_int;
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

    self->panel_io_handle.del = &spi_del;
    self->panel_io_handle.init = &spi_init;
    self->panel_io_handle.get_lane_count = &spi_get_lane_count;
    self->panel_io_handle.tx_param = &spi_tx_param;

    self->spi_device.active = true;
    self->spi_device.base.type = &machine_hw_spi_device_type;
    self->spi_device.spi_bus = spi_bus;
    self->spi_device.deinit = &spi_deinit_callback;
    self->spi_device.user_data = self;

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

void spi_deinit_callback(machine_hw_spi_device_obj_t *device)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)device->user_data;
    spi_del(MP_OBJ_FROM_PTR(self));
}


mp_lcd_err_t spi_del(mp_obj_t obj)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;
    LCD_DEBUG_PRINT("spi_del(self)\n")

    if (!self->spi_device.active) return ESP_OK;

    mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle.panel_io);
    if (ret != ESP_OK) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
    }

    machine_hw_spi_bus_remove_device(&self->spi_device);
    self->spi_device.active = false;

    if (self->spi_device.spi_bus->device_count == 0) {
        self->spi_device.spi_bus->deinit(self->spi_device.spi_bus);
    }

    return ret;
}


mp_lcd_err_t spi_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits, bool sw_rotate)
{
    LCD_DEBUG_PRINT("spi_init(self, width=%i, height=%i, bpp=%i, buffer_size=%lu, rgb565_byte_swap=%i, cmd_bits=%i, param_bits=%i)\n", width, height, bpp, buffer_size, (uint8_t)rgb565_byte_swap, cmd_bits, param_bits)
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;

    if (self->spi_device.spi_bus->state == MP_SPI_STATE_STOPPED) {
        machine_hw_spi_bus_initilize(self->spi_device.spi_bus);
    }

    if (bpp == 16) {
        self->rgb565_byte_swap = rgb565_byte_swap;
    } else {
        self->rgb565_byte_swap = false;
    }

    self->panel_io_config.trans_queue_depth = 10;
    self->panel_io_config.lcd_cmd_bits = (int)cmd_bits;
    self->panel_io_config.lcd_param_bits = (int)param_bits;

    LCD_DEBUG_PRINT("lcd_cmd_bits=%d\n", self->panel_io_config.lcd_cmd_bits)
    LCD_DEBUG_PRINT("lcd_param_bits=%d\n", self->panel_io_config.lcd_param_bits)
    LCD_DEBUG_PRINT("rgb565_byte_swap=%i\n",  (uint8_t)self->rgb565_byte_swap)
    LCD_DEBUG_PRINT("trans_queue_depth=%i\n", (uint8_t)self->panel_io_config.trans_queue_depth)

    esp_err_t ret;

    if (sw_rotation) {
        self->rotation = (rotation_t *)malloc(sizeof(rotation_t));

        self->rotation->init_func = &spi_rotation_init_func;

        rotation_task_start(self);
        ret = self->rotation->init_err.code;

        if (ret != LCD_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, self->rotation->init_err.msg, self->rotation->init_err.code);
        }

    } else {
        self->panel_io_config.on_color_trans_done = &bus_trans_done_cb;

        ret = esp_lcd_new_panel_io_spi(self->bus_handle, &self->panel_io_config, &self->panel_io_handle.panel_io);
        if (ret != LCD_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_spi)"), ret);
        }

        machine_hw_spi_bus_add_device(&self->spi_device);
    }

    return ret;
}


mp_lcd_err_t spi_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size, bool is_flush, bool last_flush_cmd)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;

    mp_lcd_err_t ret;

    if (self->rotation == NULL || !is_flush) {
        LCD_UNUSED(last_flush_cmd);
        ret = esp_lcd_panel_io_tx_param(self->panel_io_handle.panel_io, lcd_cmd, param, param_size);
    } else {
        bus_lock_acquire(&self->rotation->task.tx_param_lock);

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


mp_lcd_err_t spi_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;

    if (self->panel_io_config.flags.sio_mode) {
        *lane_count = 2;
    } else if (self->panel_io_config.flags.quad_mode) {
        *lane_count = 4;
    } else if (self->panel_io_config.flags.octal_mode) {
        *lane_count = 8;
    } else {
        *lane_count = 1;
    }

    LCD_DEBUG_PRINT("spi_get_lane_count(self) -> %i\n", (uint8_t)(*lane_count))

    return LCD_OK;
}


mp_obj_t mp_spi_bus_get_host(mp_obj_t obj)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;

    LCD_DEBUG_PRINT("mp_spi_bus_get_host(self) -> %i\n", (uint8_t)self->host)
    return mp_obj_new_int((uint8_t)self->host);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_spi_bus_get_host_obj, mp_spi_bus_get_host);


static const mp_rom_map_elem_t mp_lcd_spi_bus_locals_dict_table[] = {
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

static MP_DEFINE_CONST_DICT(mp_lcd_spi_bus_locals_dict, mp_lcd_spi_bus_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_spi_bus_type,
    MP_QSTR_SPI_Bus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_spi_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_spi_bus_locals_dict
);
