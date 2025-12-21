// Copyright (c) 2024 - 2025 Kevin G. Schlosser

//local_includes
#include "lcd_types.h"
#include "../../../micropy_updates/common/mp_spi_common.h"

#include "driver/rmt.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/objarray.h"


#ifndef __LED_H__
    #define __LED_H__

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

        mp_obj_array_t *view1;
        mp_obj_array_t *view2;

        lcd_funcs_t funcs;

        lcd_rotate_buffers_t buffers;
        lcd_rotate_data_t rot_data;
        lcd_rotate_task_t task;
        lcd_rotate_task_init_t *task_init;

        // port & bus specific fields below
        TaskHandle_t task_handle;

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

#endif

