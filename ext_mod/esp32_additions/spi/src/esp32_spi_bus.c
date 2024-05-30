#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../include/esp32_spi_bus.h"

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "py/gc.h"
#include "py/stackctrl.h"
#include "py/objarray.h"
#include "py/binary.h"

#include "driver/spi_master.h"
#include "driver/spi_common.h"
#include "soc/spi_pins.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_cpu.h"
#include "hal/spi_ll.h"


#if CONFIG_IDF_TARGET_ESP32
    #define ESP32_HW_SPI_MAX  2
#else
    #define ESP32_HW_SPI_MAX  1
#endif /* CONFIG_IDF_TARGET_ESP32 */


#if SOC_SPI_SUPPORT_OCT
    #define ESP32_HW_SPI_OCT_MAX  1
#else
    #define ESP32_HW_SPI_OCT_MAX  0
#endif /* SOC_SPI_SUPPORT_OCT */


STATIC esp32_hw_spi_bus_obj_t esp32_hw_spi_bus_obj[ESP32_HW_SPI_MAX];


mp_obj_t esp32_hw_spi_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum {
        ARG_host,
        ARG_mosi,
        ARG_miso,
        ARG_sck,
        ARG_wp,
        ARG_hd,
        ARG_data4,
        ARG_data5,
        ARG_data6,
        ARG_data7,
        ARG_dual
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_host,  MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_mosi,  MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_miso,  MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_sck,   MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_wp,    MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_hd,    MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_data4, MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_data5, MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_data6, MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_data7, MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_dual,  MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false       } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    esp32_hw_spi_bus_obj_t *self = NULL;

    esp32_hw_spi_default_pins_t default_pins_array[ESP32_HW_SPI_MAX] = {
        {
            .mosi_io_num = SPI2_IOMUX_PIN_NUM_MOSI,
            .miso_io_num = SPI2_IOMUX_PIN_NUM_MISO,
            .sclk_io_num = SPI2_IOMUX_PIN_NUM_CLK,
            .quadhd_io_num = SPI2_IOMUX_PIN_NUM_HD,
            .quadwp_io_num = SPI2_IOMUX_PIN_NUM_WP,
            .data4_io_num = -1,
            .data5_io_num = -1,
            .data6_io_num = -1,
            .data7_io_num = -1,
        },
    #if CONFIG_IDF_TARGET_ESP32
        {
            .mosi_io_num = SPI3_IOMUX_PIN_NUM_MOSI,
            .miso_io_num = SPI3_IOMUX_PIN_NUM_MISO,
            .sclk_io_num = SPI3_IOMUX_PIN_NUM_CLK,
            .quadhd_io_num = SPI3_IOMUX_PIN_NUM_HD,
            .quadwp_io_num = SPI3_IOMUX_PIN_NUM_WP,
            .data4_io_num = -1,
            .data5_io_num = -1,
            .data6_io_num = -1,
            .data7_io_num = -1,
        },
    #endif /* CONFIG_IDF_TARGET_ESP32 */
    };
    #if SOC_SPI_SUPPORT_OCT
    esp32_hw_spi_default_pins_t default_oct_pins_array[ESP32_HW_SPI_MAX] = {
        {
            .mosi_io_num = SPI2_IOMUX_PIN_NUM_MOSI_OCT,
            .miso_io_num = SPI2_IOMUX_PIN_NUM_MISO_OCT,
            .sclk_io_num = SPI2_IOMUX_PIN_NUM_CLK_OCT,
            .quadhd_io_num = SPI2_IOMUX_PIN_NUM_HD_OCT,
            .quadwp_io_num = SPI2_IOMUX_PIN_NUM_WP_OCT,
            .data4_io_num = SPI2_IOMUX_PIN_NUM_IO4_OCT,
            .data5_io_num = SPI2_IOMUX_PIN_NUM_IO5_OCT,
            .data6_io_num = SPI2_IOMUX_PIN_NUM_IO6_OCT,
            .data7_io_num = SPI2_IOMUX_PIN_NUM_IO7_OCT,
        },
    };

    esp32_hw_spi_default_pins_t *default_oct_pins;
    #endif /* SOC_SPI_SUPPORT_OCT */

    esp32_hw_spi_default_pins_t *default_pins;

    mp_int_t host = args[ARG_host].u_int;

    if (1 <= host && host <= ESP32_HW_SPI_MAX) {
        self = &esp32_hw_spi_bus_obj[host - 1];
        default_pins = &default_pins_array[host - 1];
        #if SOC_SPI_SUPPORT_OCT
        default_oct_pins = &default_oct_pins_array[host - 1];
        #endif /* SOC_SPI_SUPPORT_OCT */
    } else {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%d) doesn't exist"), host);
        return mp_const_none;
    }

    if (self->state == MACHINE_HW_SPI_STATE_INIT) {
        return MP_OBJ_FROM_PTR(self);
    }

    self->base.type = &esp32_hw_spi_bus_type;

    int8_t sck;
    int8_t mosi;
    int8_t miso;
    int8_t hd;
    int8_t wp;
    int8_t data4;
    int8_t data5;
    int8_t data6;
    int8_t data7;

    #if SOC_SPI_SUPPORT_OCT
    if (args[ARG_data4].u_obj == MP_OBJ_NULL) {
        data4 = -1;
    } else if (args[ARG_data4].u_obj == mp_const_none) {
        data4 = default_oct_pins->data4_io_num;
    } else {
        data4 = machine_pin_get_id(args[ARG_data4].u_obj);
    }

    if (args[ARG_data5].u_obj == MP_OBJ_NULL) {
        data5 = -1;
    } else if (args[ARG_data5].u_obj == mp_const_none) {
        data5 = default_oct_pins->data5_io_num;
    } else {
        data5 = machine_pin_get_id(args[ARG_data5].u_obj);
    }

    if (args[ARG_data6].u_obj == MP_OBJ_NULL) {
        data6 = -1;
    } else if (args[ARG_data6].u_obj == mp_const_none) {
        data6 = default_oct_pins->data6_io_num;
    } else {
        data6 = machine_pin_get_id(args[ARG_data6].u_obj);
    }

    if (args[ARG_data7].u_obj == MP_OBJ_NULL) {
        data7 = -1;
    } else if (args[ARG_data7].u_obj == mp_const_none) {
        data7 = default_oct_pins->data7_io_num;
    } else {
        data7 = machine_pin_get_id(args[ARG_data7].u_obj);
    }

    if (data4 != -1 && data5 != -1 && data6 != -1 && data7 != -1) {
        if (args[ARG_sck].u_obj == MP_OBJ_NULL) {
            sck = default_oct_pins->sclk_io_num;
        } else if (args[ARG_sck].u_obj == mp_const_none) {
            sck = -1;
        } else {
            sck = machine_pin_get_id(args[ARG_sck].u_obj);
        }

        if (args[ARG_mosi].u_obj == MP_OBJ_NULL) {
            mosi = default_oct_pins->mosi_io_num;
        } else if (args[ARG_mosi].u_obj == mp_const_none) {
            mosi = -1;
        } else {
            mosi = machine_pin_get_id(args[ARG_mosi].u_obj);
        }

        if (args[ARG_miso].u_obj == MP_OBJ_NULL) {
            miso = default_oct_pins->miso_io_num;
        } else if (args[ARG_miso].u_obj == mp_const_none) {
            miso = -1;
        } else {
            miso = machine_pin_get_id(args[ARG_miso].u_obj);
        }

        if (args[ARG_hd].u_obj == MP_OBJ_NULL) {
            hd = -1;
        } else if (args[ARG_hd].u_obj == mp_const_none) {
            hd = default_oct_pins->quadhd_io_num;
        } else {
            hd = machine_pin_get_id(args[ARG_hd].u_obj);
        }

        if (args[ARG_wp].u_obj == MP_OBJ_NULL) {
            wp = -1;
        } else if (args[ARG_wp].u_obj == mp_const_none) {
            wp = default_oct_pins->quadwp_io_num;
        } else {
            wp = machine_pin_get_id(args[ARG_wp].u_obj);
        }
    } else {
        data4 = -1;
        data5 = -1;
        data6 = -1;
        data7 = -1;

        if (args[ARG_sck].u_obj == MP_OBJ_NULL) {
            sck = default_pins->sclk_io_num;
        } else if (args[ARG_sck].u_obj == mp_const_none) {
            sck = -1;
        } else {
            sck = machine_pin_get_id(args[ARG_sck].u_obj);
        }

        if (args[ARG_mosi].u_obj == MP_OBJ_NULL) {
            mosi = default_pins->mosi_io_num;
        } else if (args[ARG_mosi].u_obj == mp_const_none) {
            mosi = -1;
        } else {
            mosi = machine_pin_get_id(args[ARG_mosi].u_obj);
        }

        if (args[ARG_miso].u_obj == MP_OBJ_NULL) {
            miso = default_pins->miso_io_num;
        } else if (args[ARG_miso].u_obj == mp_const_none) {
            miso = -1;
        } else {
            miso = machine_pin_get_id(args[ARG_miso].u_obj);
        }

        if (args[ARG_hd].u_obj == MP_OBJ_NULL) {
            hd = -1;
        } else if (args[ARG_hd].u_obj == mp_const_none) {
            hd = default_pins->quadhd_io_num;
        } else {
            hd = machine_pin_get_id(args[ARG_hd].u_obj);
        }

        if (args[ARG_wp].u_obj == MP_OBJ_NULL) {
            wp = -1;
        } else if (args[ARG_wp].u_obj == mp_const_none) {
            wp = default_pins->quadwp_io_num;
        } else {
            wp = machine_pin_get_id(args[ARG_wp].u_obj);
        }
    }

    #else

    if (args[ARG_sck].u_obj == MP_OBJ_NULL) {
        sck = default_pins->sclk_io_num;
    } else if (args[ARG_sck].u_obj == mp_const_none) {
        sck = -1;
    } else {
        sck = machine_pin_get_id(args[ARG_sck].u_obj);
    }

    if (args[ARG_mosi].u_obj == MP_OBJ_NULL) {
        mosi = default_pins->mosi_io_num;
    } else if (args[ARG_mosi].u_obj == mp_const_none) {
        mosi = -1;
    } else {
        mosi = machine_pin_get_id(args[ARG_mosi].u_obj);
    }

    if (args[ARG_miso].u_obj == MP_OBJ_NULL) {
        miso = default_pins->miso_io_num;
    } else if (args[ARG_miso].u_obj == mp_const_none) {
        miso = -1;
    } else {
        miso = machine_pin_get_id(args[ARG_miso].u_obj);
    }

    if (args[ARG_hd].u_obj == MP_OBJ_NULL) {
        hd = -1;
    } else if (args[ARG_hd].u_obj == mp_const_none) {
        hd = default_pins->quadhd_io_num;
    } else {
        hd = machine_pin_get_id(args[ARG_hd].u_obj);
    }

    if (args[ARG_wp].u_obj == MP_OBJ_NULL) {
        wp = -1;
    } else if (args[ARG_wp].u_obj == mp_const_none) {
        wp = default_pins->quadwp_io_num;
    } else {
        wp = machine_pin_get_id(args[ARG_wp].u_obj);
    }

    if (args[ARG_data4].u_obj == MP_OBJ_NULL) {
        data4 = -1;
    } else if (args[ARG_data4].u_obj == mp_const_none) {
        data4 = default_pins->data4_io_num;
    } else {
        data4 = machine_pin_get_id(args[ARG_data4].u_obj);
    }

    if (args[ARG_data5].u_obj == MP_OBJ_NULL) {
        data5 = -1;
    } else if (args[ARG_data5].u_obj == mp_const_none) {
        data5 = default_pins->data5_io_num;
    } else {
        data5 = machine_pin_get_id(args[ARG_data5].u_obj);
    }

    if (args[ARG_data6].u_obj == MP_OBJ_NULL) {
        data6 = -1;
    } else if (args[ARG_data6].u_obj == mp_const_none) {
        data6 = default_pins->data6_io_num;
    } else {
        data6 = machine_pin_get_id(args[ARG_data6].u_obj);
    }

    if (args[ARG_data7].u_obj == MP_OBJ_NULL) {
        data7 = -1;
    } else if (args[ARG_data7].u_obj == mp_const_none) {
        data7 = default_pins->data7_io_num;
    } else {
        data7 = machine_pin_get_id(args[ARG_data7].u_obj);
    }
    #endif /* SOC_SPI_SUPPORT_OCT */


    uint32_t buscfg_flags = SPICOMMON_BUSFLAG_MASTER;

    if (hd != -1 && wp != -1 && data4 != -1 && data5 != -1 && data6 != -1 && data7 != -1) {
    #if SOC_SPI_SUPPORT_OCT
        if (host != SPI2_HOST) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI octal mode is only supported using host 1"));
            return mp_const_none;
        } else {
            self->octal_mode = true;
        }
    #else
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI octal mode is not supported on this MCU"));
        return mp_const_none;
    #endif /* SOC_SPI_SUPPORT_OCT */
    } else {
        self->octal_mode = false;
    }
    self->dual_mode = false;
    if ((bool)args[ARG_dual].u_bool) {
        if (mosi != -1 && miso != -1) {
            buscfg_flags |= SPICOMMON_BUSFLAG_DUAL;
            self->dual_mode = true;
        } else {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("miso and mosi pins are needed to use dual mode"));
            return mp_const_none;
        }
    }

    self->buscfg = (spi_bus_config_t){
        .miso_io_num = (int)miso,
        .mosi_io_num = (int)mosi,
        .sclk_io_num = (int)sck,
        .quadwp_io_num = (int)wp,
        .quadhd_io_num = (int)hd,
        .data4_io_num = (int)data4,
        .data5_io_num = (int)data5,
        .data6_io_num = (int)data6,
        .data7_io_num = (int)data7,
        .max_transfer_sz = SPI_LL_DMA_MAX_BIT_LEN / 8,
        .flags = buscfg_flags
    };
    self->host = (spi_host_device_t)host;

#if CONFIG_IDF_TARGET_ESP32
    int dma_chan = SPI_DMA_DISABLED;

    if (self->host == SPI2_HOST) {
        dma_chan = SPI_DMA_CH1;
    } else {
       dma_chan = SPI_DMA_CH2;
    }
#else
    int dma_chan = SPI_DMA_CH_AUTO;
#endif  /* CONFIG_IDF_TARGET_ESP32 */

    esp_err_t ret = spi_bus_initialize(self->host, &self->buscfg, dma_chan);
    if (ret != ESP_OK) {
        check_esp_err(ret);
        return mp_const_none;
    }

    self->state = MACHINE_HW_SPI_STATE_INIT;

    return MP_OBJ_FROM_PTR(self);
}


STATIC mp_obj_t esp32_hw_spi_bus_deinit(mp_obj_t self_in)
{
    esp32_hw_spi_bus_obj_t *self = (esp32_hw_spi_bus_obj_t *)self_in;

    if (self->state == MACHINE_HW_SPI_STATE_INIT) {
        self->state = MACHINE_HW_SPI_STATE_DEINIT;
        esp_err_t ret = spi_bus_free(self->host);
        check_esp_err(ret);
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(esp32_hw_spi_bus_deinit_obj, esp32_hw_spi_bus_deinit);


STATIC mp_obj_t esp32_hw_spi_bus_get_host(mp_obj_t self_in) {
    esp32_hw_spi_bus_obj_t *self = (esp32_hw_spi_bus_obj_t *)self_in;
    return mp_obj_new_int(self->host);
}

MP_DEFINE_CONST_FUN_OBJ_1(esp32_hw_spi_bus_get_host_obj, esp32_hw_spi_bus_get_host);


STATIC const mp_rom_map_elem_t esp32_hw_spi_bus_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_host),  MP_ROM_PTR(&esp32_hw_spi_bus_get_host_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),    MP_ROM_PTR(&esp32_hw_spi_bus_deinit_obj)   },
    { MP_ROM_QSTR(MP_QSTR___del__),   MP_ROM_PTR(&esp32_hw_spi_bus_deinit_obj)   }
};

MP_DEFINE_CONST_DICT(esp32_hw_spi_bus_locals_dict, esp32_hw_spi_bus_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    esp32_hw_spi_bus_type,
    MP_QSTR_SPI,
    MP_TYPE_FLAG_NONE,
    make_new, esp32_hw_spi_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&esp32_hw_spi_bus_locals_dict
);