// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"


#ifndef _SPI_BUS_H_
    #define _SPI_BUS_H_

    #include "esp_lcd_panel_io.h"
    #include "driver/spi_common.h"
    #include "driver/spi_master.h"
    #include "hal/spi_types.h"

    #include "common/lcd_common_types.h"
    #include "../../../../micropy_updates/common/mp_spi_common.h"


    struct _mp_lcd_spi_bus_obj_t {
        struct _mp_lcd_bus_obj_t;

        esp_lcd_panel_io_spi_config_t *panel_io_config;
        spi_bus_config_t *bus_config;

        /* specific to bus */
        esp_lcd_spi_bus_handle_t bus_handle;

        spi_host_device_t host;
        mp_machine_hw_spi_device_obj_t spi_device;
    };

#endif /* _SPI_BUS_H_ */

