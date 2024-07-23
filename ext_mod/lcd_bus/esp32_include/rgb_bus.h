#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED
    #ifndef _ESP32_RGB_BUS_H_
        #define _ESP32_RGB_BUS_H_

        //local_includes
        #include "lcd_types.h"

        // esp-idf includes
        #include "esp_lcd_panel_io.h"
        #include "esp_lcd_panel_rgb.h"

        // micropython includes
        #include "mphalport.h"
        #include "py/obj.h"
        #include "py/objarray.h"
        #include "soc/soc_caps.h"

        typedef struct _mp_lcd_rgb_bus_obj_t {
            mp_obj_base_t base;

            mp_obj_t callback;

            void *buf1;
            void *buf2;
            uint32_t buffer_flags;

            bool trans_done;
            bool rgb565_byte_swap;

            lcd_panel_io_t panel_io_handle;

            esp_lcd_rgb_panel_config_t panel_io_config;
            esp_lcd_rgb_timing_t bus_config;

            esp_lcd_panel_handle_t panel_handle;
            uint32_t buffer_size;
            mp_obj_array_t *view1;
            mp_obj_array_t *view2;

            void *transmitting_buf;

        } mp_lcd_rgb_bus_obj_t;


        extern const mp_obj_type_t mp_lcd_rgb_bus_type;

    #endif /* _ESP32_RGB_BUS_H_ */
#else
    #include "../common_include/rgb_bus.h"

#endif /*SOC_LCD_RGB_SUPPORTED*/