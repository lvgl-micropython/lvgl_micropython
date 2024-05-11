
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
#include "hal/spi_ll.h"

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
        ARG_dc,
        ARG_host,
        ARG_sclk,
        ARG_freq,
        ARG_mosi,
        ARG_miso,
        ARG_cs,
        ARG_wp,
        ARG_hd,
        ARG_cmd_bits,
        ARG_param_bits,
        ARG_dc_low_on_data,
        ARG_sio_mode,
        ARG_lsb_first,
        ARG_cs_high_active,
        ARG_spi_mode
    };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_dc,               MP_ARG_OBJ  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_host,             MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_sclk,             MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_freq,             MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_mosi,             MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_miso,             MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_cs,               MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1       } },
        { MP_QSTR_wp,               MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1       } },
        { MP_QSTR_hd,               MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1       } },
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

    if ((args[ARG_spi_mode].u_int > 3) || (args[ARG_spi_mode].u_int < 0)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid spi mode (%d)"), args[ARG_spi_mode].u_int);
    }

    uint32_t flags = SPICOMMON_BUSFLAG_MASTER;

    int wp = (int) args[ARG_wp].u_int;
    int hd = (int) args[ARG_hd].u_int;

    if (wp != -1 && hd != -1) {
        flags |= SPICOMMON_BUSFLAG_QUAD;
    } else {
        wp = -1;
        hd = -1;
    }

    int clk = (int)args[ARG_sclk].u_int;
    int mosi = (int)args[ARG_mosi].u_int;
    int miso = (int)args[ARG_miso].u_int;
    int cs = (int)args[ARG_cs].u_int;
    int host = (int)args[ARG_host].u_int;

    int temp_host;

    if (
        mosi == SPI_IOMUX_PIN_NUM_MOSI &&
        clk == SPI_IOMUX_PIN_NUM_CLK &&
        (miso == -1 || miso == SPI_IOMUX_PIN_NUM_MISO) &&
        (cs == -1 || cs == SPI_IOMUX_PIN_NUM_CS) &&
        (hd == -1 || hd == SPI_IOMUX_PIN_NUM_HD) &&
        (wp == -1 || wp == SPI_IOMUX_PIN_NUM_WP)
    ) {
        flags |= SPICOMMON_BUSFLAG_IOMUX_PINS;
        temp_host = SPI1_HOST;
    } else if (
        mosi == SPI2_IOMUX_PIN_NUM_MOSI &&
        clk == SPI2_IOMUX_PIN_NUM_CLK &&
        (miso == -1 || miso == SPI2_IOMUX_PIN_NUM_MISO) &&
        (cs == -1 || cs == SPI2_IOMUX_PIN_NUM_CS) &&
        (hd == -1 || hd == SPI2_IOMUX_PIN_NUM_HD) &&
        (wp == -1 || wp == SPI2_IOMUX_PIN_NUM_WP)
    ) {
        flags |= SPICOMMON_BUSFLAG_IOMUX_PINS;
        temp_host = SPI2_HOST;
    }
    #ifdef SPI3_IOMUX_PIN_NUM_MISO
    else if (
        mosi == SPI3_IOMUX_PIN_NUM_MOSI &&
        clk == SPI3_IOMUX_PIN_NUM_CLK &&
        (miso == -1 || miso == SPI3_IOMUX_PIN_NUM_MISO) &&
        (cs == -1 || cs == SPI3_IOMUX_PIN_NUM_CS) &&
        (hd == -1 || hd == SPI3_IOMUX_PIN_NUM_HD) &&
        (wp == -1 || wp == SPI3_IOMUX_PIN_NUM_WP)
    ) {
        flags |= SPICOMMON_BUSFLAG_IOMUX_PINS;
        temp_host = SPI3_HOST;
    }
    #endif
    else {
        flags |= SPICOMMON_BUSFLAG_GPIO_PINS;
        temp_host = -1;
    }

    if (temp_host == -1 && host == -1) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Unable to determine SPI host, please supply a host number"));
    } else if (host == -1) {
        host = temp_host;
    }

    self->callback = mp_const_none;

    self->bus_handle = (spi_host_device_t)host;

    self->bus_config.sclk_io_num = clk;
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
    self->panel_io_config.dc_gpio_num = (int)args[ARG_dc].u_int;
    self->panel_io_config.spi_mode = (int)args[ARG_spi_mode].u_int;
    self->panel_io_config.pclk_hz = (unsigned int)args[ARG_freq].u_int;
    self->panel_io_config.on_color_trans_done = &bus_trans_done_cb;
    self->panel_io_config.user_ctx = self;
    self->panel_io_config.lcd_cmd_bits = (int)args[ARG_cmd_bits].u_int;
    self->panel_io_config.lcd_param_bits = (int)args[ARG_param_bits].u_int;
    self->panel_io_config.flags.dc_low_on_data = (unsigned int)args[ARG_dc_low_on_data].u_bool;
    self->panel_io_config.flags.sio_mode = (unsigned int)args[ARG_sio_mode].u_bool;
    self->panel_io_config.flags.lsb_first = (unsigned int)args[ARG_lsb_first].u_bool;
    self->panel_io_config.flags.cs_high_active = (unsigned int)args[ARG_cs_high_active].u_bool;
    self->panel_io_config.flags.octal_mode = 0;

    self->panel_io_handle.del = &spi_del;
    self->panel_io_handle.init = &spi_init;
    self->panel_io_handle.get_lane_count = &spi_get_lane_count;

    return MP_OBJ_FROM_PTR(self);
}


mp_lcd_err_t spi_del(mp_obj_t obj)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;

    mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle.panel_io);
    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
    }

    ret = spi_bus_free(self->bus_handle);
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


mp_lcd_err_t spi_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap)
{
    mp_lcd_spi_bus_obj_t *self = (mp_lcd_spi_bus_obj_t *)obj;

    if (bpp == 16) {
        self->rgb565_byte_swap = rgb565_byte_swap;
    } else {
        self->rgb565_byte_swap = false;
    }

    size_t max_trans_size;
    uint8_t double_buffer;

    if ((self->buffer_flags ^ MALLOC_CAP_DMA) && (self->buf2 != NULL)) {
        max_trans_size = SPI_LL_DMA_MAX_BIT_LEN / 8;
        double_buffer = 1;
    } else {
        max_trans_size = SPI_LL_CPU_MAX_BIT_LEN / 8;
        double_buffer = 0;
    }

    if (buffer_size <= max_trans_size) {
        self->bus_config.max_transfer_sz = buffer_size;
        self->panel_io_config.trans_queue_depth = 1;
    } else if (double_buffer) {
        self->bus_config.max_transfer_sz = max_trans_size;
        self->panel_io_config.trans_queue_depth = buffer_size / max_trans_size;
        if ((float)self->panel_io_config.trans_queue_depth != ((float)buffer_size / (float)max_trans_size)) {
            self->panel_io_config.trans_queue_depth++;
        }
    } else {
        self->panel_io_config.trans_queue_depth = 10;
    }

    mp_lcd_err_t ret = spi_bus_initialize(self->bus_handle, &self->bus_config, SPI_DMA_CH_AUTO);
    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(spi_bus_initialize)"), ret);
    }

    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)self->bus_handle, &self->panel_io_config, &self->panel_io_handle.panel_io);
    if (ret != 0) {
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
    }
    if (self->bus_config.quadwp_io_num != -1) {
        *lane_count = 4;
    }

    return LCD_OK;
}


STATIC const mp_rom_map_elem_t mp_lcd_spi_bus_locals_dict_table[] = {
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
    { MP_ROM_QSTR(MP_QSTR_SPI_MAXIMUM_BUFFER_SIZE),  MP_ROM_INT(SOC_SPI_MAXIMUM_BUFFER_SIZE)          },
};

STATIC MP_DEFINE_CONST_DICT(mp_lcd_spi_bus_locals_dict, mp_lcd_spi_bus_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_spi_bus_type,
    MP_QSTR_SPI_Bus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_spi_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_spi_bus_locals_dict
);
