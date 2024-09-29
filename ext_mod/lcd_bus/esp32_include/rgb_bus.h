#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED
    #ifndef _ESP32_RGB_BUS_H_
        #define _ESP32_RGB_BUS_H_

        //local_includes
        #include "lcd_types.h"
        #include "esp_rgb_bus.h"

        // esp-idf includes
        #include "esp_lcd_panel_io.h"
        #include "freertos/FreeRTOS.h"
        #include "freertos/task.h"
        #include "freertos/semphr.h"

        // micropython includes
        #include "mphalport.h"
        #include "py/obj.h"
        #include "py/objarray.h"

        typedef struct _mp_lcd_rgb_bus_obj_t {
            mp_obj_base_t base;

            mp_obj_t callback;

            mp_obj_array_t *view1;
            mp_obj_array_t *view2;
            uint32_t buffer_flags;

            bool trans_done;
            bool rgb565_byte_swap;

            lcd_panel_io_t panel_io_handle;
            esp_lcd_panel_io_rgb_config_t *panel_io_config;
            esp_lcd_rgb_bus_config_t *bus_config;
            esp_lcd_rgb_bus_handle_t bus_handle;

            uint32_t buffer_size;
            uint8_t num_lanes;
            bool has_partial_buffer;
            SemaphoreHandle_t taskCopySeph;
            SemaphoreHandle_t flushCopySeph;
            TaskHandle_t copyTask;

        } mp_lcd_rgb_bus_obj_t;

        extern const mp_obj_type_t mp_lcd_rgb_bus_type;

    #endif /* _ESP32_RGB_BUS_H_ */
#else
    #include "../common_include/rgb_bus.h"

#endif /*SOC_LCD_RGB_SUPPORTED*/