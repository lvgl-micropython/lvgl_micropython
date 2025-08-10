// Copyright (c) 2024 - 2025 Kevin G. Schlosser


/* includes */
// local includes
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "spi_bus.h"
#include "../../../micropy_updates/common/mp_spi_common.h"

// micropython includes
// #if defined(STM32_HAL_H) || defined(MICROPY_PY_NRF)
//     #include "spi.h"
// #else
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>
/* end includes */


#ifdef MICROPY_PY_NRF
   #include "modules/machine/spi.h"
#endif

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

#define CS_ON() { if (self->spi_device->cs != mp_const_none) mp_hal_pin_write(mp_hal_get_pin_obj(self->spi_device->cs), self->spi_device->cs_high_active); }
#define CS_OFF() { if (self->spi_device->cs != mp_const_none) mp_hal_pin_write(mp_hal_get_pin_obj(self->spi_device->cs), self->spi_device->cs_high_active ? 0 : 1); }
#define DC_CMD() { mp_hal_pin_write(self->dc, self->dc_low_on_data); }
#define DC_DATA() { mp_hal_pin_write(self->dc, self->dc_low_on_data ? 0: 1); }

static mp_lcd_err_t spi_del(mp_obj_t obj);
static mp_lcd_err_t spi_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
static mp_lcd_err_t spi_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
static mp_lcd_err_t spi_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
static mp_lcd_err_t spi_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
static mp_lcd_err_t spi_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update);

static void send_cmd_16(mp_lcd_spi_bus_obj_t *self, int lcd_cmd)
{
    if (lcd_cmd >= 0x00) {
        self->spi_transfer(self, 2,
                    (const uint8_t []){
                        (uint8_t)((lcd_cmd >> 8) & 0xFF),
                        (uint8_t)(lcd_cmd & 0xFF)
                    }, NULL);
    }
}


static void send_cmd_8(mp_lcd_spi_bus_obj_t *self, int lcd_cmd)
{
    if (lcd_cmd >= 0x00) {
        self->spi_transfer(self, 1,
                    (const uint8_t []){
                        (uint8_t)(lcd_cmd & 0xFF)
                    }, NULL);
    }
}


static void spi_dma_callback(mp_lcd_spi_bus_obj_t *self) {
    if (self->callback != mp_const_none) {
        mp_sched_schedule(self->callback, MP_OBJ_FROM_PTR(self));
    } else {
        self->trans_done = true;
    }

    CS_OFF();
    self->spi_device->spi_bus->state = MP_SPI_STATE_STARTED;
}

/* function definitions */
static mp_obj_t mp_lcd_spi_bus_make_new(const mp_obj_type_t *type, size_t n_args,
                                        size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_spi_bus, ARG_dc, ARG_freq, ARG_cs, ARG_dc_low_on_data, ARG_sio_mode,
           ARG_lsb_first, ARG_cs_high_active, ARG_spi_mode };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_spi_bus,          MP_ARG_OBJ  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_dc,               MP_ARG_OBJ  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_freq,             MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_cs,               MP_ARG_OBJ  | MP_ARG_KW_ONLY, { .u_obj = mp_const_none } },
        { MP_QSTR_dc_low_on_data,   MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_sio_mode,         MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_lsb_first,        MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_cs_high_active,   MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_spi_mode,         MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0        } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(make_new_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                              MP_ARRAY_SIZE(make_new_args), make_new_args, args);

    // create new object
    mp_lcd_spi_bus_obj_t *self = m_new_obj(mp_lcd_spi_bus_obj_t);
    self->base.type = &mp_lcd_spi_bus_type;

    mp_machine_hw_spi_bus_obj_t *spi_bus = MP_OBJ_TO_PTR(args[ARG_spi_bus].u_obj);

    self->callback = mp_const_none;
    self->spi_device = (machine_hw_spi_device_obj_t *)m_malloc(sizeof(machine_hw_spi_device_obj_t));

    self->dc = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_dc].u_obj);
    self->dc_low_on_data = args[ARG_dc_low_on_data].u_bool;

    mp_hal_pin_output(self->dc);
    mp_hal_pin_write(self->dc, self->dc_low_on_data ? 1 : 0);

    self->spi_device->base.type = &mp_machine_hw_spi_device_type;
    self->spi_device->active = 1;

    self->spi_device->freq = (uint32_t)args[ARG_freq].u_int;
    self->spi_device->polarity = ((uint8_t)args[ARG_spi_mode].u_int >> 1) & 0x1;
    self->spi_device->phase = (uint8_t)args[ARG_spi_mode].u_int & 0x1;
    self->spi_device->bits = 8;
    self->spi_device->firstbit = (uint8_t)args[ARG_lsb_first].u_bool;
    self->spi_device->cs = args[ARG_cs].u_obj;
    self->spi_device->cs_high_active = (uint8_t)args[ARG_cs_high_active].u_bool;
    self->spi_device->spi_bus = spi_bus;
    self->spi_device->mem_addr_size = 8;
    self->spi_device->deinit = &spi_deinit_callback;
    self->spi_device->user_data = self;

    if (self->spi_device->cs != mp_const_none) {
        mp_hal_pin_output(mp_hal_get_pin_obj(self->spi_device->cs));
        mp_hal_pin_write(mp_hal_get_pin_obj(self->spi_device->cs), self->spi_device->cs_high_active ? 0 : 1);
    }

    self->spi_transfer = ((mp_machine_spi_p_t *)MP_OBJ_TYPE_GET_SLOT(&mp_machine_hw_spi_device_obj_t, protocol))->transfer;

    self->dma_callback = &spi_dma_callback;
    self->panel_io_handle.del = s_spi_del;
    self->panel_io_handle.init = s_spi_init;
    self->panel_io_handle.tx_param = s_spi_tx_param;
    self->panel_io_handle.rx_param = s_spi_rx_param;
    self->panel_io_handle.tx_color = s_spi_tx_color;
    self->panel_io_handle.get_lane_count = s_spi_get_lane_count;

    return MP_OBJ_FROM_PTR(self);
}


mp_lcd_err_t spi_del(mp_obj_t obj)
{
    LCD_DEBUG_PRINT("spi_del(self)\n")

    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;

    if (self->spi_device != NULL) {

        if (self->view1 != NULL) {
            heap_caps_free(self->view1->items);
            self->view1->items = NULL;
            self->view1->len = 0;
            self->view1 = NULL;
            LCD_DEBUG_PRINT("spi_free_framebuffer(self, buf=1)\n")
        }

        if (self->view2 != NULL) {
            heap_caps_free(self->view2->items);
            self->view2->items = NULL;
            self->view2->len = 0;
            self->view2 = NULL;
            LCD_DEBUG_PRINT("spi_free_framebuffer(self, buf=1)\n")
        }

        mp_machine_hw_spi_bus_remove_device(self->spi_device);
        self->spi_device->active = false;

        if (self->spi_device->spi_bus->device_count == 0) {
            self->spi_device->spi_bus->deinit(self->spi_device->spi_bus);
        }

        m_free(self->spi_device);
        self->spi_device = NULL;

        return ret;
    } else {
        return LCD_FAIL;
    }
}


mp_lcd_err_t s_spi_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp,
                        uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits,
                        uint8_t param_bits)
{
    LCD_UNUSED(rgb565_byte_swap);
    LCD_UNUSED(buffer_size);
    LCD_UNUSED(bpp);
    LCD_UNUSED(width);
    LCD_UNUSED(height);
    LCD_UNUSED(param_bits);

    mp_lcd_spi_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

    if (cmd_bits == 16) {
        self->send_cmd = send_cmd_16;
    } else {
        self->send_cmd = send_cmd_8;
    }

    mp_machine_hw_spi_bus_add_device(self->spi_device);

    return LCD_OK;
}


mp_lcd_err_t spi_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
{
    LCD_UNUSED(obj);
    *lane_count = 1;
    return LCD_OK;
}


mp_lcd_err_t spi_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
{
    mp_lcd_spi_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

    while (self->spi_device->spi_bus->state == MP_SPI_STATE_SENDING) { }
    self->spi_device->spi_bus->state = MP_SPI_STATE_SENDING;

    CS_ON();
    DC_CMD();
    self->use_dma = 0;

    self->send_cmd(self, lcd_cmd);

    if (param != NULL) {
        self->spi_transfer(self, param_size, NULL, (uint8_t *)param);
    }

    CS_OFF();
    self->spi_device->spi_bus->state = MP_SPI_STATE_STARTED;


    return LCD_OK;
}


mp_lcd_err_t spi_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
{
    mp_lcd_spi_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

    while (self->spi_device->spi_bus->state == MP_SPI_STATE_SENDING) { }
    self->spi_device->spi_bus->state = MP_SPI_STATE_SENDING;

    CS_ON();
    DC_CMD();

    self->use_dma = 0;
    self->send_cmd(self->spi_device, lcd_cmd);

    if (param != NULL) {
        self->spi_transfer(self->spi_device, param_size, (const uint8_t *)param, NULL);
    }

    CS_OFF();
    self->spi_device->spi_bus->state = MP_SPI_STATE_STARTED;

    return LCD_OK;
}


mp_lcd_err_t spi_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update)
{
    LCD_UNUSED(x_start);
    LCD_UNUSED(y_start);
    LCD_UNUSED(x_end);
    LCD_UNUSED(y_end);
    LCD_UNUSED(rotation);
    LCD_UNUSED(last_update);

    mp_lcd_spi_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

    while (self->spi_device->spi_bus->state == MP_SPI_STATE_SENDING) { }
    self->spi_device->spi_bus->state = MP_SPI_STATE_SENDING;

    CS_ON();
    DC_CMD()

    self->use_dma = 0;
    if (lcd_cmd >= 0x00) {
        self->send_cmd((mp_obj_base_t *)self, lcd_cmd);
    }

    DC_DATA();
    self->use_dma = 1;
    self->spi_transfer((mp_obj_base_t *)self, color_size, (const uint8_t *)color, NULL);

    return LCD_OK;
}


static const mp_rom_map_elem_t mp_lcd_spi_bus_locals_dict_table[] = {
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
