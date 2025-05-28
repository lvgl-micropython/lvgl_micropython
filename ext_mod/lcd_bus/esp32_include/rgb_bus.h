// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED
    #ifndef _ESP32_RGB_BUS_H_
        #define _ESP32_RGB_BUS_H_

        //local_includes
        #include "lcd_types.h"
        #include "lcd_bus_task.h"

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

            mp_obj_t callback;

            mp_obj_array_t *view1;
            mp_obj_array_t *view2;

            uint32_t buffer_flags;

            uint8_t trans_done: 1;

            lcd_task_t task;
            lcd_init_t init;
            lcd_bufs_t bufs;

            lcd_tx_data_t tx_data;
            lcd_tx_cmds_t tx_cmds;

            rotation_data_t r_data;

            lcd_panel_io_t panel_io_handle;

            // ********************** bus specific **********************
            esp_lcd_rgb_panel_config_t panel_io_config;
            esp_lcd_rgb_timing_t bus_config;

            esp_lcd_panel_handle_t panel_handle;
            uint32_t buffer_size;

        } mp_lcd_rgb_bus_obj_t;

        extern const mp_obj_type_t mp_lcd_rgb_bus_type;

        extern void mp_lcd_rgb_bus_deinit_all(void);


    #endif /* _ESP32_RGB_BUS_H_ */
#else
    #include "../common_include/rgb_bus.h"

#endif /*SOC_LCD_RGB_SUPPORTED*/