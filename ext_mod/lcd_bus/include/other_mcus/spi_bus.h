// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _SPI_BUS_H_
    #define _SPI_BUS_H_

    //local_includes
    #include "common/modlcd_bus.h"
    #include "common/lcd_framebuf.h"
    #include "common/sw_rotate.h"

    #include "mphalport.h"

    // micropython includes
    #include "py/objarray.h"
    #include "py/obj.h"

    // Fix for MicroPython > 1.21 https://github.com/ricksorensen
    #include "../../../micropy_updates/common/mp_spi_common.h"

    typedef struct _lcd_panel_io_spi_config_t {
        mp_obj_t cs_gpio;
        mp_hal_pin_obj_t dc_gpio;
        void (*spi_transfer)(mp_obj_base_t *obj, size_t len, const uint8_t *src, uint8_t *dest);
        struct {
            unsigned int dc_low_on_data: 1;
            unsigned int lsb_first: 1;
            unsigned int cs_high_active: 1;
        } flags;
    } lcd_panel_io_spi_config_t;


    struct _mp_lcd_spi_bus_obj_t {
        struct _mp_lcd_bus_obj_t;


        /* config settings */
        lcd_panel_io_spi_config_t *panel_io_config;

        /* stores the SPI instance that gets created.
         * SPI that is internal to micropython is what is being used
         */
        mp_obj_base_t *bus_handle;

        /* these function pointers get set based on
         * panel_io_config.lcd_cmd_bits and panel_io_config.lcd_param_bits
         */
        void (*send_cmd)(mp_lcd_spi_bus_obj_t *self, int lcd_cmd);
        void (*send_param)(mp_lcd_spi_bus_obj_t *self, void *param, size_t param_size);

        int host;
        machine_hw_spi_device_obj_t *spi_bus;
        uint8_t firstbit;
        uint32_t freq;
    };

#endif /* _SPI_BUS_H_ */