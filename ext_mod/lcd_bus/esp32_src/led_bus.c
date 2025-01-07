// Copyright (c) 2024 - 2025 Kevin G. Schlosser


static bool led_rmt_bus_trans_done_cb(rmt_channel_handle_t tx_chan, const rmt_tx_done_event_data_t *edata, void *user_ctx)
{
    mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *)user_ctx;
    LCD_UNUSED(tx_chan);
    LCD_UNUSED(edata);

    if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
        cb_isr(self->callback);
    }
    self->trans_done = true;
    return false;
}


#include "driver/rmt_common.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_types.h"



static void c_temp2rgb(uint16_t kelvin, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint16_t temp = kelvin / 100;

    if (temp <= 66 ) {
        *r = 255;
        *g = (uint8_t)(99.4708025861f * log(temp) - 161.1195681661f);
        if (temp <= 19) {
            *b = 0;
        } else {
            *b = (uint8_t)(138.5177312231f * log(temp - 10) - 305.0447927307f);
        }
    } else {
        *r = (uint8_t)(329.698727446f * powf((float)(temp - 60), -0.1332047592f));
        *g = (uint8_t)(288.1221695283f * powf((float)(temp - 60), -0.0755148492f));
        *b = 255;
    }
}

#define LED_MIN(x, y) x < y ? x : y

// The  RGB to RGBW conversion function.
static void rgb2rgbw(mp_lcd_led_color_temp *color_temp, uint8_t rgbw[])
{
    // Calculate all of the color's white values corrected taking into account the white color temperature.
    float wRed = (float)(rgbw[0]) * (255.0f / (float)color_temp->r);
    float wGreen = (float)(rgbw[1]) * (255.0f / (float)color_temp->g);
    float wBlue = (float)(rgbw[2]) * (255.0f / (float)color_temp->b);
    
    // Determine the smallest white value from above.
    uint8_t wMin = roundf(LED_MIN(wRed, LED_MIN(wGreen, wBlue)));
    
    // Make the color with the smallest white value to be the output white value
    if (wMin == wRed) {
        rgbw[3] = rgbw[0];
    } else if (wMin == wGreen) {
        rgbw[3] = rgbw[1];
    } else {
        rgbw[3] = rgbw[2];
    }

    rgbw[0] = round(rgbw[0] - rgbw[3] * (color_temp->r / 255));
    rgbw[1] = round(rgbw[1] - rgbw[3] * (color_temp->g / 255));
    rgbw[2] = round(rgbw[2] - rgbw[3] * (color_temp->b / 255));

    if (color_temp->blue_correct) *w = (*w) - (*b) * 0.2;
}


void led_spi_deinit_callback(machine_hw_spi_device_obj_t *device);


mp_lcd_err_t led_del(mp_obj_t obj);
mp_lcd_err_t led_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
mp_lcd_err_t led_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
mp_lcd_err_t led_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
mp_lcd_err_t led_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
mp_lcd_err_t led_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update);
mp_obj_t led_allocate_framebuffer(mp_obj_t obj, uint32_t size, uint32_t caps);
mp_obj_t led_free_framebuffer(mp_obj_t obj, mp_obj_t buf);


static size_t led_encode_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_handle_t bytes_encoder = led_encoder->bytes_encoder;
    rmt_encoder_handle_t copy_encoder = led_encoder->copy_encoder;
    rmt_encode_state_t session_state = 0;
    rmt_encode_state_t state = 0;
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
                led_encoder->state = 0; // back to the initial encoding session
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

static esp_err_t led_del_strip(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}

static esp_err_t led_reset_strip(rmt_encoder_t *encoder)
{
    rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = 0;
    return ESP_OK;
}
    
    
static mp_obj_t mp_lcd_led_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum {
        ARG_data_pin,
        ARG_freq,
        ARG_ic_type,
        ARG_byte_order,
        ARG_leds_per_pixel,
        ARG_white_color_temp;
        ARG_blue_correct;
        ARG_high0,
        ARG_low0,
        ARG_high1,
        ARG_low1,
        ARG_res,
        ARG_spi_bus
    };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_data_pin,         MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = -1            } },
        { MP_QSTR_freq,             MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = 10000000      } },
        { MP_QSTR_ic_type,          MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = CUSTOM        } },
        { MP_QSTR_byte_order,       MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = GRB           } },
        { MP_QSTR_leds_per_pixel,   MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = 3             } },
        { MP_QSTR_white_color_temp, MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = 3000          } },
        { MP_QSTR_blue_correct,     MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool = false        } },
        { MP_QSTR_high0,            MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = -1            } },
        { MP_QSTR_low0,             MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = -1            } },
        { MP_QSTR_high1,            MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = -1            } },
        { MP_QSTR_low1,             MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = -1            } },
        { MP_QSTR_res,              MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int = -1            } },
        { MP_QSTR_spi_bus,          MP_ARG_OBJ  | MP_ARG_KW_ONLY, {.u_obj = mp_const_none } },

    };

    mp_arg_val_t args[MP_ARRAY_SIZE(make_new_args)];
    mp_arg_parse_all_kw_array(
        n_args,
        n_kw,
        all_args,
        MP_ARRAY_SIZE(make_new_args),
        make_new_args,
        args
    );

    // create new object
    mp_lcd_led_bus_obj_t *self = m_new_obj(mp_lcd_led_bus_obj_t);
    self->base.type = &mp_lcd_led_bus_type;

    self->callback = mp_const_none;

    self->leds_per_pixel = (uint8_t)args[ARG_leds_per_pixel].u_int;
    self->color_temp.blue_correct = (bool)args[ARG_blue_correct].u_bool;

    if (self->leds_per_pixel == 4) {
        uint16_t color_temp = (uint16_t)args[ARG_white_color_temp].u_int;
        if (color_temp == 0) color_temp = 3000;
        c_temp2rgb(color_temp, &self->color_temp.r, &self->color_temp.g, &self->color_temp.b);
    }
    
    self->pixel_order = (mp_lcd_led_pixel_order)args[ARG_byte_order].u_int;

    if (args[ARG_spi_bus].u_int == mp_const_none) {
        self->data_pin = (esp_gpio_t)args[ARG_data_pin].u_int;

        self->strip_encoder = m_new_obj(mp_lcd_led_strip_encoder_t);

        self->bit0.duration1 = (int)args[ARG_b0dur1].u_int;
        self->bit0.duration2 = (int)args[ARG_b0dur2].u_int;
        self->bit1.duration1 = (int)args[ARG_b1dur1].u_int;
        self->bit1.duration2 = (int)args[ARG_b1dur2].u_int;

        self->res = (int)args[ARG_res].u_int;
    } else {
        machine_hw_spi_bus_obj_t *spi_bus = MP_OBJ_TO_PTR(args[ARG_spi_bus].u_obj);

        self->panel_io_handle.panel_io = NULL;

        self->bus_handle = m_new_obj(esp_lcd_spi_bus_handle_t);

        self->panel_io_config = m_new_obj(esp_lcd_panel_io_spi_config_t);
        self->panel_io_config->cs_gpio_num = -1;
        self->panel_io_config->dc_gpio_num = -1;
        self->panel_io_config->spi_mode = 0;
        self->panel_io_config->pclk_hz = (unsigned int)args[ARG_freq].u_int;
        self->panel_io_config->on_color_trans_done = &bus_trans_done_cb;
        self->panel_io_config->user_ctx = self;
        self->panel_io_config->flags.lsb_first = 0;
        self->panel_io_config->trans_queue_depth = 10;
        self->panel_io_config->lcd_cmd_bits = 8;
        self->panel_io_config->lcd_param_bits = 8;

        self->spi_device = m_new_obj(machine_hw_spi_device_obj_t);
        self->spi_device->active = true;
        self->spi_device->base.type = &machine_hw_spi_device_type;
        self->spi_device->spi_bus = spi_bus;
        self->spi_device->deinit = &led_spi_deinit_callback;
        self->spi_device->user_data = self;
    }

    self->freq = (uint32_t)args[ARG_freq].u_int;

    switch(self->pixel_order) {
        case RGB:
            self->rgb_order = (uint8_t *) { 0, 1, 2 };
            break;
        case RBG:
            self->rgb_order = (uint8_t *) { 0, 2, 1 };
            break;
        case GRB:
            self->rgb_order = (uint8_t *) { 1, 0, 2 };
            break;
        case GBR
            self->rgb_order = (uint8_t *) { 1, 2, 0 };
            break;
        case BRG:
            self->rgb_order = (uint8_t *) { 2, 0, 1 };
            break;
        case BGR:
            self->rgb_order = (uint8_t *) { 2, 1, 0 };
            break;
        default:
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Invalid byte order"));
            return mp_const_none;
    }

    self->panel_io_handle.get_lane_count = &led_get_lane_count;
    self->panel_io_handle.del = &led_del;
    self->panel_io_handle.rx_param = &led_rx_param;
    self->panel_io_handle.tx_param = &led_tx_param;
    self->panel_io_handle.tx_color = &led_tx_color;
    self->panel_io_handle.allocate_framebuffer = &led_allocate_framebuffer;
    self->panel_io_handle.free_framebuffer = &led_free_framebuffer;
    self->panel_io_handle.init = &led_init;

    return MP_OBJ_FROM_PTR(self);
}

mp_lcd_err_t led_del(mp_obj_t obj)
{
    // mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *)obj;
    LCD_UNUSED(obj);
    return LCD_OK;
}

mp_lcd_err_t led_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
{
    LCD_UNUSED(obj);
    LCD_UNUSED(param);
    LCD_UNUSED(lcd_cmd);
    LCD_UNUSED(param_size);

    return LCD_OK;
}

mp_lcd_err_t led_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
{
    LCD_UNUSED(obj);
    LCD_UNUSED(param);
    LCD_UNUSED(lcd_cmd);
    LCD_UNUSED(param_size);

    return LCD_OK;
}

mp_obj_t led_free_framebuffer(mp_obj_t obj, mp_obj_t buf)
{
    mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *)obj;

    if (self->panel_handle != NULL) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Unable to free buffer"));
        return mp_const_none;
    }

    mp_obj_array_t *array_buf = (mp_obj_array_t *)MP_OBJ_TO_PTR(buf);

    if (array_buf == self->view1) {
        heap_caps_free(array_buf->items);
        self->view1 = NULL;
    } else if (array_buf == self->view2) {
        heap_caps_free(array_buf->items);
        self->view2 = NULL;
    } else {
        mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("No matching buffer found"));
    }
    return mp_const_none;
}

mp_obj_t led_allocate_framebuffer(mp_obj_t obj, uint32_t size, uint32_t caps)
{
    mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *)obj;

    void *buf = heap_caps_calloc(1, 1, MALLOC_CAP_INTERNAL);
    mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, 1, buf));
    view->typecode |= 0x80; // used to indicate writable buffer

    if (self->view1 == NULL) {
        self->buffer_size = size;
        self->view1 = view;
    } else if (self->buffer_size != size) {
        heap_caps_free(buf);
        mp_raise_msg_varg(
            &mp_type_MemoryError,
            MP_ERROR_TEXT("Frame buffer sizes do not match (%d)"),
            size
        );
        return mp_const_none;
    } else if (self->view2 == NULL) {
        self->view2 = view;
    } else {
        heap_caps_free(buf);
        mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("There is a maximum of 2 frame buffers allowed"));
        return mp_const_none;
    }

    return MP_OBJ_FROM_PTR(view);
}


mp_lcd_err_t led_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits)
{
    LCD_UNUSED(cmd_bits);
    LCD_UNUSED(param_bits);
    LCD_UNUSED(rgb565_byte_swap);

    mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *)obj;

    if (bpp != 24 || bpp != 32) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Must set bpp to 24 or 32"));
        return LCD_ERR_INVALID_ARG;
    }

    self->pixel_count = width * height;
    self->leds_per_pixel = bpp / 8;
    uint32_t buf_size = self->leds_per_pixel * self->pixel_count;

    if (self->buffer_size != buf_size) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("buffer size is not correct"));
        return LCD_ERR_INVALID_ARG;
    }

    if (self->spi_device == NULL) {
        rmt_tx_channel_config_t rmt_chan_config = {
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .gpio_num = self->data_pin,
            .mem_block_symbols = 64,
            .resolution_hz = self->freq,
            .trans_queue_depth = 4,
            .flags.with_dma = self->buf2 != NULL ? 1 : 0,
            .flags.invert_out = 0,
        };

        ESP_ERROR_CHECK(rmt_new_tx_channel(&rmt_chan_config, &self->rmt_chan));

        self->strip_encoder = calloc(1, sizeof(mp_lcd_led_strip_encoder_t));
        self->strip_encoder->base.encode = led_encode_strip;
        self->strip_encoder->base.del = led_del_strip;
        self->strip_encoder->base.reset = led_reset_strip;

        uint16_t bit0_level0 = self->bit0.duration1 >= 0 ? 1 : 0;
        uint16_t bit0_level1 = self->bit0.duration2 >= 0 ? 1 : 0;

        uint16_t bit0_duration0 = self->bit0.duration1 >= 0 ? (uint16_t)self->bit0.duration1 : (uint16_t)-self->bit0.duration1;
        uint16_t bit0_duration1 = self->bit0.duration2 >= 0 ? (uint16_t)self->bit0.duration2 : (uint16_t)-self->bit0.duration2;

        uint16_t bit1_level0 = self->bit1.duration1 >= 0 ? 1 : 0;
        uint16_t bit1_level1 = self->bit1.duration1 >= 0 ? 1 : 0;

        uint16_t bit1_duration0 = self->bit1.duration1 >= 0 ? (uint16_t)self->bit1.duration1 : (uint16_t)-self->bit1.duration1;
        uint16_t bit1_duration1 = self->bit1.duration2 >= 0 ? (uint16_t)self->bit1.duration2 : (uint16_t)-self->bit1.duration2;

        rmt_bytes_encoder_config_t bytes_encoder_config = (rmt_bytes_encoder_config_t) {
            .bit0 = {
                .level0 = bit0_level0,
                .duration0 = bit0_duration0 * self->freq / 1000000000,
                .level1 = bit0_level1,
                .duration1 = bit0_duration1 * self->freq / 1000000000,
            },
            .bit1 = {
                .level0 = bit1_level0,
                .duration0 = bit1_duration0 * self->freq / 1000000000,
                .level1 = bit1_level1,
                .duration1 = bit1_duration1 * self->freq / 1000000000,
            },
            .flags.msb_first = (uint32_t)self->msb_first;
        };

        ESP_ERROR_CHECK(rmt_new_bytes_encoder(&bytes_encoder_config, $self->strip_encoder->bytes_encoder));
        rmt_copy_encoder_config_t copy_encoder_config = {};
        ESP_ERROR_CHECK(rmt_new_copy_encoder(&copy_encoder_config, &self->strip_encoder->copy_encoder));

        uint16_t reset_level = self->res >= 0 ? 1 : 0;
        uint16_t reset_ticks = reset_level ? (uint16_t)(self->freq / 1000000 * (uint32_t) self->res / 2) : (uint16_t)(self->freq / 1000000 * (uint32_t)-self->res / 2);

        self->strip_encoder->reset_code = (rmt_symbol_word_t) {
            .level0 = reset_level,
            .duration0 = reset_ticks,
            .level1 = reset_level,
            .duration1 = reset_ticks,
        };

        rmt_tx_event_callbacks_t callback = {
            .on_trans_done = &led_rmt_bus_trans_done_cb
        }

        rmt_tx_register_event_callbacks(self->rmt_chan, &callback, self);
        ESP_ERROR_CHECK(rmt_enable(self->rmt_chan));
    } else {
        if (self->spi_device->spi_bus->state == MP_SPI_STATE_STOPPED) {
            machine_hw_spi_bus_initilize(self->spi_device->spi_bus);
        }

        mp_lcd_err_t ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)self->spi_device->spi_bus->host, self->panel_io_config, &self->panel_io_handle.panel_io);
        if (ret != ESP_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_spi)"), ret);
        }

        machine_hw_spi_bus_add_device(*self->spi_device);

        buf_size = self->pixel_count * 4 + 8;
    }

    void *buf1 = self->view1->items;
    self->view1->items = heaps_caps_calloc(1, buf_size, self->buffer_flags);
    self->buf1 = self->view1->items;
    self->view1->len = self->buffer_size;
    heap_caps_free(buf1);

    if (self->buf2 != NULL) {
        void *buf2 = self->view2->items;
        self->view2->items = heaps_caps_calloc(1, buf_size, self->buffer_flags);
        self->buf2 = self->view2->items;
        self->view2->len = self->buffer_size;
        heap_caps_free(buf2);
    }

    return LCD_OK;
}

mp_lcd_err_t led_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
{
    mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *)obj;
    *lane_count = 1;

    return LCD_OK;
}


mp_lcd_err_t led_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update)
{
    LCD_UNUSED(lcd_cmd);
    LCD_UNUSED(x_start);
    LCD_UNUSED(y_start);
    LCD_UNUSED(x_end);
    LCD_UNUSED(y_end);
    LCD_UNUSED(rotation);
    LCD_UNUSED(last_update);

    mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *)obj;
    mp_lcd_err_t err;
    uint8_t tmp_color[4];

    if (self->spi_device == NULL) {
        if (self->leds_per_pixel == 4) {
            for (uint32_t i = color_size - 3;i >= 0;i -= 3) {
                for (uint8_t j = 0; j < 3; j++) tmp_color[j] = color[i + j];
                rgb2rgbw(&self->color_temp, tmp_color);
                for (uint8_t j = 0; j < 3; j++) color[i + rgb_order[j] + self->pixel_count] = tmp_color[j];
                color[i + 4 + self->pixel_count] = tmp_color[3];
            }
        } else {
            if (pixel_order != RGB) {
                for (uint32_t i = 0;i < color_size;i += 3) {
                    for (uint8_t j = 0; j < 3; j++) tmp_color[rgb_order[j]] = color[i + j];
                    for (uint8_t j = 0; j < 3; j++) color[i + j] = tmp_color[j];
                }
            }
        }
        rmt_transmit_config_t tx_conf = {
            .loop_count = 0,
        };
        err = rmt_transmit(self->rmt_chan, self->strip_encoder, color, color_size, &tx_conf);
    } else {
        for (uint32_t i = color_size - 3;i >= 0;i -= 3) {
            for (uint8_t j = 0; j < 3; j++) tmp_color[j] = color[i + j];
            rgb2rgbw(&self->color_temp, tmp_color);
            for (uint8_t j = 0; j < 3; j++) color[i + rgb_order[j] + self->pixel_count + 5] = tmp_color[j];
            color[i + self->pixel_count + 4] = tmp_color[3] | 0xE0;
        }
        for (uint8_t i = 0;i < 4;i++) {
            color[i] = 0x00;
            color[i + self->pixel_count * 4] = 0xFF
        }

        color_size = (size_t)self->pixel_count * 4 + 8;

        err = esp_lcd_panel_io_tx_color(self->panel_io_handle.panel_io, -1, color, color_size);
    }

    if (err == LCD_OK && self->callback == mp_const_none) {
        while (!self->trans_done) {}
        self->trans_done = false;
    }

    return err;
}


static const mp_rom_map_elem_t mp_lcd_led_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_lane_count),       MP_ROM_PTR(&mp_lcd_bus_get_lane_count_obj)       },
    { MP_ROM_QSTR(MP_QSTR_allocate_framebuffer), MP_ROM_PTR(&mp_lcd_bus_allocate_framebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_free_framebuffer),     MP_ROM_PTR(&mp_lcd_bus_free_framebuffer_obj)     },
    { MP_ROM_QSTR(MP_QSTR_register_callback),    MP_ROM_PTR(&mp_lcd_bus_register_callback_obj)    },
    { MP_ROM_QSTR(MP_QSTR_tx_param),             MP_ROM_PTR(&mp_lcd_bus_tx_param_obj)             },
    { MP_ROM_QSTR(MP_QSTR_tx_color),             MP_ROM_PTR(&mp_lcd_bus_tx_color_obj)             },
    { MP_ROM_QSTR(MP_QSTR_rx_param),             MP_ROM_PTR(&mp_lcd_bus_rx_param_obj)             },
    { MP_ROM_QSTR(MP_QSTR_init),                 MP_ROM_PTR(&mp_lcd_bus_init_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_deinit),               MP_ROM_PTR(&mp_lcd_bus_deinit_obj)               },
    { MP_ROM_QSTR(MP_QSTR___del__),              MP_ROM_PTR(&mp_lcd_bus_deinit_obj)               },
    { MP_ROM_QSTR(MP_QSTR_RGB),                  MP_ROM_INT(RGB)                                  },
    { MP_ROM_QSTR(MP_QSTR_RBG),                  MP_ROM_INT(RBG)                                  },
    { MP_ROM_QSTR(MP_QSTR_GRB),                  MP_ROM_INT(GRB)                                  },
    { MP_ROM_QSTR(MP_QSTR_GBR),                  MP_ROM_INT(GBR)                                  },
    { MP_ROM_QSTR(MP_QSTR_BRG),                  MP_ROM_INT(BRG)                                  },
    { MP_ROM_QSTR(MP_QSTR_BGR),                  MP_ROM_INT(BGR)                                  },
};

MP_DEFINE_CONST_DICT(mp_lcd_led_locals_dict, mp_lcd_led_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_led_bus_type,
    MP_QSTR_LEDBus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_led_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_led_locals_dict
);
