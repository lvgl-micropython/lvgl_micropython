// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED
    #ifndef __RGB_H__
        #define __RGB_H__

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
        #include "esp_private/gdma.h"
        #include "esp_private/gdma_link.h"

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
            size_t dma_burst_size;  // DMA transfer burst size
            // removed in ESP-IDF 5.5
            // int disp_gpio_num;     // Display control GPIO, which is used to perform action like "disp_off"
            intr_handle_t intr;    // LCD peripheral interrupt handle
            esp_pm_lock_handle_t pm_lock; // Power management lock
            size_t num_dma_nodes;  // Number of DMA descriptors that used to carry the frame buffer
            gdma_channel_handle_t dma_chan; // DMA channel handle
            gdma_link_list_handle_t dma_fb_links[3]; // DMA link lists for multiple frame buffers
            gdma_link_list_handle_t dma_bb_link; // DMA link list for bounce buffer
        #if CONFIG_IDF_TARGET_ESP32S3
            gdma_link_list_handle_t dma_restart_link; // DMA link list for restarting the DMA
        #endif
            uint8_t *fbs[3]; // Frame buffers
            uint8_t *bounce_buffer[2]; // Pointer to the bounce buffers
            size_t fb_size;        // Size of frame buffer, in bytes
            size_t bb_size;        // Size of the bounce buffer, in bytes. If not-zero, the driver uses two bounce buffers allocated from internal memory
            uint8_t cur_fb_index;  // Current frame buffer index
        } rgb_panel_t;


        typedef struct _mp_lcd_rgb_bus_obj_t {
            mp_obj_base_t base;

            mp_obj_t callback;

            mp_obj_array_t *view1;
            mp_obj_array_t *view2;

            lcd_funcs_t funcs;

            lcd_rotate_buffers_t buffers;
            lcd_rotate_data_t rot_data;
            lcd_rotate_task_t task;
            lcd_rotate_task_init_t *task_init;

            // port & bus specific fields below
            esp_lcd_panel_io_handle_t panel_io;
            TaskHandle_t task_handle;


            esp_lcd_rgb_panel_config_t panel_io_config;
            esp_lcd_rgb_timing_t bus_config;

            esp_lcd_panel_handle_t panel_handle;
            uint32_t buffer_size;

        } mp_lcd_rgb_bus_obj_t;

    #endif
#else
    #include "../../unsupported/inc/rgb.h"

#endif