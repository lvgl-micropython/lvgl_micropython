#ifndef _ESP32_SPI_BUS_H_
    #define _ESP32_SPI_BUS_H_

    //local_includes
    #include "lcd_types.h"
    #include "esp32_spi_bus.h"

    // esp-idf includes
    #include "esp_lcd_panel_io.h"
    #include "driver/spi_common.h"
    #include "driver/spi_master.h"
    #include "hal/spi_types.h"

    // micropython includes
    #include "py/objarray.h"
    #include "mphalport.h"
    #include "py/obj.h"


    typedef struct _mp_lcd_spi_bus_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        void *buf1;
        void *buf2;
        uint32_t buffer_flags;

        bool trans_done;
        bool rgb565_byte_swap;

        lcd_panel_io_t panel_io_handle;
        esp_lcd_panel_io_spi_config_t panel_io_config;
        esp32_hw_spi_bus_obj_t *spi_bus;
        esp_lcd_spi_bus_handle_t bus_handle;

    } mp_lcd_spi_bus_obj_t;

    extern const mp_obj_type_t mp_lcd_spi_bus_type;
#endif /* _ESP32_SPI_BUS_H_ */

