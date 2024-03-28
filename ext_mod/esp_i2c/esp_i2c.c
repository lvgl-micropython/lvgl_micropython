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
#include "esp_private/esp_clk.h"


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

#if CONFIG_IDF_TARGET_ESP32S3
    #define I2C_MASTER_FREQ_HZ    2000000
#else
    #define I2C_MASTER_FREQ_HZ    1000000
#endif

#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
    #define I2C_MAX_TIMEOUT_US (104000) // 104ms
#else
    #define I2C_MAX_TIMEOUT_US (50000) // 50ms
#endif

#define I2C_DEFAULT_TIMEOUT_US   (50000)

#define I2C_BUF_MIN_SIZE (sizeof(dummy_i2c_cmd_desc_t) + sizeof(dummy_i2c_cmd_link_t) * 8)


typedef struct _mp_esp_i2c_obj_t {
    mp_obj_base_t base;
    i2c_port_t port : 8;
    gpio_num_t scl : 8;
    gpio_num_t sda : 8;
    uint32_t freq;
} mp_esp_i2c_obj_t;


const mp_obj_type_t mp_esp_i2c_type;


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


STATIC mp_esp_i2c_obj_t machine_hw_i2c_obj[I2C_NUM_MAX];


STATIC size_t fill_memaddr_buf(uint8_t *memaddr_buf, uint32_t memaddr, uint8_t addrsize)
{
    switch (addrsize){
        case 8:
            memaddr_buf[0] = memaddr
            break;
        case 16:
            memaddr_buf[0] = memaddr & 0xFF
            memaddr_buf[1] = (memaddr >> 8) & 0xFF
            break;
        case 24:
            memaddr_buf[0] = memaddr & 0xFF
            memaddr_buf[1] = (memaddr >> 8) & 0xFF
            memaddr_buf[2] = (memaddr >> 16) & 0xFF
            break;
        case 32:
            memaddr_buf[0] = memaddr & 0xFF
            memaddr_buf[1] = (memaddr >> 8) & 0xFF
            memaddr_buf[2] = (memaddr >> 16) & 0xFF
            memaddr_buf[3] = (memaddr >> 24) & 0xFF
            break;
        default:
            mp_raise_ValueError(MP_ERROR_TEXT("invalid addrsize"));
            break;
    }

    return addrsize / 8;
}


// borrowed form the esp-idf driver/i2c.c
STATIC uint32_t s_get_src_clk_freq(i2c_clock_source_t clk_src)
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


STATIC void mp_esp_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    mp_esp_i2c_obj_t *self = (mp_esp_i2c_obj_t *)MP_OBJ_TO_PTR(self_in);
    int h, l;
    i2c_get_period(self->port, &h, &l);
    mp_printf(print, "I2C(%u, scl=%u, sda=%u, freq=%u)",
        self->port, self->scl, self->sda, s_get_src_clk_freq(I2C_CLK_SRC_DEFAULT) / (h + l));
}


STATIC mp_obj_t mp_esp_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_scl, ARG_sda, ARG_freq, ARG_timeout };
    
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,      MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_scl,     MP_ARG_KW_ONLY  | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_sda,     MP_ARG_KW_ONLY  | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_freq,    MP_ARG_KW_ONLY  | MP_ARG_INT, { .u_int = 400000      } },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY  | MP_ARG_INT, { .u_int = I2C_DEFAULT_TIMEOUT_US } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get I2C bus
    i2c_port_t i2c_id = (i2c_port_t)args[ARG_id].u_int);
    if (!(I2C_NUM_0 <= i2c_id && i2c_id < I2C_NUM_MAX)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%d) doesn't exist"), i2c_id);
    }

    // Get static peripheral object
    mp_esp_i2c_obj_t *self = (mp_esp_i2c_obj_t *)&machine_hw_i2c_obj[i2c_id];

    bool first_init = false;
    if (self->base.type == NULL) {
        // Created for the first time, set default pins
        self->base.type = &mp_esp_i2c_type;
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

    if (args[ARG_timeout].u_int > I2C_MAX_TIMEOUT_US) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("Maximum timeout allowed is %dus"), I2C_MAX_TIMEOUT_US);
    }
    
    self->freq = args[ARG_freq].u_int;
    uint32_t timeout_us = args[ARG_timeout].u_int
        
    if (!first_init) {
        i2c_driver_delete(self->port);
    }
    
    i2c_driver_install(self->port, I2C_MODE_MASTER, 0, 0, 0);

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = self->sda,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = self->scl,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = self->freq,
        .clk_flags = I2C_SCLK_SRC_FLAG_FOR_NOMAL,
    };

    i2c_param_config(self->port, &conf);
    
    i2c_set_timeout(self->port, I2C_LL_MAX_TIMEOUT);

    /*
    #if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32S2
        int tout = (int)(sizeof(uint32_t) * 8 - __builtin_clz(timeout_us << 14));
        i2c_set_timeout(self->port, tout);
    #else
        uint32_t src_clk = s_get_src_clk_freq(I2C_CLK_SRC_DEFAULT);
        int tout = (int)(src_clk / 1000000 * timeout_us);
        i2c_set_timeout(self->port, (tout > I2C_LL_MAX_TIMEOUT) ? I2C_LL_MAX_TIMEOUT : tout);
    #endif
    */

    return MP_OBJ_FROM_PTR(self);
}


STATIC mp_obj_t mp_esp_i2c_scan(mp_obj_t self_in)
{
    mp_esp_i2c_obj_t *self = (mp_esp_i2c_obj_t *)MP_OBJ_TO_PTR(self_in);
    
    mp_obj_t list = mp_obj_new_list(0, NULL);
    uint32_t timeout = ((2 * 8 * 1000000) / self->freq) / portTICK_PERIOD_MS;
    
    // 7-bit addresses 0b0000xxx and 0b1111xxx are reserved
    for (int addr = 0x08; addr < 0x78; ++addr) {
        uint8_t buffer[I2C_BUF_MIN_SIZE] = { 0 };
        
        i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
        i2c_master_start(handle);
        i2c_master_write_byte(handle, addr << 1 | I2C_MASTER_WRITE, true);
        i2c_master_stop(handle);
        
        err = i2c_master_cmd_begin(port, handle, timeout);
        i2c_cmd_link_delete_static(handle);
                 
        if (err == 0) {
            mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(addr));
        }
        // This scan loop may run for some time, so process any pending events/exceptions,
        // or allow the port to run any necessary background tasks.  But do it as fast as
        // possible, in particular we are not waiting on any events.
        mp_event_handle_nowait();
    }
    return list;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_esp_i2c_scan_obj, mp_esp_i2c_scan);


STATIC mp_obj_t mp_esp_i2c_readfrom(size_t n_args, const mp_obj_t *args)
{

    enum { ARG_self, ARG_addr, ARG_num_bytes, ARG_stop };
    
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,      MP_ARG_REQUIRED | MP_ARG_OBJ  },
        { MP_QSTR_addr,      MP_ARG_REQUIRED | MP_ARG_INT  },
        { MP_QSTR_num_bytes, MP_ARG_REQUIRED | MP_ARG_INT  },
        { MP_QSTR_stop,      MP_ARG_BOOL, { .u_bool = true } },
    };
    
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_esp_i2c_obj_t *self = (mp_esp_i2c_obj_t *)MP_OBJ_TO_PTR(args[ARG_self].u_obj);
    uint8_t addr = (uint8_t)args[ARG_addr].u_int;
    bool stop = (bool)args[ARG_stop].u_bool;

    vstr_t vstr;
    vstr_init_len(&vstr, args[ARG_num_bytes].u_int);
        
    uint8_t buffer[I2C_BUF_MIN_SIZE] = { 0 };
    i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
    i2c_master_start(handle);
    i2c_master_write_byte(handle, addr << 1 | I2C_MASTER_WRITE, true);
    i2c_master_read(handle, (uint8_t *)vstr.buf, vstr.len, I2C_MASTER_LAST_NACK);
    
    if (stop) {
        i2c_master_stop(handle);
    }

    esp_err_t err = i2c_master_cmd_begin(
        self->port, handle,
        (((1 + vstr.len) * 8000000) / self->freq) / portTICK_PERIOD_MS
    );

    i2c_cmd_link_delete_static(handle);
    
    if (err == ESP_FAIL) {
        mp_raise_OSError(MP_ENODEV);
    } else if (err == ESP_ERR_TIMEOUT) {
        mp_raise_OSError(MP_ETIMEDOUT);
    } else if (err != ESP_OK) {
        mp_raise_OSError(abs(err));
    }
    
    return mp_obj_new_bytes_from_vstr(&vstr);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_esp_i2c_readfrom_obj, 3, mp_esp_i2c_readfrom);


STATIC mp_obj_t mp_esp_i2c_readfrom_into(size_t n_args, const mp_obj_t *args)
{
    enum { ARG_self, ARG_addr, ARG_buf, ARG_stop };
    
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,   MP_ARG_REQUIRED | MP_ARG_OBJ  },
        { MP_QSTR_addr,   MP_ARG_REQUIRED | MP_ARG_INT  },
        { MP_QSTR_buf,    MP_ARG_REQUIRED | MP_ARG_OBJ  },
        { MP_QSTR_stop,   MP_ARG_BOOL, { .u_bool = true } },
    };
    
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_esp_i2c_obj_t *self = (mp_esp_i2c_obj_t *)MP_OBJ_TO_PTR(args[ARG_self].u_obj);
    uint8_t addr = (uint8_t)args[ARG_addr].u_int;
    bool stop = (bool)args[ARG_stop].u_bool;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_WRITE);
    
    uint8_t buffer[I2C_BUF_MIN_SIZE] = { 0 };
    i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
    i2c_master_start(handle);
    i2c_master_write_byte(handle, addr << 1 | I2C_MASTER_WRITE, true);
    i2c_master_read(handle, bufinfo.buf, bufinfo.len, I2C_MASTER_LAST_NACK);
    
    if (stop) {
        i2c_master_stop(handle);
    }

    esp_err_t err = i2c_master_cmd_begin(
        self->port, handle,
        (((1 + bufinfo.len) * 8000000) / self->freq) / portTICK_PERIOD_MS
    );

    i2c_cmd_link_delete_static(handle);
    
    if (err == ESP_FAIL) {
        mp_raise_OSError(MP_ENODEV);
    } else if (err == ESP_ERR_TIMEOUT) {
        mp_raise_OSError(MP_ETIMEDOUT);
    } else if (err != ESP_OK) {
        mp_raise_OSError(abs(err));
    }
    
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_esp_i2c_readfrom_into_obj, 3, mp_esp_i2c_readfrom_into);


STATIC mp_obj_t mp_esp_i2c_writeto(size_t n_args, const mp_obj_t *args)
{
    enum { ARG_self, ARG_addr, ARG_buf, ARG_stop };
    
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,   MP_ARG_REQUIRED | MP_ARG_OBJ  },
        { MP_QSTR_addr,   MP_ARG_REQUIRED | MP_ARG_INT  },
        { MP_QSTR_buf,    MP_ARG_REQUIRED | MP_ARG_OBJ  },
        { MP_QSTR_stop,   MP_ARG_BOOL, { .u_bool = true } },
    };
    
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_esp_i2c_obj_t *self = (mp_esp_i2c_obj_t *)MP_OBJ_TO_PTR(args[ARG_self].u_obj);
    uint8_t addr = (uint8_t)args[ARG_addr].u_int;
    bool stop = (bool)args[ARG_stop].u_bool;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_READ);
    
    uint8_t buffer[I2C_BUF_MIN_SIZE] = { 0 };
    i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
    i2c_master_start(handle);
    i2c_master_write_byte(handle, addr << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write(handle, bufinfo.buf, bufinfo.len, true);
    
    if (stop) {
        i2c_master_stop(handle);
    }

    esp_err_t err = i2c_master_cmd_begin(
        self->port, handle,
        (((1 + bufinfo.len) * 8000000) / self->freq) / portTICK_PERIOD_MS
    );

    i2c_cmd_link_delete_static(handle);
    
    if (err == ESP_FAIL) {
        mp_raise_OSError(MP_ENODEV);
    } else if (err == ESP_ERR_TIMEOUT) {
        mp_raise_OSError(MP_ETIMEDOUT);
    } else if (err != ESP_OK) {
        mp_raise_OSError(abs(err));
    }
    
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_esp_i2c_writeto_obj, 3, mp_esp_i2c_writeto);


STATIC mp_obj_t mp_esp_i2c_writevto(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_addr, ARG_vector, ARG_stop };
    
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,   MP_ARG_REQUIRED | MP_ARG_OBJ  },
        { MP_QSTR_addr,   MP_ARG_REQUIRED | MP_ARG_INT  },
        { MP_QSTR_vector, MP_ARG_REQUIRED | MP_ARG_OBJ  },
        { MP_QSTR_stop,   MP_ARG_BOOL, { .u_bool = true } },
    };
    
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_esp_i2c_obj_t *self = (mp_esp_i2c_obj_t *)MP_OBJ_TO_PTR(args[ARG_self].u_obj);
    uint8_t addr = (uint8_t)args[ARG_addr].u_int;
    bool stop = (bool)args[ARG_stop].u_bool;

    size_t nitems;
    const mp_obj_t *items;
    
    mp_obj_get_array(args[ARG_vector].u_obj, &nitems, (mp_obj_t **)&items);

    uint32_t data_len = 0;
    
    uint8_t buffer[I2C_BUF_MIN_SIZE] = { 0 };
    i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
    i2c_master_start(handle);
    i2c_master_write_byte(handle, addr << 1 | I2C_MASTER_WRITE, true);
        
    for (; nitems--; ++items) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(*items, &bufinfo, MP_BUFFER_READ);
        if (bufinfo.len > 0) {
            data_len += bufinfo.len;
            i2c_master_write(handle, bufinfo.buf, bufinfo.len, true);
        }
    }
    
    if (stop) {
        i2c_master_stop(handle);
    }

    esp_err_t err = i2c_master_cmd_begin(
        self->port, handle,
        (((1 + data_len) * 8000000) / self->freq) / portTICK_PERIOD_MS
    );

    i2c_cmd_link_delete_static(handle);
    
    if (err == ESP_FAIL) {
        mp_raise_OSError(MP_ENODEV);
    } else if (err == ESP_ERR_TIMEOUT) {
        mp_raise_OSError(MP_ETIMEDOUT);
    } else if (err != ESP_OK) {
        mp_raise_OSError(abs(err));
    }
    
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_esp_i2c_writevto_obj, 3, mp_esp_i2c_writevto);


STATIC mp_obj_t mp_esp_i2c_readfrom_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_addr, ARG_memaddr, ARG_num_bytes, ARG_addrsize };
    
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,      MP_ARG_REQUIRED | MP_ARG_OBJ  },
        { MP_QSTR_addr,      MP_ARG_REQUIRED | MP_ARG_INT  },
        { MP_QSTR_memaddr,   MP_ARG_REQUIRED | MP_ARG_INT  },
        { MP_QSTR_num_bytes, MP_ARG_REQUIRED | MP_ARG_OBJ  },
        { MP_QSTR_addrsize,  MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
    };
    
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_esp_i2c_obj_t *self = (mp_esp_i2c_obj_t *)MP_OBJ_TO_PTR(args[ARG_self].u_obj);

    // create the buffer to store data into
    vstr_t vstr;
    vstr_init_len(&vstr, args[ARG_num_bytes].u_int);
    
    uint8_t memaddr_buf[4];
    size_t memaddr_len = fill_memaddr_buf(&memaddr_buf[0], args[ARG_memaddr].u_int, args[ARG_addrsize].u_int);
    
    esp_err_t err = i2c_master_write_read_device(
        self->port, args[ARG_addr].u_int, memaddr_buf, memaddr_len, (uint8_t *)vsr.buf, vstr.len,
        (((2 + memaddr_len + vstr.len) * 8000000) / self->freq) / portTICK_PERIOD_MS
    );
        
    if (err == ESP_FAIL) {
        mp_raise_OSError(MP_ENODEV);
    } else if (err == ESP_ERR_TIMEOUT) {
        mp_raise_OSError(MP_ETIMEDOUT);
    } else if (err != ESP_OK) {
        mp_raise_OSError(abs(err));
    }
    
    return mp_obj_new_bytes_from_vstr(&vstr);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_esp_i2c_readfrom_mem_obj, 4, mp_esp_i2c_readfrom_mem);


STATIC mp_obj_t mp_esp_i2c_readfrom_mem_into(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_addr, ARG_memaddr, ARG_buf, ARG_addrsize };
    
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,     MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_addr,     MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_memaddr,  MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_buf,      MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_addrsize, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    mp_esp_i2c_obj_t *self = (mp_esp_i2c_obj_t *)MP_OBJ_TO_PTR(args[ARG_self].u_obj);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_WRITE);
    
    uint8_t memaddr_buf[4];
    size_t memaddr_len = fill_memaddr_buf(&memaddr_buf[0], args[ARG_memaddr].u_int, args[ARG_addrsize].u_int);
    
    esp_err_t err = i2c_master_write_read_device(
        self->port, args[ARG_addr].u_int, memaddr_buf, memaddr_len, bufinfo.buf, bufinfo.len,
        (((2 + memaddr_len + bufinfo.len) * 8000000) / self->freq) / portTICK_PERIOD_MS
    );
        
    if (err == ESP_FAIL) {
        mp_raise_OSError(MP_ENODEV);
    } else if (err == ESP_ERR_TIMEOUT) {
        mp_raise_OSError(MP_ETIMEDOUT);
    } else if (err != ESP_OK) {
        mp_raise_OSError(abs(err));
    }
    
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_esp_i2c_readfrom_mem_into_obj, 4, mp_esp_i2c_readfrom_mem_into);


STATIC mp_obj_t mp_esp_i2c_writeto_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_addr, ARG_memaddr, ARG_buf, ARG_addrsize };
    
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,     MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_addr,     MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_memaddr,  MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_buf,      MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_addrsize, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
    };
    
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    mp_esp_i2c_obj_t *self = (mp_esp_i2c_obj_t *)MP_OBJ_TO_PTR(args[ARG_self].u_obj);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[ARG_buf].u_obj, &bufinfo, MP_BUFFER_READ);
    
    uint8_t memaddr_buf[4];
    size_t memaddr_len = fill_memaddr_buf(&memaddr_buf[0], args[ARG_memaddr].u_int, args[ARG_addrsize].u_int);
    
    uint8_t addr = (uint8_t)args[ARG_addr].u_int;
    
    uint8_t buffer[I2C_BUF_MIN_SIZE] = { 0 };
    i2c_cmd_handle_t handle = i2c_cmd_link_create_static(buffer, sizeof(buffer));
    i2c_master_start(handle);
    i2c_master_write_byte(handle, addr << 1 | I2C_MASTER_WRITE, true);
    
    i2c_master_write(handle, memaddr_buf, memaddr_len, true);
    i2c_master_write(handle, bufinfo.buf, bufinfo.len, true);
    
    i2c_master_stop(handle);

    esp_err_t err = i2c_master_cmd_begin(
        self->port, handle,
        (((1 + memaddr_len + bufinfo.len) * 8000000) / self->freq) / portTICK_PERIOD_MS
    );

    i2c_cmd_link_delete_static(handle);
    
    if (err == ESP_FAIL) {
        mp_raise_OSError(MP_ENODEV);
    } else if (err == ESP_ERR_TIMEOUT) {
        mp_raise_OSError(MP_ETIMEDOUT);
    } else if (err != ESP_OK) {
        mp_raise_OSError(abs(err));
    }
    
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_esp_i2c_writeto_mem_obj, 4, mp_esp_i2c_writeto_mem);


STATIC const mp_rom_map_elem_t mp_esp_i2c_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_scan), MP_ROM_PTR(&mp_esp_i2c_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom), MP_ROM_PTR(&mp_esp_i2c_readfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_into), MP_ROM_PTR(&mp_esp_i2c_readfrom_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto), MP_ROM_PTR(&mp_esp_i2c_writeto_obj) },
    { MP_ROM_QSTR(MP_QSTR_writevto), MP_ROM_PTR(&mp_esp_i2c_writevto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem), MP_ROM_PTR(&mp_esp_i2c_readfrom_mem_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem_into), MP_ROM_PTR(&mp_esp_i2c_readfrom_mem_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto_mem), MP_ROM_PTR(&mp_esp_i2c_writeto_mem_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_esp_i2c_locals_dict, mp_esp_i2c_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_esp_i2c_type,
    MP_QSTR_I2C,
    MP_TYPE_FLAG_NONE,
    make_new, mp_esp_i2c_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_esp_i2c_locals_dict
);

STATIC const mp_map_elem_t mp_module_esp_i2c_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_esp_i2c) },
    { MP_ROM_QSTR(MP_QSTR_I2C),      (mp_obj_t)&mp_esp_i2c_type       }
};

STATIC MP_DEFINE_CONST_DICT(mp_module_esp_i2c_globals, mp_module_esp_i2c_globals_table);


const mp_obj_module_t mp_module_esp_i2c = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_esp_i2c_globals,
};

MP_REGISTER_MODULE(MP_QSTR_esp_i2c, mp_module_esp_i2c);

