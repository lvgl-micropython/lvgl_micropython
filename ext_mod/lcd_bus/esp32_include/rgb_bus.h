// Copyright (c) 2024 - 2025 Kevin G. Schlosser

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
            int disp_gpio_num;     // Display control GPIO, which is used to perform action like "disp_off"
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

        typedef struct _rgb_bus_lock_t {
            SemaphoreHandle_t handle;
            StaticSemaphore_t buffer;
        } rgb_bus_lock_t;

        typedef struct _rgb_bus_event_t {
            EventGroupHandle_t handle;
            StaticEventGroup_t buffer;
        } rgb_bus_event_t;

        typedef struct _mp_lcd_rgb_bus_obj_t {
            mp_obj_base_t base;

            mp_obj_t callback;

            mp_obj_array_t *view1;
            mp_obj_array_t *view2;

            uint32_t buffer_flags;

            bool trans_done;
            bool rgb565_byte_swap;

            lcd_panel_io_t panel_io_handle;

            esp_lcd_rgb_panel_config_t panel_io_config;
            esp_lcd_rgb_timing_t bus_config;

            esp_lcd_panel_handle_t panel_handle;
            uint32_t buffer_size;

            uint8_t *active_fb;
            uint8_t *idle_fb;
            uint8_t *partial_buf;

            int x_start;
            int y_start;
            int x_end;
            int y_end;
            uint16_t width;
            uint16_t height;
            uint8_t rotation: 2;
            uint8_t bytes_per_pixel: 2;
            uint8_t last_update: 1;
            uint8_t rgb565_dither: 1;

            rgb_bus_lock_t copy_lock;
            rgb_bus_event_t copy_task_exit;
            rgb_bus_lock_t tx_color_lock;
            rgb_bus_event_t swap_bufs;
            rgb_bus_lock_t init_lock;

            TaskHandle_t copy_task_handle;

            mp_lcd_err_t init_err;
            mp_rom_error_text_t init_err_msg;

        } mp_lcd_rgb_bus_obj_t;

        void rgb_bus_event_init(rgb_bus_event_t *event);
        void rgb_bus_event_delete(rgb_bus_event_t *event);
        bool rgb_bus_event_isset(rgb_bus_event_t *event);
        void rgb_bus_event_set(rgb_bus_event_t *event);
        void rgb_bus_event_clear(rgb_bus_event_t *event);
        void rgb_bus_event_clear_from_isr(rgb_bus_event_t *event);
        bool rgb_bus_event_isset_from_isr(rgb_bus_event_t *event);
        void rgb_bus_event_set_from_isr(rgb_bus_event_t *event);
        void rgb_bus_event_wait(rgb_bus_event_t *event);

        int  rgb_bus_lock_acquire(rgb_bus_lock_t *lock, int32_t wait_ms);
        void rgb_bus_lock_release(rgb_bus_lock_t *lock);
        void rgb_bus_lock_init(rgb_bus_lock_t *lock);
        void rgb_bus_lock_delete(rgb_bus_lock_t *lock);
        void rgb_bus_lock_release_from_isr(rgb_bus_lock_t *lock);

        void rgb_bus_copy_task(void *self_in);

        extern const mp_obj_type_t mp_lcd_rgb_bus_type;

        extern void mp_lcd_rgb_bus_deinit_all(void);

    #endif /* _ESP32_RGB_BUS_H_ */
#else
    #include "../common_include/rgb_bus.h"

#endif /*SOC_LCD_RGB_SUPPORTED*/