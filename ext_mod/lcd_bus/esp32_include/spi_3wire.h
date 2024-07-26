
#ifndef _ESP32_SPI_3WIRE_H_
    #define _ESP32_SPI_3WIRE_H_

    //local_includes
    #include "lcd_types.h"

    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"

    // esp-idf includes
    #include "esp_lcd_panel_io.h"
    #include "esp_lcd_panel_io_additions.h"


    typedef struct _mp_lcd_spi_3wire_obj_t {
        mp_obj_base_t base;

        uint32_t freq;
        int cs_io_num;
        int sclk_io_num;
        int mosi_io_num;
        uint8_t flags;
        esp_lcd_panel_io_handle_t panel_io;
    } mp_lcd_spi_3wire_obj_t;

    extern const mp_obj_type_t mp_lcd_spi_3wire_type;

#endif /* _ESP32_SPI_3WIRE_H_ */