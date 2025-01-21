// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#ifndef _ESP32_LED_BUS_H_
    #define _ESP32_LED_BUS_H_

    //local_includes
    #include "common/lcd_types.h"
    #include "common/lcd_framebuf.h"
    #include "common/sw_rotate.h"

    #include "../../../../micropy_updates/common/mp_spi_common.h"

    #include "driver/rmt.h"

    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"
    #include "py/objarray.h"

    typedef enum lcd_led_pixel_order {
        RGB,
        RBG,
        GRB,
        GBR,
        BRG,
        BGR,
    } lcd_led_pixel_order;

    typedef struct _mp_lcd_led_bit_timing {
        int duration1;
        int duration2;
    } lcd_led_bit_timing;

    typedef struct {
        rmt_encoder_t base;
        rmt_encoder_t *bytes_encoder;
        rmt_encoder_t *copy_encoder;
        int state;
        rmt_symbol_word_t reset_code;
    } lcd_led_strip_encoder_t;

    typedef struct _lcd_led_color_temp {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        bool blue_correct;
    } lcd_led_color_temp;

    typedef struct _mp_lcd_led_bus_obj_t {
        mp_obj_base_t base;
        lcd_panel_io_t panel_io_handle;

        mp_obj_t callback;

        mp_lcd_framebuf_t *fb1;
        mp_lcd_framebuf_t *fb2;

        uint8_t trans_done: 1;
        uint8_t rgb565_byte_swap: 1;
        uint8_t sw_rotate: 1;

        mp_lcd_sw_rotation_t *sw_rot;

        lcd_led_pixel_order *pixel_order;
        lcd_led_color_temp *color_temp;

        /* specific to bus */
        // common config
        uint32_t buffer_size;
        uint8_t rgb_order[3];
        uint16_t pixel_count;
        uint8_t leds_per_pixel;


        // RMT specific config
        esp_gpio_t data_pin;
        lcd_led_bit_timing bit0;
        lcd_led_bit_timing bit1;
        uint16_t res;
        rmt_channel_handle_t rmt_chan;
        lcd_led_strip_encoder_t *strip_encoder

        // SPI specific config
        esp_lcd_panel_io_spi_config_t *panel_io_config;
        esp_lcd_spi_bus_handle_t *bus_handle;
        machine_hw_spi_device_obj_t *spi_device;

    } mp_lcd_led_bus_obj_t;


    extern const mp_obj_type_t mp_lcd_led_bus_type;

#endif /* _ESP32_LED_BUS_H_ */

