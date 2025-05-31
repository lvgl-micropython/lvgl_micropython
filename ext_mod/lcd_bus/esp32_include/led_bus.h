// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#ifndef _ESP32_LED_BUS_H_
    #define _ESP32_LED_BUS_H_

    //local_includes
    #include "lcd_types.h"
    #include "lcd_bus_task.h"

    #include "../../../micropy_updates/common/mp_spi_common.h"

    #include "driver/rmt.h"
    #include "driver/rmt_common.h"
    #include "driver/rmt_encoder.h"
    #include "driver/rmt_tx.h"
    #include "driver/rmt_types.h"
    #include "esp_lcd_panel_io.h"


    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"
    #include "py/objarray.h"


    typedef enum mp_lcd_led_pixel_order {
        RGB = 0x06,
        RBG = 0x09,
        GRB = 0x12,
        GBR = 0x18,
        BRG = 0x21,
        BGR = 0x24,
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
    } mp_rmt_led_strip_encoder_t;

    typedef union {
        struct {
            uint8_t reserved: 2;
            uint8_t index0: 2;
            uint8_t index1: 2;
            uint8_t index2: 2;
        };
        uint8_t value;
    } rgb_order_t;

    typedef struct _mp_lcd_led_color_temp {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t blue_correct: 1;
    } mp_lcd_led_color_temp;

    typedef struct _mp_lcd_led_bus_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        mp_obj_array_t *view1;
        mp_obj_array_t *view2;

        uint32_t buffer_flags;

        uint8_t trans_done: 1;
        uint8_t num_lanes: 5;

        lcd_task_t task;
        lcd_init_t init;
        lcd_bufs_t bufs;

        lcd_tx_data_t tx_data;
        lcd_tx_cmds_t tx_cmds;

        rotation_data_t r_data;

        internal_cb_funcs_t internal_cb_funcs;

        // ********************** bus specific **********************
        uint32_t buffer_size;

        // common config
        rgb_order_t pixel_order;
        uint16_t pixel_count;
        uint8_t leds_per_pixel;
        mp_lcd_led_color_temp color_temp;
        uint32_t freq;
        uint8_t msb_first: 1;

        // RMT specific config
        int data_pin;
        mp_lcd_led_bit_timing bit0;
        mp_lcd_led_bit_timing bit1;
        int res;
        rmt_channel_handle_t rmt_chan;
        rmt_encoder_handle_t led_encoder;

        // SPI specific config
        esp_lcd_panel_io_handle_t panel_io_handle;
        esp_lcd_panel_io_spi_config_t *panel_io_config;
        esp_lcd_spi_bus_handle_t *bus_handle;
        mp_machine_hw_spi_device_obj_t *spi_device;

    } mp_lcd_led_bus_obj_t;


    extern const mp_obj_type_t mp_lcd_led_bus_type;

#endif /* _ESP32_LED_BUS_H_ */

