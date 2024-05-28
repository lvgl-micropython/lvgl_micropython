
#ifndef __ESP32_SPI_DEVICE_H__
    #define __ESP32_SPI_DEVICE_H__

    #include "esp32_spi_bus.h"

    #include "py/runtime.h"
    #include "py/stream.h"
    #include "py/mphal.h"
    #include "mphalport.h"

    #include "driver/spi_master.h"
    #include "driver/spi_common.h"
    #include "soc/spi_pins.h"


    typedef struct {
        spi_transaction_ext_t base;
        bool trans_done;
        mp_obj_t callback;
    } esp32_spi_trans_descriptor_t;


    typedef struct _esp32_hw_spi_dev_obj_t{
        mp_obj_base_t base;
        spi_device_interface_config_t devcfg;
        esp32_hw_spi_bus_obj_t *spi_bus;
        uint8_t bits;
        size_t max_transfer_sz;
        spi_device_handle_t spi_dev;

        mp_obj_t trans_start_cb;
        mp_obj_t trans_start_user_data;

        mp_obj_t trans_end_cb;
        mp_obj_t trans_end_user_data;

        size_t num_trans_inflight;
        size_t trans_pool_count;
        esp32_spi_trans_descriptor_t trans_pool[];
    } esp32_hw_spi_dev_obj_t;

    extern const mp_obj_type_t esp32_hw_spi_dev_type;

#endif