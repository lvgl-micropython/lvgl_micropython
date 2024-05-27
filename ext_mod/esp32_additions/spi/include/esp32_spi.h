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
        spi_device_interface_config_t devcfg;
        esp32_hw_spi_bus_obj_t *spi_bus;
        uint8_t bits;
        size_t max_transfer_sz;
        spi_device_handle_t spi_dev;

        mp_obj_t trans_start_cb;            // User register's callback, invoked when data trans starts
        mp_obj_t trans_start_user_data;     // User's private data, passed directly to callback trans_start_cb

        mp_obj_t trans_end_cb;              // User register's callback, invoked when data trans end
        mp_obj_t trans_end_user_data;       // User's private data, passed directly to callback trans_end_cb

        size_t num_trans_inflight;          // Number of transactions that are undergoing (the descriptor not recycled yet)
        size_t trans_pool_count;            // Size of transaction queue
        esp32_spi_trans_descriptor_t trans_pool[]; // Transaction pool
    };

    struct _esp32_hw_spi_bus_obj_t{
        mp_obj_base_t base;
        spi_host_device_t host;
        spi_bus_config_t buscfg;
        bool octal_mode;                    // Indicates whether the transmitting is enabled with octal mode (8 data lines)

        enum {
            MACHINE_HW_SPI_STATE_NONE,
            MACHINE_HW_SPI_STATE_INIT,
            MACHINE_HW_SPI_STATE_DEINIT
        } state;

    };

    typedef struct _esp32_hw_spi_default_pins_t{
        int8_t mosi_io_num;    ///< GPIO pin for Master Out Slave In (=spi_d) signal, or -1 if not used.
        int8_t miso_io_num;    ///< GPIO pin for Master In Slave Out (=spi_q) signal, or -1 if not used.
        int8_t sclk_io_num;      ///< GPIO pin for SPI Clock signal, or -1 if not used.
        int8_t quadwp_io_num;  ///< GPIO pin for WP (Write Protect) signal, or -1 if not used.
        int8_t quadhd_io_num;  ///< GPIO pin for HD (Hold) signal, or -1 if not used.
        int8_t data4_io_num;     ///< GPIO pin for spi data4 signal in octal mode, or -1 if not used.
        int8_t data5_io_num;     ///< GPIO pin for spi data5 signal in octal mode, or -1 if not used.
        int8_t data6_io_num;     ///< GPIO pin for spi data6 signal in octal mode, or -1 if not used.
        int8_t data7_io_num;     ///< GPIO pin for spi data7 signal in octal mode, or -1 if not used.
        int8_t cs_io_num;
    } esp32_hw_spi_default_pins_t;

    extern const mp_obj_type_t esp32_hw_spi_bus_type;
    extern const mp_obj_type_t esp32_hw_spi_dev_type;

#endif