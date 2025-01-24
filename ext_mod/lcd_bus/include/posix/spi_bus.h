// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _SPI_BUS_H_
    #define _SPI_BUS_H_

    //local_includes
    #include "common/lcd_common_types.h"
    #include "common/modlcd_bus.h"

    #include "py/obj.h"


    struct _mp_lcd_spi_bus_obj_t {
        struct _mp_lcd_bus_obj_t;

        void *panel_io_config;
        void *bus_handle;

        /* specific to bus */
        void (*send_cmd)(mp_lcd_spi_bus_obj_t *self, int lcd_cmd);
        void (*send_param)(mp_lcd_spi_bus_obj_t *self, void *param, size_t param_size);
    };

#endif /* _SPI_BUS_H_ */