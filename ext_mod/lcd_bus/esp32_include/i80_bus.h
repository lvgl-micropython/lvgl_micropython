#ifndef _ESP32_I80_BUS_H_
    #define _ESP32_I80_BUS_H_

    //local_includes
    #include "lcd_types.h"

    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"
    #include "py/objarray.h"

    // esp-idf includes
    #include "soc/soc_caps.h"

    #if SOC_LCD_I80_SUPPORTED
        // esp-idf includes
        #include "esp_lcd_panel_io.h"


        typedef struct _mp_lcd_i80_bus_obj_t {
            mp_obj_base_t base;

            rotation_t *rotation;

            mp_obj_t callback;

            mp_obj_array_t *view1;
            mp_obj_array_t *view2;
            uint32_t buffer_flags;

            uint8_t trans_done : 1;
            uint8_t rgb565_byte_swap : 1;

            lcd_panel_io_t panel_io_handle;

            esp_lcd_panel_io_i80_config_t panel_io_config;
            esp_lcd_i80_bus_config_t bus_config;
            esp_lcd_i80_bus_handle_t bus_handle;
        } mp_lcd_i80_bus_obj_t;

        extern const mp_obj_type_t mp_lcd_i80_bus_type;

    #else
        #include "../common_include/i80_bus.h"

    #endif /* SOC_LCD_I80_SUPPORTED */
#endif /* _ESP32_I80_BUS_H_ */
