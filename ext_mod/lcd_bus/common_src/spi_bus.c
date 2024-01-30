/* includes */
// local includes
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "spi_bus.h"

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


/* macros */
#define CS_ON() { if (self->panel_io_config.cs_gpio_num) mp_hal_pin_write(self->panel_io_config.cs_gpio_num, self->panel_io_config.flags.cs_high_active); }
#define CS_OFF() { if (self->panel_io_config.cs_gpio_num) mp_hal_pin_write(self->panel_io_config.cs_gpio_num, !self->panel_io_config.flags.cs_high_active); }
#define DC_CMD() { mp_hal_pin_write(self->panel_io_config.dc_gpio_num, self->panel_io_config.flags.dc_low_on_data); }
#define DC_DATA() { mp_hal_pin_write(self->panel_io_config.dc_gpio_num, !self->panel_io_config.flags.dc_low_on_data); }
/* end macros */

/* forward declarations */
mp_lcd_err_t s_spi_del(lcd_panel_io_t *io);
mp_lcd_err_t s_spi_init(lcd_panel_io_t *io, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size);
mp_lcd_err_t s_spi_get_lane_count(lcd_panel_io_t *io, uint8_t *lane_count);
mp_lcd_err_t s_spi_rx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size);
mp_lcd_err_t s_spi_tx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size);
mp_lcd_err_t s_spi_tx_color(lcd_panel_io_t *io, int lcd_cmd, void *color, size_t color_size);

void send_param_16(mp_lcd_spi_bus_obj_t *self, void *param, size_t param_size);
void send_param_8(mp_lcd_spi_bus_obj_t *self, void *param, size_t param_size);
void send_cmd_16(mp_lcd_spi_bus_obj_t *self, int lcd_cmd);
void send_cmd_8(mp_lcd_spi_bus_obj_t *self, int lcd_cmd);
/* end forward declarations */


/* function definitions */
STATIC mp_obj_t mp_lcd_spi_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum {
        ARG_dc,
        ARG_host,
        ARG_sclk,
        ARG_freq,
        ARG_mosi,
        ARG_miso,
        ARG_cs,
        ARG_wp,
        ARG_hd,
        ARG_quad_spi,
        ARG_tx_only,
        ARG_cmd_bits,
        ARG_param_bits,
        ARG_dc_low_on_data,
        ARG_sio_mode,
        ARG_lsb_first,
        ARG_cs_high_active,
        ARG_spi_mode
    };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_dc,               MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_host,             MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_sclk,             MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_freq,             MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_mosi,             MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_miso,             MP_ARG_OBJ  | MP_ARG_KW_ONLY, { .u_obj = mp_const_none } },
        { MP_QSTR_cs,               MP_ARG_OBJ  | MP_ARG_KW_ONLY, { .u_obj = mp_const_none } },
        { MP_QSTR_wp,               MP_ARG_OBJ  | MP_ARG_KW_ONLY, { .u_obj = mp_const_none } },
        { MP_QSTR_hd,               MP_ARG_OBJ  | MP_ARG_KW_ONLY, { .u_obj = mp_const_none } },
        { MP_QSTR_quad_spi,         MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_tx_only,          MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_cmd_bits,         MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 8        } },
        { MP_QSTR_param_bits,       MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 8        } },
        { MP_QSTR_dc_low_on_data,   MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_sio_mode,         MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_lsb_first,        MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_cs_high_active,   MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_spi_mode,         MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0        } }
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

    #if !defined(mp_hal_pin_output) && !defined(IDF_VER)
        mp_raise_msg(&mp_type_NotImplementedError, MP_ERROR_TEXT("LCD SPI but is not available for this MCU"));
    #else
        self->callback = mp_const_none;

        mp_obj_base_t *spi;
        mp_obj_t spi_args[10];

        spi_args[1] = MP_OBJ_NEW_SMALL_INT(args[ARG_freq].u_int);
        uint8_t firstbit;

        if (args[ARG_lsb_first].u_bool) {
            #ifdef MICROPY_PY_MACHINE_SPI_LSB
                firstbit = MICROPY_PY_MACHINE_SPI_LSB;
            #else
                firstbit = 1;
            #endif
        } else {
            #ifdef MICROPY_PY_MACHINE_SPI_MSB
                firstbit = MICROPY_PY_MACHINE_SPI_MSB;
            #else
                firstbit = 0;
            #endif
        }

        spi_args[0] = MP_OBJ_NEW_SMALL_INT(args[ARG_host].u_int);
        spi_args[1] = MP_OBJ_NEW_SMALL_INT(args[ARG_freq].u_int);
        spi_args[2] = MP_ROM_QSTR(MP_QSTR_firstbit);
        spi_args[3] = MP_OBJ_NEW_SMALL_INT(firstbit);
        spi_args[4] = MP_ROM_QSTR(MP_QSTR_sck);
        spi_args[5] = MP_OBJ_FROM_PTR(mp_hal_get_pin_obj(args[ARG_sclk].u_obj));
        spi_args[6] = MP_ROM_QSTR(MP_QSTR_mosi);
        spi_args[7] = MP_OBJ_FROM_PTR(mp_hal_get_pin_obj(args[ARG_mosi].u_obj));

        if (args[ARG_miso].u_obj != mp_const_none) {
            spi_args[8] = MP_ROM_QSTR(MP_QSTR_miso);
            spi_args[9] = MP_OBJ_FROM_PTR(mp_hal_get_pin_obj(args[ARG_miso].u_obj));
            spi = MP_OBJ_TO_PTR(MP_OBJ_TYPE_GET_SLOT(&machine_spi_type, make_new)((mp_obj_t)&machine_spi_type, 2, 4, spi_args));
        } else {
            spi = MP_OBJ_TO_PTR(MP_OBJ_TYPE_GET_SLOT(&machine_spi_type, make_new)((mp_obj_t)&machine_spi_type, 2, 3, spi_args));
        }

        self->callback = mp_const_none;

        self->bus_handle = spi;

        if (args[ARG_cs].u_obj != mp_const_none) {
            self->panel_io_config.cs_gpio_num = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_cs].u_obj);
        }
        self->panel_io_config.dc_gpio_num = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_dc].u_obj);
        self->panel_io_config.lcd_cmd_bits = args[ARG_cmd_bits].u_int;
        self->panel_io_config.lcd_param_bits = args[ARG_param_bits].u_int;
        self->panel_io_config.flags.dc_low_on_data = args[ARG_dc_low_on_data].u_bool;
        self->panel_io_config.flags.lsb_first = args[ARG_lsb_first].u_bool;
        self->panel_io_config.flags.cs_high_active = args[ARG_cs_high_active].u_bool;

        mp_hal_pin_output(self->panel_io_config.dc_gpio_num);
        mp_hal_pin_write(self->panel_io_config.dc_gpio_num, !self->panel_io_config.flags.dc_low_on_data);

        if (self->panel_io_config.cs_gpio_num) {
            mp_hal_pin_output(self->panel_io_config.cs_gpio_num);
            mp_hal_pin_write(self->panel_io_config.cs_gpio_num, !self->panel_io_config.flags.cs_high_active);
        }

        self->panel_io_handle.del = s_spi_del;
        self->panel_io_handle.init = s_spi_init;
        self->panel_io_handle.tx_param = s_spi_tx_param;
        self->panel_io_handle.rx_param = s_spi_rx_param;
        self->panel_io_handle.tx_color = s_spi_tx_color;
        self->panel_io_handle.get_lane_count = s_spi_get_lane_count;

        self->panel_io_config.spi_transfer = ((mp_machine_spi_p_t *)MP_OBJ_TYPE_GET_SLOT(spi->type, protocol))->transfer;
    #endif /* !defined(mp_hal_pin_output) && !defined(IDF_VER) */

    return MP_OBJ_FROM_PTR(self);
}


mp_lcd_err_t s_spi_del(lcd_panel_io_t *io)
{
    mp_lcd_spi_bus_obj_t *self = __containerof(io, mp_lcd_spi_bus_obj_t, panel_io_handle);
    LCD_UNUSED(self);
    return LCD_OK;
}


mp_lcd_err_t s_spi_init(lcd_panel_io_t *io, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size)
{
    mp_lcd_spi_bus_obj_t *self = __containerof(io, mp_lcd_spi_bus_obj_t, panel_io_handle);

    if (self->panel_io_config.lcd_cmd_bits == 16) {
        self->send_cmd = send_cmd_16;
    } else {
        self->send_cmd = send_cmd_8;
    }

    if (self->panel_io_config.lcd_param_bits == 16) {
        self->send_param = send_param_16;
    } else {
        self->send_param = send_param_8;
    }

    return LCD_OK;
}


mp_lcd_err_t s_spi_get_lane_count(lcd_panel_io_t *io, uint8_t *lane_count)
{
    LCD_UNUSED(io);
    *lane_count = 1;
    return LCD_OK;
}


mp_lcd_err_t s_spi_rx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size)
{
    mp_lcd_spi_bus_obj_t *self = __containerof(io, mp_lcd_spi_bus_obj_t, panel_io_handle);

    CS_ON();

    self->send_cmd(self, lcd_cmd);

    self->panel_io_config.spi_transfer(
        self->bus_handle, param_size, NULL, (uint8_t *) param);

    CS_OFF();
    return LCD_OK;

}


mp_lcd_err_t s_spi_tx_param(lcd_panel_io_t *io, int lcd_cmd, void *param, size_t param_size)
{
    mp_lcd_spi_bus_obj_t *self = __containerof(io, mp_lcd_spi_bus_obj_t, panel_io_handle);

    CS_ON();
    self->send_cmd(self, lcd_cmd);

    if (param != NULL) {
        self->send_param(self, param, param_size);
    }

    CS_OFF();
    return LCD_OK;
}


mp_lcd_err_t s_spi_tx_color(lcd_panel_io_t *io, int lcd_cmd, void *color, size_t color_size)
{
    mp_lcd_spi_bus_obj_t *self = __containerof(io, mp_lcd_spi_bus_obj_t, panel_io_handle);

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

/* create micropython class */
MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_spi_bus_type,
    MP_QSTR_SPI_Bus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_spi_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
);
/* end create micropython class */
