

#ifndef __ESP32_SPI_H__
    #define __ESP32_SPI_H__

    #include "py/runtime.h"
    #include "py/stream.h"
    #include "py/mphal.h"
    #include "mphalport.h"

    #include "driver/spi_master.h"
    #include "driver/spi_common.h"
    #include "soc/spi_pins.h"

    typedef struct _esp32_hw_spi_bus_obj_t{
        mp_obj_base_t base;
        spi_host_device_t host;
        spi_bus_config_t buscfg;
        bool octal_mode;
        bool dual_mode;

        enum {
            MACHINE_HW_SPI_STATE_NONE,
            MACHINE_HW_SPI_STATE_INIT,
            MACHINE_HW_SPI_STATE_DEINIT
        } state;
    } esp32_hw_spi_bus_obj_t;

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

#endif