
#ifndef __ESP_RGB_BUS_H__
#define __ESP_RGB_BUS_H__

#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED

    #include <stdint.h>
    #include <stdbool.h>
    #include "esp_err.h"
    #include "esp_lcd_types.h"
    #include "soc/soc_caps.h"
    #include "hal/lcd_types.h"

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"

    #include "esp_attr.h"
    #include "esp_check.h"
    #include "esp_pm.h"
    #include "esp_lcd_panel_interface.h"
    #include "esp_lcd_panel_ops.h"
    #include "esp_lcd_panel_io_interface.h"

    #include "esp_rom_gpio.h"
    #include "soc/soc_caps.h"
    #include "esp_clk_tree.h"
    #include "hal/dma_types.h"
    #include "hal/gpio_hal.h"
    #include "esp_private/gdma.h"
    #include "driver/gpio.h"
    #include "esp_bit_defs.h"
    #include "esp_private/periph_ctrl.h"
    #include "esp_psram.h"
    #include "esp_lcd_common.h"
    #include "soc/lcd_periph.h"
    #include "hal/lcd_hal.h"
    #include "hal/lcd_ll.h"
    #include "hal/gdma_ll.h"
    #include "rom/cache.h"

    #define RGB_BUS_LCD_PANEL_MAX_FB_NUM         3 // maximum supported frame buffer number
    #define RGB_BUS_LCD_PANEL_DMA_LINKS_REPLICA  RGB_BUS_LCD_PANEL_MAX_FB_NUM

    typedef struct {
        uint32_t pclk_hz;
        uint32_t hsync_pulse_width;
        uint32_t hsync_back_porch;
        uint32_t hsync_front_porch;
        uint32_t vsync_pulse_width;
        uint32_t vsync_back_porch;
        uint32_t vsync_front_porch;
        struct _esp_lcd_panel_io_rgb_config_flags_t {
            uint32_t hsync_idle_low: 1;
            uint32_t vsync_idle_low: 1;
            uint32_t de_idle_high: 1;
        } flags;
    } esp_lcd_panel_io_rgb_config_t;


    typedef struct {
        lcd_clock_source_t clk_src;
        size_t data_width;
        size_t bits_per_pixel;
        size_t num_fbs;
        uint32_t h_res;
        uint32_t v_res;
        size_t sram_trans_align;
        size_t psram_trans_align;
        int hsync_gpio_num;
        int vsync_gpio_num;
        int de_gpio_num;
        int pclk_gpio_num;
        int data_gpio_nums[SOC_LCD_RGB_DATA_WIDTH];
        struct _esp_lcd_rgb_bus_config_flags_t {
            uint32_t pclk_active_neg: 1;
            uint32_t pclk_idle_high: 1;
            uint32_t refresh_on_demand: 1;
            uint32_t fb_in_psram: 1;
            uint32_t double_fb: 1;
            uint32_t no_fb: 1;
        } flags;
    } esp_lcd_rgb_bus_config_t;


    typedef struct _esp_lcd_rgb_bus_t {
        esp_lcd_panel_io_t *panel_io;
        int panel_id;
        lcd_hal_context_t hal;
        size_t data_width;
        size_t fb_bits_per_pixel;
        size_t num_fbs;
        uint32_t h_res;
        uint32_t v_res;
        size_t output_bits_per_pixel;
        size_t sram_trans_align;
        size_t psram_trans_align;
        intr_handle_t intr;
        esp_pm_lock_handle_t pm_lock;
        size_t num_dma_nodes;
        uint8_t *fbs[RGB_BUS_LCD_PANEL_MAX_FB_NUM];
        uint8_t cur_fb_index;
        uint8_t *cur_buf;
        bool swapped_framebuf;
        size_t fb_size;
        int data_gpio_nums[SOC_LCD_RGB_DATA_WIDTH];
        uint32_t src_clk_hz;
        uint32_t pclk_hz;
        size_t expect_eof_count;
        gdma_channel_handle_t dma_chan;
        esp_lcd_panel_io_color_trans_done_cb_t on_vsync;
        void *user_ctx;
        portMUX_TYPE spinlock;
        int lcd_clk_flags;
        struct _esp_lcd_rgb_bus_flags_t {
            uint32_t stream_mode: 1;
            uint32_t fb_in_psram: 1;
            uint32_t pclk_active_neg: 1;
            uint32_t pclk_idle_high: 1;
            uint32_t need_update_pclk: 1;
            uint32_t need_restart: 1;
        } flags;
        dma_descriptor_t *dma_links[RGB_BUS_LCD_PANEL_DMA_LINKS_REPLICA];
        dma_descriptor_t dma_restart_node;
        dma_descriptor_t dma_nodes[];
    } esp_lcd_rgb_bus_t;


    typedef esp_lcd_rgb_bus_t *esp_lcd_rgb_bus_handle_t;


    typedef struct {
        esp_lcd_panel_io_t base;
        esp_lcd_rgb_bus_handle_t bus;
    } lcd_panel_io_rgb_t;


    esp_err_t esp_lcd_new_rgb_bus(const esp_lcd_rgb_bus_config_t *bus_config, esp_lcd_rgb_bus_handle_t *ret_bus);
    esp_err_t esp_lcd_new_panel_io_rgb(esp_lcd_rgb_bus_handle_t bus, const esp_lcd_panel_io_rgb_config_t *io_config, esp_lcd_panel_io_handle_t *ret_io);

    esp_err_t esp_lcd_del_rgb_bus(esp_lcd_rgb_bus_handle_t rgb_bus);

#endif // SOC_LCD_RGB_SUPPORTED
#endif /* __ESP_RGB_BUS_H__ */