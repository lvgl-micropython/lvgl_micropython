// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED
    #ifndef _ESP32_RGB_BUS_H_
        #define _ESP32_RGB_BUS_H_

        //local_includes
        #include "common/lcd_types.h"
        #include "common/lcd_framebuf.h"
        #include "common/sw_rotate.h"

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

        typedef struct {
            esp_lcd_panel_t base;  // Base class of generic lcd panel
            int panel_id;          // LCD panel ID
            lcd_hal_context_t hal; // Hal layer object
            size_t data_width;     // Number of data lines
            size_t fb_bits_per_pixel; // Frame buffer color depth, in bpp
            size_t num_fbs;           // Number of frame buffers
            size_t output_bits_per_pixel; // Color depth seen from the output data line. Default to fb_bits_per_pixel, but can be changed by YUV-RGB conversion
            size_t sram_trans_align;  // Alignment for framebuffer that allocated in SRAM
            size_t psram_trans_align; // Alignment for framebuffer that allocated in PSRAM
            int disp_gpio_num;     // Display control GPIO, which is used to perform action like "disp_off"
            intr_handle_t intr;    // LCD peripheral interrupt handle
            esp_pm_lock_handle_t pm_lock; // Power management lock
            size_t num_dma_nodes;  // Number of DMA descriptors that used to carry the frame buffer
            uint8_t *fbs[3]; // Frame buffers
            uint8_t cur_fb_index;  // Current frame buffer index
            uint8_t bb_fb_index;  // Current frame buffer index which used by bounce buffer
        } rgb_panel_t;

        typedef struct _mp_lcd_rgb_bus_obj_t {
            mp_obj_base_t base;
            lcd_panel_io_t panel_io_handle;

            mp_obj_t callback;

            mp_lcd_framebuf_t *fb1;
            mp_lcd_framebuf_t *fb2;

            uint8_t trans_done: 1;
            uint8_t rgb565_byte_swap: 1;
            uint8_t sw_rotate: 1;

            mp_lcd_sw_rotation_t *sw_rot;

            esp_lcd_rgb_panel_config_t *panel_io_config;
            void *padding;

            /* specific to bus */
            esp_lcd_panel_handle_t panel_handle;

        } mp_lcd_rgb_bus_obj_t;

        extern const mp_obj_type_t mp_lcd_rgb_bus_type;

        extern void mp_lcd_rgb_bus_deinit_all(void);


    #endif /* _ESP32_RGB_BUS_H_ */
#else
    #include "../common_include/rgb_bus.h"

#endif /*SOC_LCD_RGB_SUPPORTED*/