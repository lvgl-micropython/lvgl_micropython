// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _SPI_BUS_H_
    #define _SPI_BUS_H_

    //local_includes
    #include "modlcd_bus.h"
    #include "mphalport.h"

    // micropython includes
    #include "py/objarray.h"
    #include "py/obj.h"


    typedef struct _mp_lcd_spi_bus_obj_t mp_lcd_spi_bus_obj_t;

    // Fix for MicroPython > 1.21 https://github.com/ricksorensen
    #include "../../../micropy_updates/common/mp_spi_common.h"

    struct _mp_lcd_spi_bus_obj_t {
        mp_obj_base_t base;

        /* callback function that gets called after the buffer
         * has finished being sent. This only gets called after
         * sending the frame buffer
         */
        mp_obj_t callback;

        mp_obj_array_t *view1;
        mp_obj_array_t *view2;

        uint32_t buffer_flags;

        bool trans_done;
        bool rgb565_byte_swap;

        /* stores function pointers to carry out work to be done */
        lcd_panel_io_t panel_io_handle;

        /* these function pointers get set based on
         * panel_io_config.lcd_cmd_bits and panel_io_config.lcd_param_bits
         */
        void (*send_cmd)(mp_lcd_spi_bus_obj_t *self, int lcd_cmd);

        machine_hw_spi_device_obj_t *spi_device;

        void (*spi_transfer)(mp_obj_base_t *obj, size_t len, const uint8_t *src, uint8_t *dest);
        uint8_t use_dma: 1;

        mp_obj_t dc;
        uint8_t dc_low_on_data : 1;
        void (*dma_callback)(mp_lcd_spi_bus_obj_t *self);
    };

    extern const mp_obj_type_t mp_lcd_spi_bus_type;
#endif /* _SPI_BUS_H_ */