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


#ifdef MP_PORT_UNIX
    static mp_obj_t mp_lcd_spi_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        LCD_UNUSED(type);
        LCD_UNUSED(n_args);
        LCD_UNUSED(n_kw);
        LCD_UNUSED(all_args);

        mp_raise_msg(&mp_type_NotImplementedError, MP_ERROR_TEXT("SPI display bus is not supported"));
        return mp_const_none;
    }

#else
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

    /* macros */
    #define CS_ON() { if (self->panel_io_config.cs_gpio != mp_const_none) mp_hal_pin_write((mp_hal_pin_obj_t)mp_hal_get_pin_obj(self->panel_io_config.cs_gpio), self->panel_io_config.flags.cs_high_active); }
    #define CS_OFF() { if (self->panel_io_config.cs_gpio != mp_const_none) mp_hal_pin_write((mp_hal_pin_obj_t)mp_hal_get_pin_obj(self->panel_io_config.cs_gpio), !self->panel_io_config.flags.cs_high_active); }
    #define DC_CMD() { mp_hal_pin_write(self->panel_io_config.dc_gpio, self->panel_io_config.flags.dc_low_on_data); }
    #define DC_DATA() { mp_hal_pin_write(self->panel_io_config.dc_gpio, !self->panel_io_config.flags.dc_low_on_data); }
    /* end macros */

    /* forward declarations */
    mp_lcd_err_t s_spi_del(mp_obj_t obj);
    mp_lcd_err_t s_spi_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
    mp_lcd_err_t s_spi_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
    mp_lcd_err_t s_spi_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t s_spi_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t s_spi_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update);

    void send_param_16(mp_lcd_spi_bus_obj_t *self, void *param, size_t param_size);
    void send_param_8(mp_lcd_spi_bus_obj_t *self, void *param, size_t param_size);
    void send_cmd_16(mp_lcd_spi_bus_obj_t *self, int lcd_cmd);
    void send_cmd_8(mp_lcd_spi_bus_obj_t *self, int lcd_cmd);
    /* end forward declarations */


    /* function definitions */
    static mp_obj_t mp_lcd_spi_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        enum {
            ARG_spi_bus,
            ARG_dc,
            ARG_freq,
            ARG_cs,
            ARG_dc_low_on_data,
            ARG_sio_mode,
            ARG_lsb_first,
            ARG_cs_high_active,
            ARG_spi_mode
        };

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

        self->spi_bus = (machine_hw_spi_device_obj_t *)MP_OBJ_TO_PTR(args[ARG_spi_bus].u_obj);

    #if !defined(IDF_VER)
        self->callback = mp_const_none;

        self->freq = (uint32_t)args[ARG_freq].u_int;

        if (args[ARG_lsb_first].u_bool) {
            self->firstbit = 1;
        } else {
            self->firstbit = 0;
        }

        self->callback = mp_const_none;

        self->host = (int)self->spi_bus->spi_bus->host;

        self->panel_io_config.cs_gpio = args[ARG_cs].u_obj;
        self->panel_io_config.dc_gpio = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_dc].u_obj);
        self->panel_io_config.flags.dc_low_on_data = args[ARG_dc_low_on_data].u_bool;
        self->panel_io_config.flags.lsb_first = args[ARG_lsb_first].u_bool;
        self->panel_io_config.flags.cs_high_active = args[ARG_cs_high_active].u_bool;

        mp_hal_pin_output(self->panel_io_config.dc_gpio);
        mp_hal_pin_write(self->panel_io_config.dc_gpio, !self->panel_io_config.flags.dc_low_on_data);

        if (self->panel_io_config.cs_gpio != mp_const_none) {
            mp_hal_pin_output(mp_hal_get_pin_obj(self->panel_io_config.cs_gpio));
            mp_hal_pin_write(mp_hal_get_pin_obj(self->panel_io_config.cs_gpio), !self->panel_io_config.flags.cs_high_active);
        }

        self->panel_io_handle.del = s_spi_del;
        self->panel_io_handle.init = s_spi_init;
        self->panel_io_handle.tx_param = s_spi_tx_param;
        self->panel_io_handle.rx_param = s_spi_rx_param;
        self->panel_io_handle.tx_color = s_spi_tx_color;
        self->panel_io_handle.get_lane_count = s_spi_get_lane_count;

    #endif /* !defined(IDF_VER) */

        return MP_OBJ_FROM_PTR(self);
    }


    mp_lcd_err_t s_spi_del(mp_obj_t obj)
    {
        LCD_UNUSED(obj);
        return LCD_OK;
    }


    mp_lcd_err_t s_spi_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits)
    {
        LCD_UNUSED(rgb565_byte_swap);

        mp_lcd_spi_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

        uint8_t bits;

        if (cmd_bits == 16) {
            self->send_cmd = send_cmd_16;
            bits = 16;
        } else {
            self->send_cmd = send_cmd_8;
            bits = 8;
        }

        if (param_bits == 16) {
            self->send_param = send_param_16;
            bits = 16;
        } else {
            self->send_param = send_param_8;
        }

        mp_obj_base_t *spi;
        mp_obj_t spi_args[12];


        spi_args[0] = mp_obj_new_int(self->host);
        spi_args[1] = mp_obj_new_int_from_uint(self->freq);
        spi_args[2] = MP_ROM_QSTR(MP_QSTR_bits);
        spi_args[3] = mp_obj_new_int_from_uint(bits);
        spi_args[4] = MP_ROM_QSTR(MP_QSTR_firstbit);
        spi_args[5] = mp_obj_new_int_from_uint(self->firstbit);
        spi_args[6] = MP_ROM_QSTR(MP_QSTR_sck);
        spi_args[7] = self->spi_bus->spi_bus->sck;
        spi_args[8] = MP_ROM_QSTR(MP_QSTR_mosi);
        spi_args[9] = self->spi_bus->spi_bus->mosi;
        spi_args[10] = MP_ROM_QSTR(MP_QSTR_miso);
        spi_args[11] = self->spi_bus->spi_bus->miso;


        spi = MP_OBJ_TO_PTR(MP_OBJ_TYPE_GET_SLOT(&machine_spi_type, make_new)((mp_obj_t)&machine_spi_type, 2, 5, spi_args));

        self->bus_handle = spi;
        self->panel_io_config.spi_transfer = ((mp_machine_spi_p_t *)MP_OBJ_TYPE_GET_SLOT(spi->type, protocol))->transfer;

        return LCD_OK;
    }


    mp_lcd_err_t s_spi_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
    {
        LCD_UNUSED(obj);
        *lane_count = 1;
        return LCD_OK;
    }


    mp_lcd_err_t s_spi_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        mp_lcd_spi_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

        CS_ON();

        self->send_cmd(self, lcd_cmd);
        self->panel_io_config.spi_transfer(
            self->bus_handle, param_size, NULL, (uint8_t *) param);

        CS_OFF();
        return LCD_OK;
    }


    mp_lcd_err_t s_spi_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        mp_lcd_spi_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

        CS_ON();
        self->send_cmd(self, lcd_cmd);

        if (param != NULL) {
            self->send_param(self, param, param_size);
        }

        CS_OFF();
        return LCD_OK;
    }


    mp_lcd_err_t s_spi_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update)
    {
        LCD_UNUSED(x_start);
        LCD_UNUSED(y_start);
        LCD_UNUSED(x_end);
        LCD_UNUSED(y_end);
        LCD_UNUSED(rotation);
        LCD_UNUSED(last_update);

        mp_lcd_spi_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

        CS_ON();
        if (lcd_cmd >= 0x00) {
            self->send_cmd(self, lcd_cmd);
        }

        send_param_8(self, color, color_size);

        CS_OFF();

        // we are calling the callback function manually if one has been supplied.
        // This is done so the user is able to do work that needs to be done after
        // a buffer has finished being transfered. Since we are not using DMA transfers
        // the call to send the spi data is going to be blocking so we need to manually
        // make this call. This will also stop the blocking that takes place if no
        // callback is supplied. With the ESP32 this function gets called by the
        // interfaces built into the esp-idf so we need to block until that transfer
        // has completed if no callback is supplied. Instead of changing all of the
        // mechanics of doing that it is just easier to call the function manually.
        bus_trans_done_cb(&self->panel_io_handle, NULL, self);

        return LCD_OK;
    }

    mp_obj_t s_spi_bus_get_host(mp_obj_t obj)
    {
        mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;
        return mp_obj_new_int(self->host);
    }

    MP_DEFINE_CONST_FUN_OBJ_1(s_spi_bus_get_host_obj, s_spi_bus_get_host);

    /* sending functions
     * These functions are here so a constant checking of the bit depth doesn't
     * need to be done. The functions gets stored in a structure according to the
     * bit depth the user has supplied.
    */
    void send_param_16(mp_lcd_spi_bus_obj_t *self, void *param, size_t param_size)
    {
        self->panel_io_config.spi_transfer(
            self->bus_handle, param_size * 2, (const uint8_t *) param, NULL
        );
    }


    void send_param_8(mp_lcd_spi_bus_obj_t *self, void *param, size_t param_size)
    {
        self->panel_io_config.spi_transfer(
            self->bus_handle, param_size, (const uint8_t *) param, NULL
        );
    }


    void send_cmd_16(mp_lcd_spi_bus_obj_t *self, int lcd_cmd)
    {
        DC_CMD();

        if (lcd_cmd >= 0x00) {
            self->panel_io_config.spi_transfer(
                self->bus_handle, 2,
                (const uint8_t []){ (uint8_t) ((lcd_cmd >> 8) & 0xFF), (uint8_t) (lcd_cmd & 0xFF) }, NULL
            );
        }

        DC_DATA();
    }


    void send_cmd_8(mp_lcd_spi_bus_obj_t *self, int lcd_cmd)
    {
        DC_CMD();

        if (lcd_cmd >= 0x00) {
            self->panel_io_config.spi_transfer(
                self->bus_handle, 1,
                (const uint8_t []){ (uint8_t) (lcd_cmd & 0xFF) }, NULL
            );
        }

        DC_DATA();

    }
    /* end sending functions */

    /* end function definitions */


#endif

static const mp_rom_map_elem_t mp_lcd_spi_bus_locals_dict_table[] = {
#ifndef MP_PORT_UNIX
    { MP_ROM_QSTR(MP_QSTR_get_host),             MP_ROM_PTR(&s_spi_bus_get_host_obj)             },
#endif
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

/* create micropython class */
/* create micropython class */
MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_spi_bus_type,
    MP_QSTR_SPI_Bus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_spi_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_spi_bus_locals_dict
);
/* end create micropython class */
