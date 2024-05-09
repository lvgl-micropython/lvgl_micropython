

#ifndef _SPI_PANEL_BUS_H_
#define _SPI_PANEL_BUS_H_
    #include <stdbool.h>
    #include "esp_err.h"
    #include "soc/soc_caps.h"
    #include "hal/lcd_types.h"

    #include "esp_lcd_types.h"
    #include "esp_lcd_panel_io_interface.h"
    #include "esp_lcd_panel_io.h"
    #include "../../../lib/esp-idf/components/esp_lcd/src/esp_lcd_common.h"

    esp_err_t lcdbus_new_panel_io_spi(esp_lcd_spi_bus_handle_t bus, const esp_lcd_panel_io_spi_config_t *io_config, esp_lcd_panel_io_handle_t *ret_io, uint8_t double_buffer);

#endif /* _SPI_PANEL_BUS_H_ */


