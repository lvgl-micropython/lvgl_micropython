/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <stdint.h>
#include "driver/rmt_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _led_strip_color_order_t {
   LED_STRIP_RGB,
   LED_STRIP_GRB,
   LED_STRIP_RGBW,
   LED_STRIP_GRBW
} led_strip_color_order_t;

typedef enum _led_strip_byte_order_t {
   LED_STRIP_LSB,
   LED_STRIP_MSB
} led_strip_byte_order_t;


/**
 * @brief Type of led strip encoder configuration
 */
typedef struct {
    uint32_t resolution; /*!< Encoder resolution, in Hz */
    int32_t bit0_duration0;
    int32_t bit0_duration1;
    int32_t bit1_duration0;
    int32_t bit1_duration1;
    int32_t reset_duration;
    led_strip_color_order_t color_order;
    led_strip_byte_order_t byte_order;
} led_strip_encoder_config_t;

/**
 * @brief Create RMT encoder for encoding LED strip pixels into RMT symbols
 *
 * @param[in] config Encoder configuration
 * @param[out] ret_encoder Returned encoder handle
 * @return
 *      - ESP_ERR_INVALID_ARG for any invalid arguments
 *      - ESP_ERR_NO_MEM out of memory when creating led strip encoder
 *      - ESP_OK if creating encoder successfully
 */
esp_err_t rmt_new_led_strip_encoder(const led_strip_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);

#ifdef __cplusplus
}
#endif