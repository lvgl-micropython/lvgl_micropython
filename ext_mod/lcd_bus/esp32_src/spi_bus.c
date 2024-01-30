
// local includes
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "spi_bus.h"

// esp-idf includes
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "soc/gpio_sig_map.h"
#include "soc/spi_pins.h"
#include "rom/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_heap_caps.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>


mp_lcd_err_t spi_del(lcd_panel_io_t *io);
mp_lcd_err_t spi_init(lcd_panel_io_t *io, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size);
mp_lcd_err_t spi_get_lane_count(lcd_panel_io_t *io, uint8_t *lane_count);


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

    /*
     * The required parameters are required to keep the API consistant.
     * This is done for portability reasons.
     * You can set the required values in a manner that will ttrigger
     * an automatic setup procedure
     * host: -1 if mosi != -1 and sclk != -1
     * mosi -1 & sclk -1: if host != -1
     * freq: -1
     */

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_dc,               MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_host,             MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_sclk,             MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_freq,             MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_mosi,             MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_miso,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1       } },
        { MP_QSTR_cs,               MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1       } },
        { MP_QSTR_wp,               MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1       } },
        { MP_QSTR_hd,               MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1       } },
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

    uint32_t flags = SPICOMMON_BUSFLAG_MASTER;
    int dc = (int) args[ARG_dc].u_int;
    int host = (int) args[ARG_host].u_int;
    int mosi = (int) args[ARG_mosi].u_int;
    int miso = (int) args[ARG_miso].u_int;
    int sclk = (int) args[ARG_sclk].u_int;
    int cs = (int) args[ARG_cs].u_int;
    int freq = (int) args[ARG_freq].u_int;
    int wp = (int) args[ARG_wp].u_int;
    int hd = (int) args[ARG_hd].u_int;

    if ((args[ARG_spi_mode].u_int > 3) || (args[ARG_spi_mode].u_int < 0)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid spi mode (%d)"), args[ARG_spi_mode].u_int);
    }

    if ((mosi == -1) && (miso == -1) && (sclk == -1)) {
        if (host == 0) {
            #ifdef SPI_IOMUX_PIN_NUM_MISO
                sclk = SPI_IOMUX_PIN_NUM_CLK;
                mosi = SPI_IOMUX_PIN_NUM_MOSI;

                if (!args[ARG_tx_only].u_bool) {
                    miso = SPI_IOMUX_PIN_NUM_MISO;
                }
                if (cs == -1) {
                    cs = SPI_IOMUX_PIN_NUM_CS;
                }
                if (args[ARG_quad_spi].u_bool) {
                    if (wp == -1) {
                        wp = SPI_IOMUX_PIN_NUM_WP;
                    }
                    if (hd == -1) {
                        hd = SPI_IOMUX_PIN_NUM_HD;
                    }
                }
            #else
                mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("host 0 is not available for this board"));
            #endif
        } else if (host == 1) {
            #ifdef SPI2_IOMUX_PIN_NUM_MISO
                sclk = SPI2_IOMUX_PIN_NUM_CLK;
                mosi = SPI2_IOMUX_PIN_NUM_MOSI;

                if (!args[ARG_tx_only].u_bool) {
                    miso = SPI2_IOMUX_PIN_NUM_MISO;
                }
                if (cs == -1) {
                    cs = SPI2_IOMUX_PIN_NUM_CS;
                }
                if (args[ARG_quad_spi].u_bool) {
                    if (wp == -1) {
                        wp = SPI2_IOMUX_PIN_NUM_WP;
                    }
                    if (hd == -1) {
                        hd = SPI2_IOMUX_PIN_NUM_HD;
                    }
                }
            #else
                mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("host 1 is not available for this board"));
            #endif

        } else if (host == 2) {
            #ifdef SPI3_IOMUX_PIN_NUM_MISO
                sclk = SPI3_IOMUX_PIN_NUM_CLK;
                mosi = SPI3_IOMUX_PIN_NUM_MOSI;

                if (!args[ARG_tx_only].u_bool) {
                    miso = SPI3_IOMUX_PIN_NUM_MISO;
                }
                if (cs == -1) {
                    cs = SPI3_IOMUX_PIN_NUM_CS;
                }
                if (args[ARG_quad_spi].u_bool) {
                    if (wp == -1) {
                        wp = SPI3_IOMUX_PIN_NUM_WP;
                    }
                    if (hd == -1) {
                        hd = SPI3_IOMUX_PIN_NUM_HD;
                    }
                }
            #else
                mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("host 2 is not available for this board"));
            #endif
        } else {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid spi host (%d)"), host);
        }
    } else if (host == -1) {
        #ifdef SPI_IOMUX_PIN_NUM_MISO
            if (
                (mosi == SPI_IOMUX_PIN_NUM_MOSI) &&
                (sclk == SPI_IOMUX_PIN_NUM_CLK) &&
                (((args[ARG_tx_only].u_bool) && (miso == -1)) || ((!args[ARG_tx_only].u_bool) && (miso == SPI_IOMUX_PIN_NUM_MISO))) &&
                (
                    ((args[ARG_quad_spi].u_bool) && (wp == SPI_IOMUX_PIN_NUM_WP) && (hd == SPI_IOMUX_PIN_NUM_HD)) ||
                    ((!args[ARG_quad_spi].u_bool) && (wp == -1) && (hd == -1))
                ) &&
                ((cs == -1) || (cs == SPI_IOMUX_PIN_NUM_CS))
            ) {
                host = 0;
            } else {
                mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("unable to set host"));
            }
        #endif

        #ifdef SPI2_IOMUX_PIN_NUM_MISO
            if (
                (mosi == SPI2_IOMUX_PIN_NUM_MOSI) &&
                (sclk == SPI2_IOMUX_PIN_NUM_CLK) &&
                (((args[ARG_tx_only].u_bool) && (miso == -1)) || ((!args[ARG_tx_only].u_bool) && (miso == SPI2_IOMUX_PIN_NUM_MISO))) &&
                (
                    ((args[ARG_quad_spi].u_bool) && (wp == SPI2_IOMUX_PIN_NUM_WP) && (hd == SPI2_IOMUX_PIN_NUM_HD)) ||
                    ((!args[ARG_quad_spi].u_bool) && (wp == -1) && (hd == -1))
                ) &&
                ((cs == -1) || (cs == SPI2_IOMUX_PIN_NUM_CS))
            ) {
                host = 1;
            } else {
                mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("unable to set host"));
            }
        #endif

        #ifdef SPI3_IOMUX_PIN_NUM_MISO
            if (
                (mosi == SPI3_IOMUX_PIN_NUM_MOSI) &&
                (sclk == SPI3_IOMUX_PIN_NUM_CLK) &&
                (((args[ARG_tx_only].u_bool) && (miso == -1)) || ((!args[ARG_tx_only].u_bool) && (miso == SPI3_IOMUX_PIN_NUM_MISO))) &&
                (
                    ((args[ARG_quad_spi].u_bool) && (wp == SPI3_IOMUX_PIN_NUM_WP) && (hd == SPI3_IOMUX_PIN_NUM_HD)) ||
                    ((!args[ARG_quad_spi].u_bool) && (wp == -1) && (hd == -1))
                ) &&
                ((cs == -1) || (cs == SPI3_IOMUX_PIN_NUM_CS))
            ) {
                host = 2;
            } else {
                mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("unable to set host"));
            }
        #endif

    }

    if (!args[ARG_quad_spi].u_bool) {
        wp = -1;
        hd = -1;
    } else {
        flags |= SPICOMMON_BUSFLAG_QUAD;
    }

    if (args[ARG_tx_only].u_bool) {
        miso = -1;
    }

    if (host == 0) {
        #ifdef SPI_IOMUX_PIN_NUM_MISO
            if (
                (mosi == SPI_IOMUX_PIN_NUM_MOSI) &&
                (sclk == SPI_IOMUX_PIN_NUM_CLK) &&
                (((args[ARG_tx_only].u_bool) && (miso == -1)) || ((!args[ARG_tx_only].u_bool) && (miso == SPI_IOMUX_PIN_NUM_MISO))) &&
                (
                    ((args[ARG_quad_spi].u_bool) && (wp == SPI_IOMUX_PIN_NUM_WP) && (hd == SPI_IOMUX_PIN_NUM_HD)) ||
                    ((!args[ARG_quad_spi].u_bool) && (wp == -1) && (hd == -1))
                ) &&
                ((cs == -1) || (cs == SPI_IOMUX_PIN_NUM_CS))
            ) {
                if (freq == -1) {
                    freq = 80000000;
                }
                flags |= SPICOMMON_BUSFLAG_IOMUX_PINS;
            } else {
                if (freq == -1) {
                    freq = 26600000;
                }
                flags |= SPICOMMON_BUSFLAG_GPIO_PINS;
            }
        #else
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("spi host 0 is not supported by this board"));
        #endif
    } else if (host == 1) {
        #ifdef SPI2_IOMUX_PIN_NUM_MISO
            if (
                (mosi == SPI2_IOMUX_PIN_NUM_MOSI) &&
                (sclk == SPI2_IOMUX_PIN_NUM_CLK) &&
                (((args[ARG_tx_only].u_bool) && (miso == -1)) || ((!args[ARG_tx_only].u_bool) && (miso == SPI2_IOMUX_PIN_NUM_MISO))) &&
                (
                    ((args[ARG_quad_spi].u_bool) && (wp == SPI2_IOMUX_PIN_NUM_WP) && (hd == SPI2_IOMUX_PIN_NUM_HD)) ||
                    ((!args[ARG_quad_spi].u_bool) && (wp == -1) && (hd == -1))
                ) &&
                ((cs == -1) || (cs == SPI2_IOMUX_PIN_NUM_CS))
            ) {
                if (freq == -1) {
                    freq = 80000000;
                }
                flags |= SPICOMMON_BUSFLAG_IOMUX_PINS;
            } else {
                if (freq == -1) {
                    freq = 26600000;
                }
                flags |= SPICOMMON_BUSFLAG_GPIO_PINS;
            }
        #else
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("spi host 1 is not supported by this board"));
        #endif
    } else if (host == 2) {
        #ifdef SPI3_IOMUX_PIN_NUM_MISO
            if (
                (mosi == SPI3_IOMUX_PIN_NUM_MOSI) &&
                (sclk == SPI3_IOMUX_PIN_NUM_CLK) &&
                (((args[ARG_tx_only].u_bool) && (miso == -1)) || ((!args[ARG_tx_only].u_bool) && (miso == SPI3_IOMUX_PIN_NUM_MISO))) &&
                (
                    ((args[ARG_quad_spi].u_bool) && (wp == SPI3_IOMUX_PIN_NUM_WP) && (hd == SPI3_IOMUX_PIN_NUM_HD)) ||
                    ((!args[ARG_quad_spi].u_bool) && (wp == -1) && (hd == -1))
                ) &&
                ((cs == -1) || (cs == SPI3_IOMUX_PIN_NUM_CS))
            ) {
                if (freq == -1) {
                    freq = 80000000;
                }
                flags |= SPICOMMON_BUSFLAG_IOMUX_PINS;
            } else {
                if (freq == -1) {
                    freq = 26600000;
                }
                flags |= SPICOMMON_BUSFLAG_GPIO_PINS;
            }
        #else
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("spi host 2 is not supported by this board"));
        #endif
    } else {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid spi host (%d)"), host);
    }

    self->callback = mp_const_none;

    self->host = host;
    self->bus_handle = (esp_lcd_spi_bus_handle_t)((uint32_t)host);

    self->bus_config.sclk_io_num = sclk;
    self->bus_config.mosi_io_num = mosi;
    self->bus_config.miso_io_num = miso;
    self->bus_config.quadwp_io_num = wp;
    self->bus_config.quadhd_io_num = hd;
    self->bus_config.data4_io_num = -1;
    self->bus_config.data5_io_num = -1;
    self->bus_config.data6_io_num = -1;
    self->bus_config.data7_io_num = -1;
    self->bus_config.flags = flags;

    self->panel_io_config.cs_gpio_num = cs;
    self->panel_io_config.dc_gpio_num = dc;
    self->panel_io_config.spi_mode = args[ARG_spi_mode].u_int;
    self->panel_io_config.pclk_hz = freq;
    self->panel_io_config.trans_queue_depth = 1;
    self->panel_io_config.on_color_trans_done = bus_trans_done_cb;
    self->panel_io_config.user_ctx = self;
    self->panel_io_config.lcd_cmd_bits = args[ARG_cmd_bits].u_int;
    self->panel_io_config.lcd_param_bits = args[ARG_param_bits].u_int;
    self->panel_io_config.flags.dc_low_on_data = args[ARG_dc_low_on_data].u_bool;
    self->panel_io_config.flags.sio_mode = args[ARG_sio_mode].u_bool;
    self->panel_io_config.flags.lsb_first = args[ARG_lsb_first].u_bool;
    self->panel_io_config.flags.cs_high_active = args[ARG_cs_high_active].u_bool;
    self->panel_io_config.flags.octal_mode = 0;

    self->panel_io_handle.del = spi_del;
    self->panel_io_handle.init = spi_init;
    self->panel_io_handle.get_lane_count = spi_get_lane_count;

    return MP_OBJ_FROM_PTR(self);
}


mp_lcd_err_t spi_del(lcd_panel_io_t *io)
{
    mp_lcd_spi_bus_obj_t *self = __containerof(io, mp_lcd_spi_bus_obj_t, panel_io_handle);

    mp_lcd_err_t ret = esp_lcd_panel_io_del(io->panel_io);
    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
    }

    ret = spi_bus_free(self->host);
    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(spi_bus_free)"), ret);
    }

    gpio_pad_select_gpio(self->bus_config.miso_io_num);
    gpio_matrix_out(self->bus_config.miso_io_num, SIG_GPIO_OUT_IDX, false, false);
    gpio_set_direction(self->bus_config.miso_io_num, GPIO_MODE_INPUT);

    gpio_pad_select_gpio(self->bus_config.mosi_io_num);
    gpio_matrix_out(self->bus_config.mosi_io_num, SIG_GPIO_OUT_IDX, false, false);
    gpio_set_direction(self->bus_config.mosi_io_num, GPIO_MODE_INPUT);

    gpio_pad_select_gpio(self->bus_config.sclk_io_num);
    gpio_matrix_out(self->bus_config.sclk_io_num, SIG_GPIO_OUT_IDX, false, false);
    gpio_set_direction(self->bus_config.sclk_io_num, GPIO_MODE_INPUT);

    return ret;
}


mp_lcd_err_t spi_init(lcd_panel_io_t *io, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size)
{
    mp_lcd_spi_bus_obj_t *self = __containerof(io, mp_lcd_spi_bus_obj_t, panel_io_handle);

    self->bus_config.max_transfer_sz = (size_t)buffer_size;

    mp_lcd_err_t ret = spi_bus_initialize(self->host, &self->bus_config, SPI_DMA_CH_AUTO);
    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(spi_bus_initialize)"), ret);
    }

    ret = esp_lcd_new_panel_io_spi(self->bus_handle, &self->panel_io_config, &io->panel_io);
    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_spi)"), ret);
    }

    return ret;
}


mp_lcd_err_t spi_get_lane_count(lcd_panel_io_t *io, uint8_t *lane_count)
{
    mp_lcd_spi_bus_obj_t *self = __containerof(io, mp_lcd_spi_bus_obj_t, panel_io_handle);

    *lane_count = 1;

    if (self->panel_io_config.flags.sio_mode) {
        *lane_count = 2;
    }
    if (self->bus_config.quadwp_io_num != -1) {
        *lane_count = 4;
    }

    return LCD_OK;
}


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_spi_bus_type,
    MP_QSTR_SPI_Bus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_spi_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
);
