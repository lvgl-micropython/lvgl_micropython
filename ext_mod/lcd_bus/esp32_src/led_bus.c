// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include <math.h>

#include "lcd_types.h"
#include "lcd_bus_task.h"
#include "modlcd_bus.h"
#include "led_bus.h"
#include "allocate_buffers.h"
#include "bus_trans_done.h"
#include "../../../micropy_updates/common/mp_spi_common.h"


#include "esp_heap_caps.h"
#include "driver/rmt_common.h"
#include "driver/rmt_encoder.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_types.h"
#include "esp_task.h"
#include "esp_lcd_panel_io.h"



// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/objarray.h"


#define LED_MIN(x, y) (x) < (y) ? (x) : (y)
#define LED_MAX(x, y) (x) >= (y) ? (x) : (y)

mp_lcd_err_t rmt_new_led_strip_encoder(mp_lcd_led_bus_obj_t *self, uint32_t resolution, rmt_encoder_handle_t *ret_encoder);
void led_deinit_callback(mp_machine_hw_spi_device_obj_t *device);
mp_lcd_err_t led_del(mp_lcd_bus_obj_t *self_in);
mp_lcd_err_t led_init(mp_lcd_bus_obj_t *self_in, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,
                      uint8_t cmd_bits, uint8_t param_bits);


static void led_send_func(mp_lcd_bus_obj_t *self_in, int cmd, uint8_t *params, size_t params_len)
{
    LCD_UNUSED(cmd);
    LCD_UNUSED(params_len);
}

 
static bool led_bus_spi_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    LCD_UNUSED(edata);
    mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *)user_ctx;

    if (!lcd_bus_event_isset_from_isr(&self->tx_data.swap_bufs)) {
        uint8_t *idle_fb = self->bufs.idle;
        self->bufs.idle = self->bufs.active;
        self->bufs.active = idle_fb;
        lcd_bus_event_set_from_isr(self->tx_data.swap_bufs);
    }

    return false;
}


static bool led_bus_rmt_trans_done_cb(rmt_channel_handle_t tx_chan, const rmt_tx_done_event_data_t *edata, void *user_ctx)
{
    LCD_UNUSED(tx_chan);
    LCD_UNUSED(edata);

    mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *)user_ctx;

    if (!lcd_bus_event_isset_from_isr(&self->tx_data.swap_bufs)) {
        lcd_bus_event_set_from_isr(self->tx_data.swap_bufs);
    }

    return false;
}


// The  RGB to RGBW conversion function.
static void rgb2rgbw(mp_lcd_led_color_temp *color_temp, uint8_t *rgbw)
{
    // Calculate all of the color's white values corrected taking into account the white color temperature.
    float wRed = (float)(rgbw[0]) * (255.0f / (float)color_temp->r);
    float wGreen = (float)(rgbw[1]) * (255.0f / (float)color_temp->g);
    float wBlue = (float)(rgbw[2]) * (255.0f / (float)color_temp->b);

    // Determine the smallest white value from above.
    uint8_t wMin = roundf(LED_MIN(wRed, LED_MIN(wGreen, wBlue)));

    // Make the color with the smallest white value to be the output white value
    if (wMin == wRed) rgbw[3] = rgbw[0];
    else if (wMin == wGreen) rgbw[3] = rgbw[1];
    else rgbw[3] = rgbw[2];

    rgbw[0] = LED_MAX(0, (uint8_t)roundf((float)rgbw[0] - ((float)rgbw[3] * ((float)color_temp->r / 255.0f))));
    rgbw[1] = LED_MAX(0, (uint8_t)roundf((float)rgbw[1] - ((float)rgbw[3] * ((float)color_temp->g / 255.0f))));
    rgbw[2] = LED_MAX(0, (uint8_t)roundf((float)rgbw[2] - ((float)rgbw[3] * ((float)color_temp->b / 255.0f))));

    if (color_temp->blue_correct) rgbw[3] = LED_MAX(0, rgbw[3] - (rgbw[2] * 0.2));
}


static void led_flush_func(mp_lcd_bus_obj_t *self_in, rotation_data_t *r_data, rotation_data_t *original_r_data,
                           uint8_t *idle_fb, uint8_t last_update)
{
    mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *) self_in;

    if (self->callback != mp_const_none) cb_isr(self->callback);

    if (last_update) {
        mp_lcd_err_t ret;

        uint16_t src_bytes_per_line = r_data->dst_width * 3;
        uint16_t dst_bytes_per_line = r_data->dst_width * self->leds_per_pixel;
        uint32_t src_i;
        uint32_t dst_i;
        uint8_t *temp_fb;
        uint8_t offset;
        if (self->spi_device == NULL) offset = 0;
        else offset = 4;

        lcd_bus_event_wait(self->tx_data.swap_bufs);

        uint8_t *active_fb = self->bufs.active;

        if (self->leds_per_pixel == 4) {
            for (uint16_t y=r_data->dst_width - 1;y != 0; y--) {
                for (uint16_t x=r_data->dst_width - 1; x != 0; x--) {
                    src_i = (uint32_t)(y * src_bytes_per_line) + (uint32_t)(x * 3);
                    dst_i = (uint32_t)(y * dst_bytes_per_line) + (uint32_t)(x * 4) + offset;
                    temp_fb = idle_fb + src_i;

                    rgb2rgbw(&self->color_temp, temp_fb);

                    active_fb[dst_i + self->pixel_order.index0] = temp_fb[0];
                    active_fb[dst_i + self->pixel_order.index1] = temp_fb[1];
                    active_fb[dst_i + self->pixel_order.index2] = temp_fb[2];
                    active_fb[dst_i + 4] = temp_fb[3] | 0xE0;
                }
            }
        } else {
            for (uint16_t y=r_data->dst_height - 1;y != 0; y--) {
                for (uint16_t x=r_data->dst_width - 1; x != 0; x--) {
                    src_i = (uint32_t)(y * src_bytes_per_line) + (uint32_t)(x * 3);
                    dst_i = (uint32_t)(y * dst_bytes_per_line) + (uint32_t)(x * 3) + offset;
                    temp_fb = idle_fb + src_i;
                    active_fb[dst_i + self->pixel_order.index0] = temp_fb[0];
                    active_fb[dst_i + self->pixel_order.index1] = temp_fb[1];
                    active_fb[dst_i + self->pixel_order.index2] = temp_fb[2];
                }
            }
        }

        size_t byte_count = (size_t)dst_bytes_per_line * (size_t)r_data->dst_height;

        if (self->spi_device == NULL) {
            rmt_transmit_config_t tx_conf = {
                .loop_count = 0,
            };
            ret = rmt_transmit(self->rmt_chan, self->led_encoder, active_fb, byte_count, &tx_conf);
        } else {
            byte_count += 4;

            for (uint8_t i = 0;i < 4;i++) {
                active_fb[i] = 0x00;
                active_fb[i + byte_count] = 0xFF;
            }

            ret = esp_lcd_panel_io_tx_color(self->panel_io_handle, -1, active_fb, byte_count + 4);
        }

        if (ret != 0) {
            mp_printf(&mp_plat_print, "led_flush_func error (%d)\n", ret);
        } else {
            lcd_bus_event_clear(self->tx_data.swap_bufs);
        }
    }
}


static bool led_init_func(mp_lcd_bus_obj_t *self_in)
{
    mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *) self_in;


    if (self->spi_device == NULL) {

        rmt_tx_channel_config_t rmt_chan_config = {
            .clk_src = RMT_CLK_SRC_DEFAULT,
            .gpio_num = self->data_pin,
            .mem_block_symbols = 64,
            .resolution_hz = self->freq,
            .trans_queue_depth = 4,
            .flags.with_dma = 1,
            .flags.invert_out = 0,
        };

        self->init.err = rmt_new_tx_channel(&rmt_chan_config, &self->rmt_chan);
        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(rmt_new_tx_channel)");
            return false;
        }

        self->init.err = rmt_new_led_strip_encoder(self, self->freq, &self->led_encoder);
        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(rmt_new_led_strip_encoder)");
            return false;
        }

        rmt_tx_event_callbacks_t callback = {
            .on_trans_done = &led_bus_rmt_trans_done_cb
        };

        self->init.err = rmt_tx_register_event_callbacks(self->rmt_chan, &callback, self);
        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(rmt_tx_register_event_callbacks)");
            return false;
        }

        self->init.err = rmt_enable(self->rmt_chan);
        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(rmt_enable)");
            return false;
        }

    } else {
        self->init.err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)self->spi_device->spi_bus->host,
                                                   self->panel_io_config, &self->panel_io_handle);

        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_spi)");
            return false;
        }
    }

    return true;
}


static void c_temp2rgb(uint16_t kelvin, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint16_t temp = kelvin / 100;

    if (temp <= 66 ) {
        *r = 255;
        *g = (uint8_t)(99.4708025861f * (float)log(temp) - 161.1195681661f);
        if (temp <= 19) {
            *b = 0;
        } else {
            *b = (uint8_t)(138.5177312231f * (float)log(temp - 10) - 305.0447927307f);
        }
    } else {
        *r = (uint8_t)(329.698727446f * powf((float)(temp - 60), -0.1332047592f));
        *g = (uint8_t)(288.1221695283f * powf((float)(temp - 60), -0.0755148492f));
        *b = 255;
    }
}


static mp_obj_t mp_lcd_led_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum {
        ARG_data_pin,
        ARG_freq,
        ARG_byte_order,
        ARG_msb_first,
        ARG_leds_per_pixel,
        ARG_white_color_temp,
        ARG_blue_correct,
        ARG_high0,
        ARG_low0,
        ARG_high1,
        ARG_low1,
        ARG_res,
        ARG_spi_bus
    };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_data_pin,         MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int=-1            } },
        { MP_QSTR_freq,             MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int=10000000      } },
        { MP_QSTR_byte_order,       MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int=GRB           } },
        { MP_QSTR_msb_first,        MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool=true         } },
        { MP_QSTR_leds_per_pixel,   MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int=3             } },
        { MP_QSTR_white_color_temp, MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int=3000          } },
        { MP_QSTR_blue_correct,     MP_ARG_BOOL | MP_ARG_KW_ONLY, {.u_bool=false        } },
        { MP_QSTR_high0,            MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int=-1            } },
        { MP_QSTR_low0,             MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int=-1            } },
        { MP_QSTR_high1,            MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int=-1            } },
        { MP_QSTR_low1,             MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int=-1            } },
        { MP_QSTR_res,              MP_ARG_INT  | MP_ARG_KW_ONLY, {.u_int=-1            } },
        { MP_QSTR_spi_bus,          MP_ARG_OBJ  | MP_ARG_KW_ONLY, {.u_obj=mp_const_none } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(make_new_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(make_new_args), make_new_args, args);

    // create new object
    mp_lcd_led_bus_obj_t *self = m_new_obj(mp_lcd_led_bus_obj_t);
    self->base.type = &mp_lcd_led_bus_type;

    self->callback = mp_const_none;

    self->leds_per_pixel = (uint8_t)args[ARG_leds_per_pixel].u_int;
    self->color_temp.blue_correct = (uint8_t)args[ARG_blue_correct].u_bool;
    self->msb_first = (uint8_t)args[ARG_msb_first].u_bool;

    if (self->leds_per_pixel == 4) {
        uint16_t color_temp = (uint16_t)args[ARG_white_color_temp].u_int;
        if (color_temp == 0) color_temp = 3000;
        c_temp2rgb(color_temp, &self->color_temp.r, &self->color_temp.g, &self->color_temp.b);
    }
    
    self->pixel_order = (rgb_order_t)((uint8_t)args[ARG_byte_order].u_int);

    if (args[ARG_spi_bus].u_obj == mp_const_none) {
        self->spi_device = NULL;
        self->data_pin = (int)args[ARG_data_pin].u_int;

        self->bit0.duration1 = (int)args[ARG_high0].u_int;
        self->bit0.duration2 = (int)args[ARG_low0].u_int;
        self->bit1.duration1 = (int)args[ARG_high1].u_int;
        self->bit1.duration2 = (int)args[ARG_low1].u_int;
        self->res = (int)args[ARG_res].u_int;

        self->led_encoder = NULL;

    } else {
        mp_machine_hw_spi_bus_obj_t *spi_bus = MP_OBJ_TO_PTR(args[ARG_spi_bus].u_obj);

        self->panel_io_handle = NULL;

        self->bus_handle = m_new_obj(esp_lcd_spi_bus_handle_t);

        self->panel_io_config = m_new_obj(esp_lcd_panel_io_spi_config_t);
        self->panel_io_config->cs_gpio_num = -1;
        self->panel_io_config->dc_gpio_num = -1;
        self->panel_io_config->spi_mode = 0;
        self->panel_io_config->pclk_hz = (unsigned int)args[ARG_freq].u_int;
        self->panel_io_config->on_color_trans_done = &led_bus_spi_trans_done_cb;
        self->panel_io_config->user_ctx = self;
        self->panel_io_config->flags.lsb_first = 0;
        self->panel_io_config->trans_queue_depth = 10;
        self->panel_io_config->lcd_cmd_bits = 8;
        self->panel_io_config->lcd_param_bits = 8;

        self->spi_device = m_new_obj(mp_machine_hw_spi_device_obj_t);
        self->spi_device->active = true;
        self->spi_device->base.type = &mp_machine_hw_spi_device_type;
        self->spi_device->spi_bus = spi_bus;
        self->spi_device->deinit = &led_deinit_callback;
        self->spi_device->user_data = self;
    }

    self->freq = (uint32_t)args[ARG_freq].u_int;

    self->internal_cb_funcs.deinit = &led_del;
    self->internal_cb_funcs.init = &led_init;

    self->tx_data.flush_func = led_flush_func;
    self->init.init_func = led_init_func;

    self->tx_cmds.send_func = &led_send_func;

    self->num_lanes = 1;

    return MP_OBJ_FROM_PTR(self);
}


mp_lcd_err_t led_del(mp_lcd_bus_obj_t *self_in)
{
    // mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *)obj;
    LCD_UNUSED(self_in);
    return LCD_OK;
}

void led_deinit_callback(mp_machine_hw_spi_device_obj_t *device)
{
    mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *)device->user_data;
    led_del(MP_OBJ_FROM_PTR(self));
}




mp_lcd_err_t led_init(mp_lcd_bus_obj_t *self_in, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,
                      uint8_t cmd_bits, uint8_t param_bits)
{
    LCD_UNUSED(cmd_bits);
    LCD_UNUSED(param_bits);
    
    mp_lcd_led_bus_obj_t *self = (mp_lcd_led_bus_obj_t *)self_in;
    
    self->r_data.sw_rotation = 0;

    if (bpp != 24) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Must set bpp to 24 or 32"));
        return LCD_ERR_INVALID_ARG;
    }

    self->pixel_count = width * height;

    uint32_t buf_size = self->leds_per_pixel * self->pixel_count;


    if (self->spi_device != NULL) buf_size += 8;
            
    self->r_data.dst_width = (uint16_t)width;
    self->r_data.dst_height = (uint16_t)height;
    
    if (
        !allocate_framebuffer(self->bufs.idle, buf_size, MALLOC_CAP_DMA) || 
        !allocate_framebuffer(self->bufs.active, buf_size, MALLOC_CAP_DMA)
    ) {
        mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("not enough dma memory to allocate buffer"));
    }

    if (self->spi_device == NULL) {
        self->led_encoder = NULL;
    } else {
        if (self->spi_device->spi_bus->state == MP_SPI_STATE_STOPPED) {
            mp_machine_hw_spi_bus_initilize(self->spi_device->spi_bus);
        }
    }

    xTaskCreatePinnedToCore(
            lcd_bus_task, "led_task", LCD_DEFAULT_STACK_SIZE / sizeof(StackType_t),
            self, ESP_TASK_PRIO_MAX - 1, &self->task.handle, 0);

    lcd_bus_lock_acquire(self->init.lock);
    lcd_bus_lock_release(self->init.lock);
    lcd_bus_lock_delete(self->init.lock);

    if (self->init.err == 0 && self->spi_device != NULL) {
        mp_machine_hw_spi_bus_add_device(self->spi_device);
    }

    return self->init.err;
}


static mp_obj_t led_bus_tx_color(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_cmd, ARG_data, ARG_x_start, ARG_y_start, ARG_x_end,
           ARG_y_end, ARG_dither, ARG_rotation, ARG_last_update };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,        MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_cmd,         MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_data,        MP_ARG_OBJ  | MP_ARG_REQUIRED },
        { MP_QSTR_x_start,     MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_y_start,     MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_x_end,       MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_y_end,       MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_dither,      MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_rotation,    MP_ARG_INT  | MP_ARG_REQUIRED },
        { MP_QSTR_last_update, MP_ARG_BOOL | MP_ARG_REQUIRED },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)args[ARG_self].u_obj;

    int cmd = (int)args[ARG_cmd].u_int;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_data].u_obj, &bufinfo, MP_BUFFER_RW);
    uint8_t * color = (uint8_t *)bufinfo.buf;
    size_t color_size = (size_t)bufinfo.len;

    int32_t x_start = (int32_t)args[ARG_x_start].u_int;
    int32_t y_start = (int32_t)args[ARG_y_start].u_int;
    int32_t x_end = (int32_t)args[ARG_x_end].u_int;
    int32_t y_end = (int32_t)args[ARG_y_end].u_int;
    uint8_t dither = (uint8_t)args[ARG_dither].u_int;
    uint8_t last_update = (uint8_t)args[ARG_last_update].u_bool;

    self->bufs.partial = (uint8_t *)color;

    lcd_bus_lock_acquire(self->tx_data.lock);

    self->r_data.last_update = (uint8_t)last_update;
    self->r_data.color_size = color_size;
    self->r_data.x_start = x_start;
    self->r_data.y_start = y_start;
    self->r_data.x_end = x_end;
    self->r_data.y_end = y_end;
    self->r_data.rotation = 0;
    self->r_data.dither = dither;
    self->r_data.cmd = cmd;

    lcd_bus_lock_release(self->task.lock);

    if (self->callback == mp_const_none) {
        while (self->trans_done == false) {}
        self->trans_done = false;
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(led_bus_tx_color_obj, 10, led_bus_tx_color);


static size_t rmt_encode_led_strip(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data,
                                   size_t data_size, rmt_encode_state_t *ret_state)
{
    mp_rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, mp_rmt_led_strip_encoder_t, base);
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
    mp_rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, mp_rmt_led_strip_encoder_t, base);
    rmt_del_encoder(led_encoder->bytes_encoder);
    rmt_del_encoder(led_encoder->copy_encoder);
    free(led_encoder);
    return ESP_OK;
}


static esp_err_t rmt_led_strip_encoder_reset(rmt_encoder_t *encoder)
{
    mp_rmt_led_strip_encoder_t *led_encoder = __containerof(encoder, mp_rmt_led_strip_encoder_t, base);
    rmt_encoder_reset(led_encoder->bytes_encoder);
    rmt_encoder_reset(led_encoder->copy_encoder);
    led_encoder->state = RMT_ENCODING_RESET;
    return ESP_OK;
}


mp_lcd_err_t rmt_new_led_strip_encoder(mp_lcd_led_bus_obj_t *self, uint32_t resolution, rmt_encoder_handle_t *ret_encoder)
{
    mp_rmt_led_strip_encoder_t *led_encoder = NULL;

    led_encoder = rmt_alloc_encoder_mem(sizeof(mp_rmt_led_strip_encoder_t));
    if (led_encoder == NULL) return LCD_ERR_NO_MEM;

    led_encoder->base.encode = rmt_encode_led_strip;
    led_encoder->base.del = rmt_del_led_strip_encoder;
    led_encoder->base.reset = rmt_led_strip_encoder_reset;
    // different led strip might have its own timing requirements, following parameter is for WS2812

    uint16_t bit0_level0 = self->bit0.duration1 < 0 ? 0 : 1;
    uint16_t bit0_level1 = self->bit0.duration2 < 0 ? 0 : 1;

    uint16_t bit0_duration0 = self->bit0.duration1 < 0 ? (uint16_t)-self->bit0.duration1 : (uint16_t)self->bit0.duration1;
    uint16_t bit0_duration1 = self->bit0.duration2 < 0 ? (uint16_t)-self->bit0.duration2 : (uint16_t)self->bit0.duration2;

    uint16_t bit1_level0 = self->bit1.duration1 < 0 ? 0 : 1;
    uint16_t bit1_level1 = self->bit1.duration1 < 0 ? 0 : 1;

    uint16_t bit1_duration0 = self->bit1.duration1 < 0 ? (uint16_t)-self->bit1.duration1 : (uint16_t)self->bit1.duration1;
    uint16_t bit1_duration1 = self->bit1.duration2 < 0 ? (uint16_t)-self->bit1.duration2 : (uint16_t)self->bit1.duration2;

    rmt_bytes_encoder_config_t bytes_encoder_config = (rmt_bytes_encoder_config_t) {
        .bit0 = {
            .level0 = bit0_level0,
            .duration0 = bit0_duration0 * resolution / 1000000000,
            .level1 = bit0_level1,
            .duration1 = bit0_duration1 * resolution / 1000000000,
        },
        .bit1 = {
            .level0 = bit1_level0,
            .duration0 = bit1_duration0 * resolution / 1000000000,
            .level1 = bit1_level1,
            .duration1 = bit1_duration1 * resolution / 1000000000,
        },
        .flags = {
            .msb_first = (uint32_t)self->msb_first
        }
    };

    mp_lcd_err_t ret = rmt_new_bytes_encoder(&bytes_encoder_config, &led_encoder->bytes_encoder);
    if (ret != LCD_OK) return ret;

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ret = rmt_new_copy_encoder(&copy_encoder_config, &led_encoder->copy_encoder);
    if (ret != LCD_OK) return ret;

    uint16_t reset_level = self->res < 0 ? 0 : 1;
    uint16_t reset_ticks = reset_level == (
             1 ? (uint16_t)(self->freq / 1000000 * (uint32_t)self->res / 2) : (uint16_t)(self->freq / 1000000 * (uint32_t)-self->res / 2));

    led_encoder->reset_code = (rmt_symbol_word_t) {
        .level0 = reset_level,
        .duration0 = reset_ticks,
        .level1 = reset_level,
        .duration1 = reset_ticks,
    };

    *ret_encoder = &led_encoder->base;
    return LCD_OK;
}



static const mp_rom_map_elem_t mp_lcd_led_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_lane_count),       MP_ROM_PTR(&mp_lcd_bus_get_lane_count_obj)       },
    { MP_ROM_QSTR(MP_QSTR_allocate_framebuffer), MP_ROM_PTR(&mp_lcd_bus_allocate_framebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_free_framebuffer),     MP_ROM_PTR(&mp_lcd_bus_free_framebuffer_obj)     },
    { MP_ROM_QSTR(MP_QSTR_register_callback),    MP_ROM_PTR(&mp_lcd_bus_register_callback_obj)    },
    { MP_ROM_QSTR(MP_QSTR_tx_param),             MP_ROM_PTR(&mp_lcd_bus_tx_param_obj)             },
    { MP_ROM_QSTR(MP_QSTR_tx_color),             MP_ROM_PTR(&led_bus_tx_color_obj)                },
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
