// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#ifndef _ESP32_LED_BUS_H_
    #define _ESP32_LED_BUS_H_

    //local_includes
    #include "lcd_types.h"
    #include "../../../micropy_updates/common/mp_spi_common.h"

    #include "driver/rmt.h"

    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"
    #include "py/objarray.h"

    typedef enum mp_lcd_led_pixel_order {
        RGB,
        RBG,
        GRB,
        GBR,
        BRG,
        BGR,
    } mp_lcd_led_pixel_order;

    typedef struct _mp_lcd_led_bit_timing {
        int duration1;
        int duration2;
    } mp_lcd_led_bit_timing;

    typedef struct {
        rmt_encoder_t base;
        rmt_encoder_t *bytes_encoder;
        rmt_encoder_t *copy_encoder;
        int state;
        rmt_symbol_word_t reset_code;
    } mp_lcd_led_strip_encoder_t;

    typedef struct _mp_lcd_led_color_temp {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        bool blue_correct;
    } mp_lcd_led_color_temp;

    typedef struct _mp_lcd_led_bus_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        void *buf1;
        void *buf2;
        uint32_t buffer_flags;

        bool trans_done;
        bool rgb565_byte_swap;

        lcd_panel_io_t panel_io_handle;

        uint32_t buffer_size;

        mp_obj_array_t *view1;
        mp_obj_array_t *view2;

        // common config
        mp_lcd_led_pixel_order pixel_order;
        uint8_t rgb_order[3];
        uint16_t pixel_count;
        uint8_t leds_per_pixel;
        mp_lcd_led_color_temp color_temp;

        // RMT specific config
        esp_gpio_t data_pin;
        mp_lcd_led_bit_timing bit0;
        mp_lcd_led_bit_timing bit1;
        uint16_t res;
        rmt_channel_handle_t rmt_chan;
        mp_lcd_led_strip_encoder_t *strip_encoder

        // SPI specific config
        esp_lcd_panel_io_spi_config_t *panel_io_config;
        esp_lcd_spi_bus_handle_t *bus_handle;
        machine_hw_spi_device_obj_t *spi_device;

    } mp_lcd_led_bus_obj_t;


    extern const mp_obj_type_t mp_lcd_led_bus_type;

#endif /* _ESP32_LED_BUS_H_ */

