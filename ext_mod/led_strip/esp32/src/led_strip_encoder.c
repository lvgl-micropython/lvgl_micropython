/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_check.h"
#include "led_strip_encoder.h"

static const char *TAG = "led_encoder";

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;


RMT_ENCODER_FUNC_ATTR
static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;
    switch (led_encoder->state) {
        case 0: // send RGB data
            encoded_symbols += bytes_encoder->encode(bytes_encoder, channel, primary_data, data_size, &session_state);
            if (session_state & RMT_ENCODING_COMPLETE) {
                led_encoder->state = 1; // switch to next state when current encoding session finished
            }
            if (session_state & RMT_ENCODING_MEM_FULL) {
                state |= RMT_ENCODING_MEM_FULL;
                goto out; // yield if there's no free space for encoding artifacts
            }
        // fall-through
        case 1: // send reset code
            encoded_symbols += copy_encoder->encode(copy_encoder, channel, &led_encoder->reset_code,
                                                    sizeof(led_encoder->reset_code), &session_state);
            if (session_state & RMT_ENCODING_COMPLETE) {
                led_encoder->state = RMT_ENCODING_RESET; // back to the initial encoding session
                state |= RMT_ENCODING_COMPLETE;
            }
            if (session_state & RMT_ENCODING_MEM_FULL) {
                state |= RMT_ENCODING_MEM_FULL;
                goto out; // yield if there's no free space for encoding artifacts
            }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}


static esp_err_t rmt_del_led_strip_encoder(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}


RMT_ENCODER_FUNC_ATTR
static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}


esp_err_t rmt_new_led_strip_encoder(const led_strip_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_led_strip_encoder_t *led_encoder = NULL;
    ESP_GOTO_ON_FALSE(config && ret_encoder, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    led_encoder = rmt_alloc_encoder_mem(sizeof(rmt_led_strip_encoder_t));
    ESP_GOTO_ON_FALSE(led_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for led strip encoder");
    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    led_encoder->base.reset = rmt_led_strip_encoder_reset;

    uint32_t bit0_duration0;
    uint32_t bit0_duration1;

    uint32_t bit1_duration0;
    uint32_t bit2_duration1;


    if (config->bit0_duration0 < 0) bit0_duration0 = (uint32_t)-config->bit0_duration0;
    else bit0_duration0 = (uint32_t)config->bit0_duration0;

    if (config->bit0_duration1 < 0) bit0_duration1 = (uint32_t)-config->bit0_duration1;
    else bit0_duration1 = (uint32_t)config->bit0_duration1;

    if (config->bit1_duration0 < 0) bit1_duration0 = (uint32_t)-config->bit1_duration0;
    else bit1_duration0 = (uint32_t)config->bit1_duration0;

    if (config->bit1_duration1 < 0) bit1_duration1 = (uint32_t)-config->bit1_duration1;
    else bit1_duration1 = (uint32_t)config->bit1_duration1;

    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = config->bit0_duration0 < 0 ? 0 : 1,
            .duration0 = bit0_duration0 * config->resolution / 1000000000,
            .level1 = config->bit0_duration1 < 0 ? 0 : 1,
            .duration1 = bit0_duration1 * config->resolution / 1000000000,
        },
        .bit1 = {
            .level0 = config->bit1_duration0 < 0 ? 0 : 1,
            .duration0 = bit1_duration0 * config->resolution / 1000000000,
            .level1 = config->bit1_duration1 < 0 ? 0 : 1,
            .duration1 = bit1_duration1 * config->resolution / 1000000000,
        },
        .flags.msb_first = config->bit_order // WS2812 transfer bit order: G7...G0R7...R0B7...B0
    };


    ESP_GOTO_ON_ERROR(rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder), err, TAG, "create bytes encoder failed");
    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder), err, TAG, "create copy encoder failed");

    uint32_t reset_duration;

    if (config->reset_duration < 0) reset_duration = (uint32_t)-config->reset_duration;
    else reset_duration = (uint32_t)config->reset_duration;

    led_encoder->reset_code = (rmt_symbol_word_t) {
        .level0 = config->reset_duration < 0 ? 0 : 1,
        .duration0 = reset_duration * config->resolution / 1000000000 / 2,
        .level1 = config->reset_duration < 0 ? 0 : 1,
        .duration1 = reset_duration * config->resolution / 1000000000 / 2,
    };

    *ret_encoder = &led_encoder->base;
    return ESP_OK;
err:
    if (led_encoder) {
        if (led_encoder->bytes_encoder) {
            rmt_del_encoder(led_encoder->bytes_encoder);
        }
        if (led_encoder->copy_encoder) {
            rmt_del_encoder(led_encoder->copy_encoder);
        }
        free(led_encoder);
    }
    return ret;
}