/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 "Eric Poulsen" <eric@zyxod.com>
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../include/esp_spi.h"

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"
#include "mphalport.h"

#include "driver/spi_master.h"
#include "driver/spi_common.h"
#include "soc/spi_pins.h"

/* SPI DEVICE CLASS
/*********************************************************************************************************/

void _esp_spi_cb_isr(esp32_hw_spi_dev_obj_t *self, mp_obj_t cb, mp_obj_t user_data)
{
    volatile uint32_t sp = (uint32_t)esp_cpu_get_sp();

    // Calling micropython from ISR
    // See: https://github.com/micropython/micropython/issues/4895
    void *old_state = mp_thread_get_state();

    mp_state_thread_t ts; // local thread state for the ISR
    mp_thread_set_state(&ts);
    mp_stack_set_top((void*)sp); // need to include in root-pointer scan
    mp_stack_set_limit(CONFIG_FREERTOS_IDLE_TASK_STACKSIZE - 1024); // tune based on ISR thread stack size
    mp_locals_set(mp_state_ctx.thread.dict_locals); // use main thread's locals
    mp_globals_set(mp_state_ctx.thread.dict_globals); // use main thread's globals

    mp_sched_lock(); // prevent VM from switching to another MicroPython thread
    gc_lock(); // prevent memory allocation

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t args[2] = { MP_OBJ_FROM_PTR(self), user_data };
        mp_call_function_n_kw(cb, 2, 0, &args[0]);

        mp_call_function_n_kw(cb, 0, 0, NULL);
        nlr_pop();
    } else {
        ets_printf("Uncaught exception in IRQ callback handler!\n");
        mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));  // changed to &mp_plat_print to fit this context
    }

    gc_unlock();
    mp_sched_unlock();

    mp_thread_set_state(old_state);
    mp_hal_wake_main_task_from_isr();
}


// called when esp_lcd_panel_draw_bitmap is completed
bool bus_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)user_ctx;


    self->trans_done = true;
    return false;
}


static void _esp32_pre_cb(spi_transaction_t *trans)
{
    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)trans->user;

    if (self->trans_start_cb != mp_const_none && mp_obj_is_callable(self->trans_start_cb)) {
        _esp_spi_cb_isr(self, self->trans_start_cb, self->trans_start_user_data);
    }
}

static void _esp32_post_cb(spi_transaction_t *trans)
{
    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)trans->user;
    
    esp32_spi_trans_descriptor_t *lcd_trans = __containerof(trans, esp32_spi_trans_descriptor_t, base);
    
    if (lcd_trans->trans_done) {
        if (self->trans_end_cb != mp_const_none && mp_obj_is_callable(self->trans_end_cb)) {
            _esp_spi_cb_isr(self, self->trans_end_cb, self->trans_end_user_data);
        }
    }
}


mp_obj_t esp32_hw_spi_dev_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum {
        ARG_spi_bus,
        ARG_baudrate,
        ARG_cs,
        ARG_polarity,
        ARG_phase,
        ARG_bits,
        ARG_firstbit,
        ARG_three_wire,
        ARG_cs_active_pos,
        ARG_half_duplex,
        ARG_clock_as_cs,
        ARG_queue_size,
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_spi_bus,       MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_baudrate,      MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_cs,            MP_ARG_KW_ONLY | MP_ARG_INT,   {.u_int  = -1 }  },
        { MP_QSTR_polarity,      MP_ARG_KW_ONLY | MP_ARG_INT,   {.u_int  = 0 } },
        { MP_QSTR_phase,         MP_ARG_KW_ONLY | MP_ARG_INT,   {.u_int  = 0 } },
        { MP_QSTR_bits,          MP_ARG_KW_ONLY | MP_ARG_INT,   {.u_int  = 8 } },
        { MP_QSTR_firstbit,      MP_ARG_KW_ONLY | MP_ARG_INT,   {.u_int  = MICROPY_PY_MACHINE_SPI_MSB } },
        { MP_QSTR_three_wire,    MP_ARG_KW_ONLY | MP_ARG_BOOL,  {.u_bool = false } },
        { MP_QSTR_cs_active_pos, MP_ARG_KW_ONLY | MP_ARG_BOOL,  {.u_bool = false } },
        { MP_QSTR_half_duplex,   MP_ARG_KW_ONLY | MP_ARG_BOOL,  {.u_bool = false } },
        { MP_QSTR_clock_as_cs,   MP_ARG_KW_ONLY | MP_ARG_BOOL,  {.u_bool = false } },
        { MP_QSTR_queue_size,    MP_ARG_KW_ONLY | MP_ARG_INT,   {.u_int  = 5 } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    size_t trans_pool_count = (size_t)args[ARG_queue_size].u_int;
    esp32_hw_spi_dev_obj_t *self = NULL;
    self = calloc(1, sizeof(esp32_hw_spi_dev_obj_t) + sizeof(esp32_spi_trans_descriptor_t) * trans_pool_count);
    assert(self != NULL)

    self->base.type = &esp32_hw_spi_dev_type;

    self->spi_bus = (esp32_hw_spi_bus_obj_t *)args[ARG_spi_bus].u_obj;
    self->trans_pool_count = trans_pool_count;
    self->bits = (uint8_t)args[ARG_bits].u_int;

    uint32_t flags = args[ARG_firstbit].u_int == MICROPY_PY_MACHINE_SPI_LSB ? SPI_DEVICE_BIT_LSBFIRST : 0;
    if (three_wire) flags |= SPI_DEVICE_3WIRE;
    if (cs_active_pos) flags |= SPI_DEVICE_POSITIVE_CS;
    if (half_duplex) flags |= SPI_DEVICE_HALFDUPLEX;
    if (clock_as_cs) flags |= SPI_DEVICE_CLK_AS_CS;

    self->devcfg = (spi_device_interface_config_t){
        .clock_speed_hz = spi_get_actual_clock(APB_CLK_FREQ, args[ARG_baudrate].u_int, 0),
        .mode = args[ARG_phase].u_int | (args[ARG_polarity].u_int << 1),
        .spics_io_num = (int)args[ARG_cs].u_int,
        .queue_size = self->trans_pool_count,
        .flags = flags,
        .pre_cb = _esp32_pre_cb,
        .post_cb = _esp32_post_cb
    };

    self->trans_start_cb = mp_const_none;
    self->trans_start_user_data = mp_const_none;
    self->trans_end_cb = mp_const_none;
    self->trans_end_user_data = mp_const_none;

    esp_err_t ret = ESP_OK;

    ret = spi_bus_add_device(self->spi_bus->host, &self->devcfg, &self->spi_dev);
    check_esp_err(ret);

    size_t max_transfer_sz = 0;
    ret = spi_bus_get_max_transaction_len(self->spi_bus->host, &max_transfer_sz);
    check_esp_err(ret);

    self->max_transfer_sz = max_transfer_sz;

    return MP_OBJ_FROM_PTR(self);
}


mp_obj_t esp32_hw_spi_dev_trans_start_cb(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_callback, ARG_user_data };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED           },
        { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED           },
        { MP_QSTR_user_data,    MP_ARG_OBJ, { .u_obj = mp_const_none } },

    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)args[ARG_self].u_obj;

    self->trans_start_cb = args[ARG_callback].u_obj;
    self->trans_start_user_data = args[ARG_user_data].u_obj;

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(esp32_hw_spi_dev_trans_start_cb_obj, 2, esp32_hw_spi_dev_trans_start_cb);


mp_obj_t esp32_hw_spi_dev_trans_end_cb(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_callback, ARG_user_data };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED           },
        { MP_QSTR_callback,     MP_ARG_OBJ | MP_ARG_REQUIRED           },
        { MP_QSTR_user_data,    MP_ARG_OBJ, { .u_obj = mp_const_none } },

    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)args[ARG_self].u_obj;

    self->trans_end_cb = args[ARG_callback].u_obj;
    self->trans_end_user_data = args[ARG_user_data].u_obj;

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(esp32_hw_spi_dev_trans_end_cb_obj, 2, esp32_hw_spi_dev_trans_end_cb);



mp_obj_t esp32_hw_spi_dev_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_read_buf };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED           },
        { MP_QSTR_read_buf,     MP_ARG_OBJ | MP_ARG_REQUIRED           },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)args[ARG_self].u_obj;

    mp_obj_t read_buf = args[ARG_read_buf].u_obj;

    mp_buffer_info_t read_bufinfo;
    mp_get_buffer_raise(read_buf, &read_bufinfo, MP_BUFFER_WRITE);

    esp_err_t ret = ESP_OK;
    spi_transaction_t *spi_trans = NULL;
    esp32_spi_trans_descriptor_t *lcd_trans = NULL;

    esp_err_t ret = spi_device_acquire_bus(self->spi_dev, portMAX_DELAY);
    check_esp_err(ret);

    // Round to nearest whole set of bits
    size_t bits_to_recv = read_bufinfo.len * 8 / self->bits * self->bits;

    size_t rx_size = bits_to_recv / 8;
    const uint8_t * rx_buf = (const uint8_t *)read_bufinfo.buf;

    // to fill the rx buffer we are using a DMA transfer if the supplied buffer was allocated in DMA memory
    // This is looking code in the even the buffer is larger than a single transaction is able to handle.

    do {
        size_t chunk_size = rx_size;

        if (self->num_trans_inflight < self->trans_pool_count) {
            // get the next available transaction
            lcd_trans = &self->trans_pool[self->num_trans_inflight];
        } else {
            // transaction pool has used up, recycle one transaction
            MP_THREAD_GIL_EXIT();
            ret = spi_device_get_trans_result(self->spi_dev, &spi_trans, portMAX_DELAY);
            MP_THREAD_GIL_ENTER();
            check_esp_err(ret);

            lcd_trans = __containerof(spi_trans, esp32_spi_trans_descriptor_t, base);
            self->num_trans_inflight--;
        }
        memset(lcd_trans, 0, sizeof(esp32_spi_trans_descriptor_t));
        lcd_trans->base.flags = SPI_TRANS_USE_RXDATA;

        if (chunk_size > self->max_transfer_sz) {
            // cap the transfer size to the maximum supported by the bus
            chunk_size = self->max_transfer_sz;
            lcd_trans->base.flags |= SPI_TRANS_CS_KEEP_ACTIVE;
        } else {
            // mark trans_done only at the last round to avoid premature completion callback
            lcd_trans->flags.trans_done = true;
            lcd_trans->base.flags &= ~SPI_TRANS_CS_KEEP_ACTIVE;
        }

        lcd_trans->base.user = self;

        lcd_trans->base.length = chunk_size * 8; // transaction length is in bits
        lcd_trans->base.rx_buffer = rx_buf;
        if (spi_panel_io->flags.octal_mode) {
            // use 8 lines for transmitting command, address and data
            lcd_trans->base.flags |= (SPI_TRANS_MULTILINE_CMD | SPI_TRANS_MULTILINE_ADDR | SPI_TRANS_MODE_OCT);
        }

        // data is usually large, using queue+blocking mode
        ret = spi_device_queue_trans(self->spi_dev, &lcd_trans->base, portMAX_DELAY);
        check_esp_err(ret);
        self->num_trans_inflight++;

        // move on to the next chunk
        rx_buf = (const uint8_t *)rx_buf + chunk_size;
        rx_size -= chunk_size;
    } while (rx_size > 0); // continue while we have remaining data to transmit

    spi_device_release_bus(self->spi_dev);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(esp32_hw_spi_dev_read_obj, 2, esp32_hw_spi_dev_read);


mp_obj_t esp32_hw_spi_dev_write(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_write_buf };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,         MP_ARG_OBJ | MP_ARG_REQUIRED           },
        { MP_QSTR_write_buf,    MP_ARG_OBJ | MP_ARG_REQUIRED           },

    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)args[ARG_self].u_obj;

    mp_buffer_info_t write_bufinfo;
    mp_get_buffer_raise(args[ARG_write_buf].u_obj, &write_bufinfo, MP_BUFFER_READ);

    esp_err_t ret = ESP_OK;
    spi_transaction_t *spi_trans = NULL;
    esp32_spi_trans_descriptor_t *lcd_trans = NULL;

    esp_err_t ret = spi_device_acquire_bus(self->spi_dev, portMAX_DELAY);
    check_esp_err(ret);

    size_t bits_to_send = write_bufinfo.len * 8 / self->bits * self->bits;

    size_t tx_size = bits_to_send / 8;
    const uint8_t * tx_buf = (const uint8_t *)write_bufinfo.buf;

    // to write the tx buffer we are using a DMA transfer if the supplied buffer was allocated in DMA memory
    // This is looking code in the even the buffer is larger than a single transaction is able to handle.

    do {
        size_t chunk_size = tx_size;

        if (self->num_trans_inflight < self->trans_pool_count) {
            // get the next available transaction
            lcd_trans = &self->trans_pool[self->num_trans_inflight];
        } else {
            // transaction pool has used up, recycle one transaction
            MP_THREAD_GIL_EXIT();
            ret = spi_device_get_trans_result(self->spi_dev, &spi_trans, portMAX_DELAY);
            MP_THREAD_GIL_ENTER();
            check_esp_err(ret);

            lcd_trans = __containerof(spi_trans, esp32_spi_trans_descriptor_t, base);
            self->num_trans_inflight--;
        }
        memset(lcd_trans, 0, sizeof(esp32_spi_trans_descriptor_t));
        lcd_trans->base.flags = SPI_TRANS_USE_TXDATA;

        // SPI per-transfer size has its limitation, if the buffer is too big, we need to split it into multiple chunks
        if (chunk_size > self->max_transfer_sz) {
            // cap the transfer size to the maximum supported by the bus
            chunk_size = self->max_transfer_sz;
            lcd_trans->base.flags |= SPI_TRANS_CS_KEEP_ACTIVE;
        } else {
            // mark trans_done only at the last round to avoid premature completion callback
            lcd_trans->flags.trans_done = true;
            lcd_trans->base.flags &= ~SPI_TRANS_CS_KEEP_ACTIVE;
        }

        lcd_trans->base.user = self;

        lcd_trans->base.length = chunk_size * 8; // transaction length is in bits
        lcd_trans->base.tx_buffer = tx_buf;
        if (spi_panel_io->flags.octal_mode) {
            // use 8 lines for transmitting command, address and data
            lcd_trans->base.flags |= (SPI_TRANS_MULTILINE_CMD | SPI_TRANS_MULTILINE_ADDR | SPI_TRANS_MODE_OCT);
        }

        // data is usually large, using queue+blocking mode
        ret = spi_device_queue_trans(self->spi_dev, &lcd_trans->base, portMAX_DELAY);
        check_esp_err(ret);
        self->num_trans_inflight++;

        // move on to the next chunk
        tx_buf = (const uint8_t *)tx_buf + chunk_size;
        tx_size -= chunk_size;
    } while (tx_size > 0); // continue while we have remaining data to transmit

    spi_device_release_bus(self->spi_dev);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(esp32_hw_spi_dev_write_obj, 2, esp32_hw_spi_dev_write);


mp_obj_t esp32_hw_spi_dev_write_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_write_buf, ARG_read_buf };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,      MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_write_buf, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_read_buf,  MP_ARG_OBJ | MP_ARG_REQUIRED },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)args[ARG_self].u_obj;

    mp_obj_t write_buf = args[ARG_write_buf].u_obj;
    mp_obj_t read_buf = args[ARG_read_buf].u_obj;

    mp_buffer_info_t write_bufinfo;
    mp_get_buffer_raise(write_buf, &write_bufinfo, MP_BUFFER_READ);

    mp_buffer_info_t read_bufinfo;
    mp_get_buffer_raise(read_buf, &read_bufinfo, MP_BUFFER_WRITE);

    esp_err_t ret = ESP_OK;
    spi_transaction_t *spi_trans = NULL;
    esp32_spi_trans_descriptor_t *lcd_trans = NULL;

    esp_err_t ret = spi_device_acquire_bus(self->spi_dev, portMAX_DELAY);
    check_esp_err(ret);

    // Round to nearest whole set of bits
    int bits_to_send = write_bufinfo.len * 8 / self->bits * self->bits;

    // Round to nearest whole set of bits
    size_t bits_to_recv = read_bufinfo.len * 8 / self->bits * self->bits;

    // before issue a polling transaction, need to wait queued transactions finished
    size_t num_trans_inflight = self->num_trans_inflight;
    for (size_t i = 0; i < num_trans_inflight; i++) {
        MP_THREAD_GIL_EXIT();
        ret = spi_device_get_trans_result(self->spi_dev, &spi_trans, portMAX_DELAY);
        MP_THREAD_GIL_ENTER();
        check_esp_err(ret);
        self->num_trans_inflight--;
    }

    lcd_trans = &self->trans_pool[0];
    memset(lcd_trans, 0, sizeof(esp32_spi_trans_descriptor_t));
    // spi_lcd_prepare_cmd_buffer(spi_panel_io, &lcd_cmd);
    lcd_trans->base.user = self;
    lcd_trans->base.length = bits_to_send;
    lcd_trans->base.tx_buffer = write_bufinfo.buf;

    lcd_trans->base.flags = SPI_TRANS_USE_TXDATA

    if (self->spi_bus->octal_mode) {
        // use 8 lines for transmitting command, address and data
        lcd_trans->base.flags |= (SPI_TRANS_MULTILINE_CMD | SPI_TRANS_MULTILINE_ADDR | SPI_TRANS_MODE_OCT);
    }
    // command is short, using polling mode
    ret = spi_device_polling_transmit(self->spi_dev, &lcd_trans->base);
    check_esp_err(ret);

    size_t rx_size = bits_to_recv / 8;
    const uint8_t * rx_buf = (const uint8_t *)read_bufinfo.buf;

    // to fill the rx buffer we are using a DMA transfer if the supplied buffer was allocated in DMA memory
    // This is looking code in the even the buffer is larger than a single transaction is able to handle.

    do {
        size_t chunk_size = rx_size;

        if (self->num_trans_inflight < self->trans_pool_count) {
            // get the next available transaction
            lcd_trans = &self->trans_pool[self->num_trans_inflight];
        } else {
            // transaction pool has used up, recycle one transaction
            MP_THREAD_GIL_EXIT();
            ret = spi_device_get_trans_result(self->spi_dev, &spi_trans, portMAX_DELAY);
            MP_THREAD_GIL_ENTER();
            check_esp_err(ret);

            lcd_trans = __containerof(spi_trans, esp32_spi_trans_descriptor_t, base);
            self->num_trans_inflight--;
        }
        memset(lcd_trans, 0, sizeof(esp32_spi_trans_descriptor_t));
        lcd_trans->base.flags = SPI_TRANS_USE_RXDATA;

        if (chunk_size > self->max_transfer_sz) {
            // cap the transfer size to the maximum supported by the bus
            chunk_size = self->max_transfer_sz;
            lcd_trans->base.flags |= SPI_TRANS_CS_KEEP_ACTIVE;
        } else {
            // mark trans_done only at the last round to avoid premature completion callback
            lcd_trans->flags.trans_done = true;
            lcd_trans->base.flags &= ~SPI_TRANS_CS_KEEP_ACTIVE;
        }

        lcd_trans->base.user = self;

        lcd_trans->base.length = chunk_size * 8; // transaction length is in bits
        lcd_trans->base.rx_buffer = rx_buf;
        if (spi_panel_io->flags.octal_mode) {
            // use 8 lines for transmitting command, address and data
            lcd_trans->base.flags |= (SPI_TRANS_MULTILINE_CMD | SPI_TRANS_MULTILINE_ADDR | SPI_TRANS_MODE_OCT);
        }

        // data is usually large, using queue+blocking mode
        ret = spi_device_queue_trans(self->spi_dev, &lcd_trans->base, portMAX_DELAY);
        check_esp_err(ret);
        self->num_trans_inflight++;

        // move on to the next chunk
        rx_buf = (const uint8_t *)rx_buf + chunk_size;
        rx_size -= chunk_size;
    } while (rx_size > 0); // continue while we have remaining data to transmit

    spi_device_release_bus(self->spi_dev);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(esp32_hw_spi_dev_write_read_obj, 3, esp32_hw_spi_dev_write_read);


STATIC mp_obj_t esp32_hw_spi_dev_del(mp_obj_t self_in)
{
    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)self_in;
    esp_err_t ret = ESP_OK;
    spi_transaction_t *spi_trans = NULL;

    // wait all pending transaction to finish
    size_t num_trans_inflight = self->num_trans_inflight;
    for (size_t i = 0; i < num_trans_inflight; i++) {
        ret = spi_device_get_trans_result(self->spi_dev, &spi_trans, portMAX_DELAY);
        if (ret != ESP_OK) break;
        self->num_trans_inflight--;
    }
    
    spi_bus_remove_device(self->spi_dev);
        
    free(self);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(esp32_hw_spi_dev_del_obj, esp32_hw_spi_dev_del);


STATIC mp_obj_t esp32_hw_spi_dev_deinit(mp_obj_t self_in)
{
    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)self_in;
    esp_err_t ret = ESP_OK;
    spi_transaction_t *spi_trans = NULL;

    // wait all pending transaction to finish
    size_t num_trans_inflight = self->num_trans_inflight;
    for (size_t i = 0; i < num_trans_inflight; i++) {
        ret = spi_device_get_trans_result(self->spi_dev, &spi_trans, portMAX_DELAY);
        check_esp_err(ret);
        self->num_trans_inflight--;
    }
    
    ret = spi_bus_remove_device(self->spi_dev);
    check_esp_err(ret);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(esp32_hw_spi_dev_deinit_obj, esp32_hw_spi_dev_deinit);


STATIC mp_obj_t esp32_hw_spi_dev_get_host(mp_obj_t self_in) {
    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)self_in;
    return MP_OBJ_FROM_PTR(self->spi_bus);
}

MP_DEFINE_CONST_FUN_OBJ_1(esp32_hw_spi_dev_get_host_obj, esp32_hw_spi_dev_get_host);


STATIC mp_obj_t esp32_hw_spi_dev_get_bus(mp_obj_t self_in) {
    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)self_in;
    return mp_obj_new_int(self->spi_bus->host);
}

MP_DEFINE_CONST_FUN_OBJ_1(esp32_hw_spi_dev_get_bus_obj, esp32_hw_spi_dev_get_bus);


STATIC const mp_rom_map_elem_t esp32_hw_spi_dev_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read),                    MP_ROM_PTR(&esp32_hw_spi_dev_read_obj)           },
    { MP_ROM_QSTR(MP_QSTR_write),                   MP_ROM_PTR(&esp32_hw_spi_dev_write_obj)          },
    { MP_ROM_QSTR(MP_QSTR_write_read),              MP_ROM_PTR(&esp32_hw_spi_dev_write_read_obj)     },
    { MP_ROM_QSTR(MP_QSTR_register_trans_start_cb), MP_ROM_PTR(&esp32_hw_spi_dev_trans_start_cb_obj) },
    { MP_ROM_QSTR(MP_QSTR_register_trans_end_cb),   MP_ROM_PTR(&esp32_hw_spi_dev_trans_end_cb_obj)   },
    { MP_ROM_QSTR(MP_QSTR_get_bus),                 MP_ROM_PTR(&esp32_hw_spi_dev_get_bus_obj)        },
    { MP_ROM_QSTR(MP_QSTR_get_host),                MP_ROM_PTR(&esp32_hw_spi_dev_get_host_obj)       },
    { MP_ROM_QSTR(MP_QSTR_deinit),                  MP_ROM_PTR(&esp32_hw_spi_dev_deinit_obj)         },
    { MP_ROM_QSTR(MP_QSTR___del__),                 MP_ROM_PTR(&esp32_hw_spi_dev_del_obj)            },
    { MP_ROM_QSTR(MP_QSTR_MSB),                     MP_ROM_INT(MICROPY_PY_MACHINE_SPI_MSB)           },
    { MP_ROM_QSTR(MP_QSTR_LSB),                     MP_ROM_INT(MICROPY_PY_MACHINE_SPI_LSB)           },
};

MP_DEFINE_CONST_DICT(esp32_hw_spi_dev_locals_dict, esp32_hw_spi_dev_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    esp32_hw_spi_dev_type,
    MP_QSTR_Device,
    MP_TYPE_FLAG_NONE,
    make_new, esp32_hw_spi_dev_make_new,
    locals_dict, (mp_obj_dict_t *)&esp32_hw_spi_dev_locals_dict
);

/*********************************************************************************************************/

#if CONFIG_IDF_TARGET_ESP32
    #define ESP32_HW_SPI_MAX  2
#else
    #define ESP32_HW_SPI_MAX  1    
#endif


#if SOC_SPI_SUPPORT_OCT
    #define ESP32_HW_SPI_OCT_MAX  1
#else
    #define ESP32_HW_SPI_OCT_MAX  0
#endif


// Static objects mapping to SPI2 (and SPI3 if available) hardware peripherals.
STATIC esp32_hw_spi_bus_obj_t esp32_hw_spi_bus_obj[MICROPY_HW_SPI_MAX];


mp_obj_t esp32_hw_spi_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { 
        ARG_host, 
        ARG_mosi, 
        ARG_miso,
        ARG_sck,
        ARG_wp,
        ARG_hd,
        ARG_data4,
        ARG_data5,
        ARG_data6,
        ARG_data7,
        ARG_dual
    };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_host,  MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_mosi,  MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_miso,  MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_sck,   MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_wp,    MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_hd,    MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_data4, MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_data5, MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_data6, MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_data7, MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj  = MP_OBJ_NULL } },
        { MP_QSTR_dual,  MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false       } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
     mp_lcd_spi_bus_obj_t *self = NULL;
    
    esp32_hw_spi_default_pins_t default_pins_array[ESP32_HW_SPI_MAX] = {
        { 
            .mosi_io_num = SPI2_IOMUX_PIN_NUM_MOSI, 
            .miso_io_num = SPI2_IOMUX_PIN_NUM_MISO,
            .sclk_io_num = SPI2_IOMUX_PIN_NUM_CLK, 
            .quadhd_io_num = SPI2_IOMUX_PIN_NUM_HD,
            .quadwp_io_num = SPI2_IOMUX_PIN_NUM_WP,
            .data4_io_num = -1,
            .data5_io_num = -1,
            .data6_io_num = -1,
            .data7_io_num = -1,
        },
    #if CONFIG_IDF_TARGET_ESP32
        { 
            .mosi_io_num = SPI3_IOMUX_PIN_NUM_MOSI, 
            .miso_io_num = SPI3_IOMUX_PIN_NUM_MISO,
            .sclk_io_num = SPI3_IOMUX_PIN_NUM_CLK, 
            .quadhd_io_num = SPI3_IOMUX_PIN_NUM_HD,
            .quadwp_io_num = SPI3_IOMUX_PIN_NUM_WP,
            .data4_io_num = -1,
            .data5_io_num = -1,
            .data6_io_num = -1,
            .data7_io_num = -1,
        },
    #endif
    };
    #if SOC_SPI_SUPPORT_OCT
    esp32_hw_spi_default_pins_t default_oct_pins_array[ESP32_HW_SPI_MAX] = {
        { 
            .mosi_io_num = SPI2_IOMUX_PIN_NUM_MOSI_OCT, 
            .miso_io_num = SPI2_IOMUX_PIN_NUM_MISO_OCT,
            .sclk_io_num = SPI2_IOMUX_PIN_NUM_CLK_OCT, 
            .quadhd_io_num = SPI2_IOMUX_PIN_NUM_HD_OCT,
            .quadwp_io_num = SPI2_IOMUX_PIN_NUM_WP_OCT
            .data4_io_num = SPI2_IOMUX_PIN_NUM_IO4_OCT,
            .data5_io_num = SPI2_IOMUX_PIN_NUM_IO5_OCT,
            .data6_io_num = SPI2_IOMUX_PIN_NUM_IO6_OCT,
            .data7_io_num = SPI2_IOMUX_PIN_NUM_IO7_OCT,
        },
    };
    
    esp32_hw_spi_default_pins_t *default_oct_pins;
    #endif

    esp32_hw_spi_default_pins_t *default_pins;
    
    mp_int_t host = args[ARG_host].u_int;

    if (1 <= host && host <= ESP32_HW_SPI_MAX) {
        self = &esp32_hw_spi_bus_obj[host - 1];
        default_pins = &default_pins_array[host - 1];
        #if SOC_SPI_SUPPORT_OCT
        default_oct_pins = default_pins_array[host - 1];
        #endif
    } else {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%d) doesn't exist"), host);
        return mp_obj_none;
    }

    if (self->state == MACHINE_HW_SPI_STATE_INIT) {
        return MP_OBJ_FROM_PTR(self);
    }

    self->base.type = &esp32_hw_spi_bus_type;

    int8_t sck;
    int8_t mosi;
    int8_t miso;
    int8_t hd;
    int8_t wp;
    int8_t data4;
    int8_t data5;
    int8_t data6;
    int8_t data7;

    #if SOC_SPI_SUPPORT_OCT
    if (args[ARG_data4].u_obj == MP_OBJ_NULL) {
        data4 = -1;
    } else if (args[ARG_data4].u_obj == mp_const_none) {
        data4 = default_oct_pins->data4_io_num;
    } else {
        data4 = machine_pin_get_id(args[ARG_data4].u_obj);
    }
    
    if (args[ARG_data5].u_obj == MP_OBJ_NULL) {
        data5 = -1;
    } else if (args[ARG_data5].u_obj == mp_const_none) {
        data5 = default_oct_pins->data5_io_num;
    } else {
        data5 = machine_pin_get_id(args[ARG_data5].u_obj);
    }
    
    if (args[ARG_data6].u_obj == MP_OBJ_NULL) {
        data6 = -1;
    } else if (args[ARG_data6].u_obj == mp_const_none) {
        data6 = default_oct_pins->data6_io_num;
    } else {
        data6 = machine_pin_get_id(args[ARG_data6].u_obj);
    }
    
    if (args[ARG_data7].u_obj == MP_OBJ_NULL) {
        data7 = -1;
    } else if (args[ARG_data7].u_obj == mp_const_none) {
        data7 = default_oct_pins->data7_io_num;
    } else {
        data7 = machine_pin_get_id(args[ARG_data7].u_obj);
    }
    
    if (data2 != -1 && data5 != -1 && data6 != -1 && data7 != -1) {
        if (args[ARG_sck].u_obj == MP_OBJ_NULL) {
            sck = default_oct_pins->sck_io_num;
        } else if (args[ARG_sck].u_obj == mp_const_none) {
            sck = -1;
        } else {
            sck = machine_pin_get_id(args[ARG_sck].u_obj);
        }
        
        if (args[ARG_mosi].u_obj == MP_OBJ_NULL) {
            mosi = default_oct_pins->mosi_io_num;
        } else if (args[ARG_mosi].u_obj == mp_const_none) {
            mosi = -1;
        } else {
            mosi = machine_pin_get_id(args[ARG_mosi].u_obj);
        }
    
        if (args[ARG_miso].u_obj == MP_OBJ_NULL) {
            miso = default_oct_pins->miso_io_num;
        } else if (args[ARG_miso].u_obj == mp_const_none) {
            miso = -1;
        } else {
            miso = machine_pin_get_id(args[ARG_miso].u_obj);
        }
        
        if (args[ARG_cs].u_obj == MP_OBJ_NULL) {
            miso = default_oct_pins->cs_io_num;
        } else if (args[ARG_miso].u_obj == mp_const_none) {
            miso = -1;
        } else {
            miso = machine_pin_get_id(args[ARG_miso].u_obj);
        }
    
        if (args[ARG_hd].u_obj == MP_OBJ_NULL) {
            hd = -1;
        } else if (args[ARG_hd].u_obj == mp_const_none) {
            hd = default_oct_pins->quadhd_io_num;
        } else {
            hd = machine_pin_get_id(args[ARG_hd].u_obj);
        }
            
        if (args[ARG_wp].u_obj == MP_OBJ_NULL) {
            wp = -1;
        } else if (args[ARG_wp].u_obj == mp_const_none) {
            wp = default_oct_pins->quadwp_io_num;
        } else {
            wp = machine_pin_get_id(args[ARG_wp].u_obj);
        } 
    } else {
        data4 = -1;
        data5 = -1;
        data6 = -1;
        data7 = -1;
        
        if (args[ARG_sck].u_obj == MP_OBJ_NULL) {
            sck = default_pins->sck_io_num;
        } else if (args[ARG_sck].u_obj == mp_const_none) {
            sck = -1;
        } else {
            sck = machine_pin_get_id(args[ARG_sck].u_obj);
        }
        
        if (args[ARG_mosi].u_obj == MP_OBJ_NULL) {
            mosi = default_pins->mosi_io_num;
        } else if (args[ARG_mosi].u_obj == mp_const_none) {
            mosi = -1;
        } else {
            mosi = machine_pin_get_id(args[ARG_mosi].u_obj);
        }
    
        if (args[ARG_miso].u_obj == MP_OBJ_NULL) {
            miso = default_pins->miso_io_num;
        } else if (args[ARG_miso].u_obj == mp_const_none) {
            miso = -1;
        } else {
            miso = machine_pin_get_id(args[ARG_miso].u_obj);
        }

        if (args[ARG_hd].u_obj == MP_OBJ_NULL) {
            hd = -1;
        } else if (args[ARG_hd].u_obj == mp_const_none) {
            hd = default_pins->quadhd_io_num;
        } else {
            hd = machine_pin_get_id(args[ARG_hd].u_obj);
        }
            
        if (args[ARG_wp].u_obj == MP_OBJ_NULL) {
            wp = -1;
        } else if (args[ARG_wp].u_obj == mp_const_none) {
            wp = default_pins->quadwp_io_num;
        } else {
            wp = machine_pin_get_id(args[ARG_wp].u_obj);
        }
    }
    
    #else
    
    if (args[ARG_sck].u_obj == MP_OBJ_NULL) {
        sck = default_pins->sck_io_num;
    } else if (args[ARG_sck].u_obj == mp_const_none) {
        sck = -1;
    } else {
        sck = machine_pin_get_id(args[ARG_sck].u_obj);
    }
    
    if (args[ARG_mosi].u_obj == MP_OBJ_NULL) {
        mosi = default_pins->mosi_io_num;
    } else if (args[ARG_mosi].u_obj == mp_const_none) {
        mosi = -1;
    } else {
        mosi = machine_pin_get_id(args[ARG_mosi].u_obj);
    }

    if (args[ARG_miso].u_obj == MP_OBJ_NULL) {
        miso = default_pins->miso_io_num;
    } else if (args[ARG_miso].u_obj == mp_const_none) {
        miso = -1;
    } else {
        miso = machine_pin_get_id(args[ARG_miso].u_obj);
    }

    if (args[ARG_hd].u_obj == MP_OBJ_NULL) {
        hd = -1;
    } else if (args[ARG_hd].u_obj == mp_const_none) {
        hd = default_pins->quadhd_io_num;
    } else {
        hd = machine_pin_get_id(args[ARG_hd].u_obj);
    }
        
    if (args[ARG_wp].u_obj == MP_OBJ_NULL) {
        wp = -1;
    } else if (args[ARG_wp].u_obj == mp_const_none) {
        wp = default_pins->quadwp_io_num;
    } else {
        wp = machine_pin_get_id(args[ARG_wp].u_obj);
    }
    
    if (args[ARG_data4].u_obj == MP_OBJ_NULL) {
        data4 = -1;
    } else if (args[ARG_data4].u_obj == mp_const_none) {
        data4 = default_pins->data4_io_num;
    } else {
        data4 = machine_pin_get_id(args[ARG_data4].u_obj);
    }
    
    if (args[ARG_data5].u_obj == MP_OBJ_NULL) {
        data5 = -1;
    } else if (args[ARG_data5].u_obj == mp_const_none) {
        data5 = default_pins->data5_io_num;
    } else {
        data5 = machine_pin_get_id(args[ARG_data5].u_obj);
    }

    if (args[ARG_data6].u_obj == MP_OBJ_NULL) {
        data6 = -1;
    } else if (args[ARG_data6].u_obj == mp_const_none) {
        data6 = default_pins->data6_io_num;
    } else {
        data6 = machine_pin_get_id(args[ARG_data6].u_obj);
    }
        
    if (args[ARG_data7].u_obj == MP_OBJ_NULL) {
        data7 = -1;
    } else if (args[ARG_data7].u_obj == mp_const_none) {
        data7 = default_pins->data7_io_num;
    } else {
        data7 = machine_pin_get_id(args[ARG_data7].u_obj);
    }
    #endif


    uint32_t buscfg_flags = SPICOMMON_BUSFLAG_MASTER;

    if (hd != -1 && wp != -1 && data4 != -1 && data5 != -1 && data6 != -1 && data7 != -1) {
    #if SOC_SPI_SUPPORT_OCT
        if (host != SPI2_HOST) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI octal mode is only supported using host 1"));
            return mp_const_none;
        } else {
            self->octal_mode = true;
        }
    #else
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI octal mode is not supported on this MCU"));
        return mp_const_none;
    #endif
    } else {
        self->octal_mode = false;
    }

    if ((bool)args[ARG_dual].u_bool) {
        if (mosi != -1 && miso != -1) {
            buscfg_flags |= SPICOMMON_BUSFLAG_DUAL;
        } else {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("miso and mosi pins are needed to use dual mode"));
            return mp_const_none;
        }
    }

    self->buscfg = (spi_bus_config_t){
        .miso_io_num = (int)miso,
        .mosi_io_num = (int)mosi,
        .sclk_io_num = (int)sck,
        .quadwp_io_num = (int)wp,
        .quadhd_io_num = (int)hd,
        .data4_io_num = (int)data4,
        .data5_io_num = (int)data5,
        .data6_io_num = (int)data6,
        .data7_io_num = (int)data7,
        .max_transfer_sz = SPI_LL_DMA_MAX_BIT_LEN / 8
        .flags = buscfg_flags
    };
    self->host = (spi_host_device_t)host;

#if CONFIG_IDF_TARGET_ESP32
    int dma_chan = SPI_DMA_DISABLED;

    if (self->host == SPI2_HOST) {
        dma_chan = SPI_DMA_CH1;
    } else {
       dma_chan = SPI_DMA_CH2;
    }
#else
    int dma_chan = SPI_DMA_CH_AUTO;
#endif

    ret = spi_bus_initialize(self->host, &self->buscfg, dma_chan);
    if (ret != ESP_OK) {
        check_esp_err(ret);
        return mp_const_none;
    }

    self->state = MACHINE_HW_SPI_STATE_INIT;

    return MP_OBJ_FROM_PTR(self);
}


STATIC mp_obj_t esp32_hw_spi_bus_deinit(mp_obj_t self_in)
{
    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)self_in;

    esp_err_t ret = spi_bus_free(self->host);
    check_esp_err(ret);

    if (self->state == MACHINE_HW_SPI_STATE_INIT) {
        self->state = MACHINE_HW_SPI_STATE_DEINIT;
        machine_hw_spi_deinit_internal(self);
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(esp32_hw_spi_bus_deinit_obj, esp32_hw_spi_bus_deinit);



STATIC mp_obj_t esp32_hw_spi_bus_get_host(mp_obj_t self_in) {
    esp32_hw_spi_bus_obj_t *self = (esp32_hw_spi_bus_obj_t *)self_in;
    return mp_obj_new_int(self->host);
}

MP_DEFINE_CONST_FUN_OBJ_1(esp32_hw_spi_bus_get_host_obj, esp32_hw_spi_bus_get_host);


STATIC const mp_rom_map_elem_t esp32_hw_spi_bus_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_host),  MP_ROM_PTR(&esp32_hw_spi_bus_get_host_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),    MP_ROM_PTR(&esp32_hw_spi_bus_deinit_obj)   },
    { MP_ROM_QSTR(MP_QSTR___del__),   MP_ROM_PTR(&esp32_hw_spi_bus_deinit_obj)   }
};

MP_DEFINE_CONST_DICT(esp32_hw_spi_bus_locals_dict, esp32_hw_spi_bus_locals_dict_table);

    
MP_DEFINE_CONST_OBJ_TYPE(
    esp32_hw_spi_bus_type,
    MP_QSTR_SPI,
    MP_TYPE_FLAG_NONE,
    make_new, esp32_hw_spi_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&esp32_hw_spi_bus_locals_dict
);


STATIC mp_obj_t esp32_hw_spi_get_dma_buffer(mp_obj_t size_in)
{
    uint16_t size = (uint16_t)mp_obj_get_int_truncated(size_in);

    void *buf = heap_caps_calloc(1, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    if (buf == NULL) {
        buf = heap_caps_calloc(1, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    }

    if (buf == NULL) {
        mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Unable to allocate buffer"));
        return mp_const_none;
    }

    mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, buf));
    view->typecode |= 0x80; // used to indicate RW buffer

    return MP_OBJ_FROM_PTR(view);
}

MP_DEFINE_CONST_FUN_OBJ_1(esp32_hw_spi_get_dma_buffer_obj, esp32_hw_spi_get_dma_buffer);


STATIC mp_obj_t esp32_hw_spi_free_dma_buffer(mp_obj_t buf_in)
{
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_RW);
    heap_caps_free(bufinfo.buf);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(esp32_hw_spi_free_dma_buffer_obj, esp32_hw_spi_free_dma_buffer);


STATIC const mp_map_elem_t esp32_module_spi_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_OBJ_NEW_QSTR(MP_QSTR_spi)                  },
    { MP_ROM_QSTR(MP_QSTR_Bus)              (mp_obj_t)&esp32_hw_spi_bus_type              },
    { MP_ROM_QSTR(MP_QSTR_Device),          (mp_obj_t)&esp32_hw_spi_dev_type              },
    { MP_ROM_QSTR(MP_QSTR_get_dma_buffer),  MP_ROM_PTR(&esp32_hw_spi_get_dma_buffer_obj)  },
    { MP_ROM_QSTR(MP_QSTR_free_dma_buffer), MP_ROM_PTR(&esp32_hw_spi_free_dma_buffer_obj) },
};

STATIC MP_DEFINE_CONST_DICT(esp32_module_spi_globals, esp32_module_spi_globals_table);


const mp_obj_module_t esp32_module_spi = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&esp32_module_spi_globals,
};

MP_REGISTER_MODULE(MP_QSTR_spi, esp32_module_spi);
