/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Damien P. George
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

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "extmod/modmachine.h"

#include "esp_attr.h"
#include "esp_system.h"
#include "soc/soc_caps.h"
#include "soc/i2c_periph.h"
#include "hal/i2c_hal.h"
#include "hal/i2c_ll.h"
#include "hal/i2c_types.h"
#include "driver/i2c.h"

#ifndef MICROPY_HW_I2C0_SCL
#define MICROPY_HW_I2C0_SCL (GPIO_NUM_18)
#define MICROPY_HW_I2C0_SDA (GPIO_NUM_19)
#endif

#ifndef MICROPY_HW_I2C1_SCL
    #if CONFIG_IDF_TARGET_ESP32
        #define MICROPY_HW_I2C1_SCL (GPIO_NUM_25)
        #define MICROPY_HW_I2C1_SDA (GPIO_NUM_26)
    #else
        #define MICROPY_HW_I2C1_SCL (GPIO_NUM_9)
        #define MICROPY_HW_I2C1_SDA (GPIO_NUM_8)
    #endif
#endif

#define I2C_UNUSED(x) ((void)x)

if CONFIG_IDF_TARGET_ESP32S3
    #define I2C_MASTER_FREQ_HZ    2000000
#else
    #define I2C_MASTER_FREQ_HZ    1000000
#endif

#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
    #define I2C_DEFAULT_TIMEOUT_US (104000) // 104ms
#else
    #define I2C_DEFAULT_TIMEOUT_US (50000) // 50ms
#endif

#define I2C_BUF_MIN_SIZE (sizeof(dummy_i2c_cmd_desc_t) + sizeof(dummy_i2c_cmd_link_t) * 8)

typedef struct _machine_hw_i2c_obj_t {
    mp_obj_base_t base;
    i2c_port_t port : 8;
    gpio_num_t scl : 8;
    gpio_num_t sda : 8;
    uint32_t freq;
} machine_hw_i2c_obj_t;


typedef struct {
    i2c_hw_cmd_t hw_cmd;
    union {
        uint8_t* data;
        uint8_t data_byte;
    };
    size_t bytes_used;
    size_t total_bytes;
} dummy_i2c_cmd_t;

typedef struct dummy_i2c_cmd_link {
    dummy_i2c_cmd_t cmd;
    struct dummy_i2c_cmd_link *next;
} dummy_i2c_cmd_link_t;


typedef struct {
    dummy_i2c_cmd_link_t *head;
    dummy_i2c_cmd_link_t *cur;
    dummy_i2c_cmd_link_t *free;

    void     *free_buffer;
    uint32_t  free_size;
} dummy_i2c_cmd_desc_t;


STATIC machine_hw_i2c_obj_t machine_hw_i2c_obj[I2C_NUM_MAX];

// borrowed form the esp-idf driver/i2c.c
static uint32_t s_get_src_clk_freq(i2c_clock_source_t clk_src)
{
    uint32_t periph_src_clk_hz = 0;
    switch (clk_src) {
    #if SOC_I2C_SUPPORT_APB
        case I2C_CLK_SRC_APB:
            periph_src_clk_hz = esp_clk_apb_freq();
            break;
    #endif
    #if SOC_I2C_SUPPORT_XTAL
        case I2C_CLK_SRC_XTAL:
            periph_src_clk_hz = esp_clk_xtal_freq();
            break;
    #endif
    default:
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("clock source %d is not supported"), clk_src);
        break;
    }
    return periph_src_clk_hz;
}


STATIC void machine_hw_i2c_init(machine_hw_i2c_obj_t *self, uint32_t freq, uint32_t timeout_us, bool first_init)
{
    if (!first_init) {
        i2c_driver_delete(self->port);
    }
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = self->sda,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = self->scl,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = freq,
        .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL,
    };

    self->freq = freq;
    i2c_param_config(self->port, &conf);


    #if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
        int tout = (int)(sizeof(uint32_t) * 8 - __builtin_clz(timeout_us << 14);
        i2c_set_timeout(port, tout);
    #else
        uint32_t src_clk = s_get_src_clk_freq(I2C_CLK_SRC_DEFAULT);
        int tout = (int)(src_clk / 1000000 * timeout_us);
        i2c_set_timeout(port, (tout > I2C_LL_MAX_TIMEOUT) ? I2C_LL_MAX_TIMEOUT : tout);
    #endif

    i2c_driver_install(self->port, I2C_MODE_MASTER, 0, 0, 0);
}


int i2c_write(i2c_port_t port, uint32_t freq, uint16_t addr, size_t len, uint8_t *buf, unsigned int flags)
{
    esp_err_t err = ESP_OK;

    if (flags & MP_MACHINE_I2C_FLAG_STOP && buf != NULL) {
        err = i2c_master_write_to_device(
            port, addr, buf, len,
            (((1 + len) * 8 * 1000000) / freq) / portTICK_PERIOD_MS
        );
    } else {
        uint8_t buffer[I2C_BUF_MIN_SIZE] = { 0 };

        i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
        i2c_master_start(handle);
        i2c_master_write_byte(handle, addr << 1 | I2C_MASTER_WRITE, true);
        if (buf != NULL) {
            i2c_master_write(handle, buf, len, true);
        } else {
            i2c_master_stop(handle);
        }

        err = i2c_master_cmd_begin(
            port, handle,
            (((1 + len) * 8 * 1000000) / freq) / portTICK_PERIOD_MS
        );

        i2c_cmd_link_delete_static(handle);
    }

    if (err == ESP_FAIL) {
        return -MP_ENODEV;
    } else if (err == ESP_ERR_TIMEOUT) {
        return -MP_ETIMEDOUT;
    } else if (err != ESP_OK) {
        return -abs(err);
    }

    return len;
}


int i2c_read(i2c_port_t port, uint32_t freq, uint16_t addr, size_t len, uint8_t *buf, unsigned int flags)
{
    esp_err_t err = ESP_OK;

    if (flags & MP_MACHINE_I2C_FLAG_STOP) {
        err = i2c_master_read_from_device(
            port, addr, buf, len,
            (((1 + len) * 8 * 1000000) / freq) / portTICK_PERIOD_MS
        );
    } else {
        uint8_t buffer[I2C_BUF_MIN_SIZE] = { 0 };

        i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));

        err = i2c_master_start(handle);
        if (err != ESP_OK) goto end;

        err = i2c_master_write_byte(handle, addr << 1 | I2C_MASTER_READ, true);
        if (err != ESP_OK) goto end;

        err = i2c_master_read(handle, buf, len, I2C_MASTER_LAST_NACK);
        if (err != ESP_OK) goto end;

        err = i2c_master_cmd_begin(
            port, handle,
            (((1 + len) * 8 * 1000000) / freq) / portTICK_PERIOD_MS
        );

        end:
            i2c_cmd_link_delete_static(handle);
    }

    if (err == ESP_FAIL) {
        return -MP_ENODEV;
    } else if (err == ESP_ERR_TIMEOUT) {
        return -MP_ETIMEDOUT;
    } else if (err != ESP_OK) {
        return -abs(err);
    }

    return len;
}


int machine_hw_i2c_transfer_single(mp_obj_base_t *self_in, uint16_t addr, size_t len, uint8_t *buf, unsigned int flags)
{
    machine_hw_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return i2c_write(self->port, self->freq, addr, len, buf, flags);
}


int machine_hw_i2c_transfer(mp_obj_base_t *self_in, uint16_t addr, size_t n, mp_machine_i2c_buf_t *bufs, unsigned int flags)
{
    /*
     original timeout calculation
     100 * (1 + data_len) / portTICK_PERIOD_MS

     This was wrong.
     portTICK_PERIOD_MS = typically 1
     lets assume the data being sent is 100 bytes long.
     100 + 1 = 101 * 100 = 10100 / 1 = 10,100
     so there is 1000 ticks per millisecond. so the timeout is 10 milliseconds.
     Doesn't matter how fast the transfer speed is. that is always the timeout

     so if the bus speed is set to 50,000hz which is a speed of 6,250 bytes a
     second or 6.25 bytes per millisecond guess what? It's going to take 16
     milliseconds to transfer the data and the timeout is going to occur before
     that happens. This is a very poor way of calculating the timeout needed.
     The bus speed needs to be considered when calculating the timeout period

     (((1 + data_len) * 8 * 1000000) / self->freq) / portTICK_PERIOD_MS
     so now we have 101 * 8 = 808
     808 * 1000000 = 808000000
     808000000 / 50000 = 16160
     16160 / 1 = 16160

     Now this is the correct timeout needed.

     221 bytes of data to be sent
     400000hz bus speed

     221 + 1 = 222
     222 * 8 * 1000000 = 1776000000
     1776000000 / 400000 = 4440 ticks (4.44 milliseconds)
     */

    if (!(flags & MP_MACHINE_I2C_FLAG_READ) && !(flags & MP_MACHINE_I2C_FLAG_WRITE1) && n > 1) {
        return mp_machine_i2c_transfer_adaptor(self_in, addr, n, bufs, flags);
    }

    machine_hw_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);

    I2C_UNUSED(n);

    if (!(flags & MP_MACHINE_I2C_FLAG_READ) && !(flags & MP_MACHINE_I2C_FLAG_WRITE1)) {
        return i2c_write(self->port, self->freq, addr, bufs->len, bufs->buf, flags);
    } else if (flags & MP_MACHINE_I2C_FLAG_WRITE1) {
        uint8_t *wb = bufs->buf;
        uint32_t wb_len = bufs->len;

        ++bufs;

        uint8_t *rb = bufs->buf;
        uint32_t rb_len = bufs->len;

        esp_err_t err = i2c_master_write_read_device(
            self->port, addr, wb, wb_len, rb, rb_len,
            (((2 + wb_len + rb_len) * 8 * 1000000) / self->freq) / portTICK_PERIOD_MS
        );

        if (err == ESP_FAIL) {
            return -MP_ENODEV;
        } else if (err == ESP_ERR_TIMEOUT) {
            return -MP_ETIMEDOUT;
        } else if (err != ESP_OK) {
            return -abs(err);
        }

        return rb_len;
    } else {
        return i2c_read(self->port, self->freq, addr, bufs->len, bufs->buf, flags);
    }
}

/******************************************************************************/
// MicroPython bindings for machine API

STATIC void machine_hw_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    machine_hw_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int h, l;
    i2c_get_period(self->port, &h, &l);
    mp_printf(print, "I2C(%u, scl=%u, sda=%u, freq=%u)",
        self->port, self->scl, self->sda, I2C_SCLK_FREQ / (h + l));
}

mp_obj_t machine_hw_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    MP_MACHINE_I2C_CHECK_FOR_LEGACY_SOFTI2C_CONSTRUCTION(n_args, n_kw, all_args);

    // Parse args
    enum { ARG_id, ARG_scl, ARG_sda, ARG_freq, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_scl, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_sda, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_freq, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 400000} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = I2C_DEFAULT_TIMEOUT_US} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get I2C bus
    mp_int_t i2c_id = mp_obj_get_int(args[ARG_id].u_obj);
    if (!(I2C_NUM_0 <= i2c_id && i2c_id < I2C_NUM_MAX)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%d) doesn't exist"), i2c_id);
    }

    // Get static peripheral object
    machine_hw_i2c_obj_t *self = (machine_hw_i2c_obj_t *)&machine_hw_i2c_obj[i2c_id];

    bool first_init = false;
    if (self->base.type == NULL) {
        // Created for the first time, set default pins
        self->base.type = &machine_i2c_type;
        self->port = i2c_id;
        if (self->port == I2C_NUM_0) {
            self->scl = MICROPY_HW_I2C0_SCL;
            self->sda = MICROPY_HW_I2C0_SDA;
        } else {
            self->scl = MICROPY_HW_I2C1_SCL;
            self->sda = MICROPY_HW_I2C1_SDA;
        }
        first_init = true;
    }

    // Set SCL/SDA pins if given
    if (args[ARG_scl].u_obj != MP_OBJ_NULL) {
        self->scl = machine_pin_get_id(args[ARG_scl].u_obj);
    }
    if (args[ARG_sda].u_obj != MP_OBJ_NULL) {
        self->sda = machine_pin_get_id(args[ARG_sda].u_obj);
    }

    if (args[ARG_freq].u_int > I2C_MASTER_FREQ_HZ) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Maximum frequency allowed is %dhz "), I2C_MASTER_FREQ_HZ);
    }

    // Initialise the I2C peripheral
    machine_hw_i2c_init(self, args[ARG_freq].u_int, args[ARG_timeout].u_int, first_init);

    return MP_OBJ_FROM_PTR(self);
}

STATIC const mp_machine_i2c_p_t machine_hw_i2c_p = {
    .transfer_supports_write1 = true,
    .transfer = machine_hw_i2c_transfer,
    .transfer_single = machine_hw_i2c_transfer_single,
};

MP_DEFINE_CONST_OBJ_TYPE(
    machine_i2c_type,
    MP_QSTR_I2C,
    MP_TYPE_FLAG_NONE,
    make_new, machine_hw_i2c_make_new,
    print, machine_hw_i2c_print,
    protocol, &machine_hw_i2c_p,
    locals_dict, &mp_machine_i2c_locals_dict
    );
