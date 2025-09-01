/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Jim Mussared
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/mpconfig.h"
#include "py/mphal.h"
#include "modesp32.h"

#include "rom/gpio.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"

#if MICROPY_PY_MACHINE_BITSTREAM

/******************************************************************************/
// Bit-bang implementation

#define NS_TICKS_OVERHEAD (6)

// This is a translation of the cycle counter implementation in ports/stm32/machine_bitstream.c.
static void IRAM_ATTR machine_bitstream_high_low_bitbang(mp_hal_pin_obj_t pin, uint32_t *timing_ns, const uint8_t *buf, size_t len) {
    uint32_t pin_mask, gpio_reg_set, gpio_reg_clear;
    #if SOC_GPIO_PIN_COUNT > 32
    if (pin >= 32) {
        pin_mask = 1 << (pin - 32);
        gpio_reg_set = GPIO_OUT1_W1TS_REG;
        gpio_reg_clear = GPIO_OUT1_W1TC_REG;
    } else
    #endif
    {
        pin_mask = 1 << pin;
        gpio_reg_set = GPIO_OUT_W1TS_REG;
        gpio_reg_clear = GPIO_OUT_W1TC_REG;
    }

    // Convert ns to cpu ticks [high_time_0, period_0, high_time_1, period_1].
    uint32_t fcpu_mhz = esp_rom_get_cpu_ticks_per_us();
    for (size_t i = 0; i < 4; ++i) {
        timing_ns[i] = fcpu_mhz * timing_ns[i] / 1000;
        if (timing_ns[i] > NS_TICKS_OVERHEAD) {
            timing_ns[i] -= NS_TICKS_OVERHEAD;
        }
        if (i % 2 == 1) {
            // Convert low_time to period (i.e. add high_time).
            timing_ns[i] += timing_ns[i - 1];
        }
    }

    uint32_t irq_state = mp_hal_quiet_timing_enter();

    for (size_t i = 0; i < len; ++i) {
        uint8_t b = buf[i];
        for (size_t j = 0; j < 8; ++j) {
            GPIO_REG_WRITE(gpio_reg_set, pin_mask);
            uint32_t start_ticks = mp_hal_ticks_cpu();
            uint32_t *t = &timing_ns[b >> 6 & 2];
            while (mp_hal_ticks_cpu() - start_ticks < t[0]) {
                ;
            }
            GPIO_REG_WRITE(gpio_reg_clear, pin_mask);
            b <<= 1;
            while (mp_hal_ticks_cpu() - start_ticks < t[1]) {
                ;
            }
        }
    }

    mp_hal_quiet_timing_exit(irq_state);
}

/******************************************************************************/
// RMT implementation

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000

#include "driver/rmt_tx.h"
#include "driver/rmt_common.h"
#include "driver/rmt_encoder.h"


typedef struct {
    uint32_t resolution; /*!< Encoder resolution, in Hz */
    int64_t bit0_duration0;
    int64_t bit0_duration1;
    int64_t bit1_duration0;
    int64_t bit1_duration1;
    int64_t reset_duration;
} led_strip_encoder_config_t;


typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} rmt_led_strip_encoder_t;


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
    if (!(config && ret_encoder)) return ESP_ERR_INVALID_ARG;

    led_encoder = rmt_alloc_encoder_mem(sizeof(rmt_led_strip_encoder_t));
    if (!led_encoder) return ESP_ERR_NO_MEM;

    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    led_encoder->base.reset = rmt_led_strip_encoder_reset;

    uint32_t bit0_duration0 = 0;
    uint32_t bit0_duration1 = 0;
    uint32_t bit1_duration0 = 0;
    uint32_t bit1_duration1 = 0;

    if (config->bit0_duration0 < 0) bit0_duration0 = (uint32_t)(-config->bit0_duration0);
    else bit0_duration0 = (uint32_t)config->bit0_duration0;

    if (config->bit0_duration1 < 0) bit0_duration1 = (uint32_t)(-config->bit0_duration1);
    else bit0_duration1 = (uint32_t)config->bit0_duration1;

    if (config->bit1_duration0 < 0) bit1_duration0 = (uint32_t)(-config->bit1_duration0);
    else bit1_duration0 = (uint32_t)config->bit1_duration0;

    if (config->bit1_duration1 < 0) bit1_duration1 = (uint32_t)(-config->bit1_duration1);
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
        .flags.msb_first = 1
    };


    ret = rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder);
    if (ret != ESP_OK) goto err;

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ret = rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder);
    if (ret != ESP_OK) goto err;

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


// Use the reserved RMT channel to stream high/low data on the specified pin.
void machine_bitstream_high_low_rmt(mp_hal_pin_obj_t pin, uint32_t *timing_ns, const uint8_t *buf, size_t len, uint8_t channel_id) {

    ((void)channel_id);

    rmt_tx_channel_config_t channel_config = { 0 };
    channel_config.gpio_num = pin;
    channel_config.clk_src = RMT_CLK_SRC_DEFAULT;
    channel_config.resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ;
    channel_config.mem_block_symbols = 64;
    channel_config.trans_queue_depth = 1;

    led_strip_encoder_config_t encoder_config = { 0 };
    encoder_config.resolution = RMT_LED_STRIP_RESOLUTION_HZ;
    encoder_config.bit0_duration0 = (int64_t)timing_ns[0];
    encoder_config.bit0_duration1 = -(int64_t)timing_ns[1];
    encoder_config.bit1_duration0 = (int64_t)timing_ns[2];
    encoder_config.bit1_duration1 = -(int64_t)timing_ns[3];
    encoder_config.reset_duration = -50000;

    rmt_channel_handle_t channel_handle = NULL;
    check_esp_err(rmt_new_tx_channel(&channel_config, &channel_handle));

    rmt_encoder_handle_t encoder_handle = NULL;
    check_esp_err(rmt_new_led_strip_encoder(&encoder_config, &encoder_handle));
    check_esp_err(rmt_enable(channel_handle));

    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };

    check_esp_err(rmt_transmit(channel_handle, encoder_handle, buf, len, &tx_config));

    // Wait 50% longer than we expect (if every bit takes the maximum time).
    uint32_t timeout_ms = (3 * len / 2) * (1 + (8 * MAX(timing_ns[0] + timing_ns[1], timing_ns[2] + timing_ns[3])) / 1000);
    check_esp_err(rmt_tx_wait_all_done(channel_handle, pdMS_TO_TICKS(timeout_ms)));

    // Uninstall the driver.
    check_esp_err(rmt_del_led_strip_encoder(encoder_handle));
    check_esp_err(rmt_del_channel(channel_handle));

    // Cancel RMT output to GPIO pin.
    esp_rom_gpio_connect_out_signal(pin, SIG_GPIO_OUT_IDX, false, false);
}

/******************************************************************************/
// Interface to machine.bitstream

void machine_bitstream_high_low(mp_hal_pin_obj_t pin, uint32_t *timing_ns, const uint8_t *buf, size_t len) {
    if (esp32_rmt_bitstream_channel_id < 0) {
        machine_bitstream_high_low_bitbang(pin, timing_ns, buf, len);
    } else {
        machine_bitstream_high_low_rmt(pin, timing_ns, buf, len, esp32_rmt_bitstream_channel_id);
    }
}

#endif // MICROPY_PY_MACHINE_BITSTREAM
