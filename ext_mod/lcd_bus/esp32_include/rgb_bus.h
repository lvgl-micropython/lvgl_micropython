#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED
    #ifndef _ESP32_RGB_BUS_H_
        #define _ESP32_RGB_BUS_H_

        //local_includes
        #include "lcd_types.h"

        // esp-idf includes
        #include "hal/lcd_hal.h"
        #include "esp_pm.h"
        #include "esp_intr_alloc.h"
        #include "esp_heap_caps.h"

        #include "esp_lcd_panel_io.h"
        #include "esp_lcd_panel_ops.h"
        #include "esp_lcd_panel_interface.h"
        #include "esp_lcd_panel_rgb.h"

        #include "freertos/FreeRTOS.h"
        #include "freertos/task.h"
        #include "freertos/semphr.h"
        #include "freertos/event_groups.h"
        #include "freertos/idf_additions.h"

        // micropython includes
        #include "mphalport.h"
        #include "py/obj.h"
        #include "py/objarray.h"


        typedef struct _mp_lcd_rgb_bus_obj_t {
            mp_obj_base_t base;

            rotation_t *rotation;

            mp_obj_t callback;

            mp_obj_array_t *view1;
            mp_obj_array_t *view2;

            uint32_t buffer_flags;

            uint8_t lane_count : 5;
            uint8_t trans_done : 1;
            uint8_t rgb565_byte_swap : 1;

            lcd_panel_io_t panel_io_handle;

            esp_lcd_rgb_panel_config_t *panel_io_config;
            esp_lcd_panel_handle_t panel_handle;
            uint32_t buffer_size;

        } mp_lcd_rgb_bus_obj_t;



        extern const mp_obj_type_t mp_lcd_rgb_bus_type;

    #endif /* _ESP32_RGB_BUS_H_ */
#else
    #include "../common_include/rgb_bus.h"

#endif /*SOC_LCD_RGB_SUPPORTED*/