#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"
#include "mphalport.h"

#include "driver/spi_master.h"
#include "driver/spi_common.h"
#include "soc/spi_pins.h"

#ifndef __ESP32_SPI_H__
    #define __ESP32_SPI_H__

    typedef struct _esp32_hw_spi_dev_obj_t esp32_hw_spi_dev_obj_t;
    typedef struct _esp32_hw_spi_bus_obj_t esp32_hw_spi_bus_obj_t;

    typedef struct {
        spi_transaction_t base;
        bool trans_done;
    } esp32_spi_trans_descriptor_t;


    struct _esp32_hw_spi_dev_obj_t{
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
    };


    struct _esp32_hw_spi_bus_obj_t{
        mp_obj_base_t base;
        spi_host_device_t host;
        spi_bus_config_t buscfg;
        bool octal_mode;

        enum {
            MACHINE_HW_SPI_STATE_NONE,
            MACHINE_HW_SPI_STATE_INIT,
            MACHINE_HW_SPI_STATE_DEINIT
        } state;
    };


    typedef struct _esp32_hw_spi_default_pins_t{
        int8_t mosi_io_num;
        int8_t miso_io_num;
        int8_t sclk_io_num;
        int8_t quadwp_io_num;
        int8_t quadhd_io_num;
        int8_t data4_io_num;
        int8_t data5_io_num;
        int8_t data6_io_num;
        int8_t data7_io_num;
        int8_t cs_io_num;
    } esp32_hw_spi_default_pins_t;


    extern const mp_obj_type_t esp32_hw_spi_bus_type;
    extern const mp_obj_type_t esp32_hw_spi_dev_type;

#endif