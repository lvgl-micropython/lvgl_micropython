

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../include/esp32_spi_device.h"

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "py/gc.h"
#include "py/stackctrl.h"
#include "py/objarray.h"
#include "py/binary.h"

#include "driver/spi_master.h"
#include "driver/spi_common.h"
#include "soc/spi_pins.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_cpu.h"
#include "hal/spi_ll.h"

// SPI DEVICE CLASS
//---------------------------------------------------------------------------------------------------

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


static void _esp32_pre_cb(spi_transaction_t *trans)
{
    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)trans->user;

    if (self->trans_start_cb != mp_const_none && mp_obj_is_callable(self->trans_start_cb)) {
        _esp_spi_cb_isr(self, self->trans_start_cb, self->trans_start_user_data);
    }
}

static void _esp32_post_cb(spi_transaction_t *spi_trans)
{
    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)spi_trans->user;

    spi_transaction_ext_t *spi_trans_ext = __containerof(spi_trans, spi_transaction_ext_t, base);
    esp32_spi_trans_descriptor_t *spi_trans_desc = __containerof(spi_trans_ext, esp32_spi_trans_descriptor_t, base);

    if (spi_trans_desc->trans_done) {
        if (spi_trans_desc->callback != mp_const_none && mp_obj_is_callable(spi_trans_desc->callback)) {
            _esp_spi_cb_isr(self, spi_trans_desc->callback, self->trans_end_user_data);
        }

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
        { MP_QSTR_spi_bus,        MP_ARG_OBJ  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED    },
        { MP_QSTR_freq,           MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED    },
        { MP_QSTR_cs,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int  = -1    } },
        { MP_QSTR_polarity,       MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int  =  0    } },
        { MP_QSTR_phase,          MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int  =  0    } },
        { MP_QSTR_firstbit,       MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int  = MICROPY_PY_MACHINE_SPI_MSB } },
        { MP_QSTR_bits,           MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int  =  8    } },
        { MP_QSTR_three_wire,     MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false } },
        { MP_QSTR_cs_high_active, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false } },
        { MP_QSTR_half_duplex,    MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false } },
        { MP_QSTR_clock_as_cs,    MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false } },
        { MP_QSTR_queue_size,     MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int  =  5    } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    size_t trans_pool_count = (size_t)args[ARG_queue_size].u_int;
    esp32_hw_spi_dev_obj_t *self = NULL;
    self = calloc(1, sizeof(esp32_hw_spi_dev_obj_t) + sizeof(esp32_spi_trans_descriptor_t) * trans_pool_count);

    self->base.type = &esp32_hw_spi_dev_type;

    self->spi_bus = (esp32_hw_spi_bus_obj_t *)args[ARG_spi_bus].u_obj;
    self->trans_pool_count = trans_pool_count;
    self->bits = (uint8_t)args[ARG_bits].u_int;

    uint32_t flags = args[ARG_firstbit].u_int == MICROPY_PY_MACHINE_SPI_LSB ? SPI_DEVICE_BIT_LSBFIRST : 0;
    if (args[ARG_three_wire].u_bool) flags |= SPI_DEVICE_3WIRE;
    if (args[ARG_cs_active_pos].u_bool) flags |= SPI_DEVICE_POSITIVE_CS;
    if (args[ARG_half_duplex].u_bool) flags |= SPI_DEVICE_HALFDUPLEX;
    if (args[ARG_clock_as_cs].u_bool) flags |= SPI_DEVICE_CLK_AS_CS;

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


mp_obj_t esp32_hw_spi_dev_comm(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_self, ARG_tx_data, ARG_rx_data, ARG_cmd, ARG_cmd_bits, ARG_addr, ARG_addr_bits, ARG_callback };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,       MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_tx_data,    MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj = mp_const_none } },
        { MP_QSTR_rx_data,    MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj = mp_const_none } },
        { MP_QSTR_cmd,        MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj = mp_const_none } },
        { MP_QSTR_cmd_bits,   MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj = mp_const_none } },
        { MP_QSTR_addr,       MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj = mp_const_none } },
        { MP_QSTR_addr_bits,  MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj = mp_const_none } },
        { MP_QSTR_callback,   MP_ARG_KW_ONLY | MP_ARG_OBJ,  {.u_obj = mp_const_none } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint16_t cmd;
    uint8_t cmd_bits;

    if (args[ARG_cmd].u_obj != mp_const_none && args[ARG_cmd_bits].u_obj != mp_const_none) {
        cmd = mp_obj_get_int_truncated(args[ARG_cmd].u_obj);
        cmd_bits = mp_obj_get_int_truncated(args[ARG_cmd_bits].u_obj);
    } else {
        cmd = 0;
        cmd_bits = 0;

        if (args[ARG_cmd].u_obj != mp_const_none || args[ARG_cmd_bits].u_obj != mp_const_none) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("To send a command both `cmd` and `cmd_bits` must be given."));
            return mp_const_none;
        }
    }

    if (cmd_bits > 16) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("`cmd_bits` must be in the range 1-16"));
    }

    uint64_t addr;
    uint8_t addr_bits;

    if (args[ARG_addr].u_obj != mp_const_none && args[ARG_addr_bits].u_obj != mp_const_none) {
        addr = mp_obj_get_int_truncated(args[ARG_addr].u_obj);
        addr_bits = mp_obj_get_int_truncated(args[ARG_addr_bits].u_obj);
    } else {
        addr = 0;
        addr_bits = 0;

        if (args[ARG_addr].u_obj != mp_const_none || args[ARG_addr_bits].u_obj != mp_const_none) {
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("To send a parameter both `addr` and `addr_bits` must be given."));
            return mp_const_none;
        }
    }

    if (addr_bits > 64) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("`addr_bits` must be in the range 1-64"));
    }

    esp32_hw_spi_dev_obj_t *self = (esp32_hw_spi_dev_obj_t *)args[ARG_self].u_obj;

    size_t tx_size = 0;
    uint8_t *tx_buf = NULL;

    if (args[ARG_tx_data].u_obj != mp_const_none) {
        mp_buffer_info_t tx_bufinfo;
        mp_get_buffer_raise(args[ARG_tx_data].u_obj, &tx_bufinfo, MP_BUFFER_READ);
        tx_size = tx_bufinfo.len * 8 / self->bits * self->bits / 8;
        tx_buf = (uint8_t *)tx_bufinfo.buf;
    }

    size_t rx_size = 0;
    uint8_t *rx_buf = NULL;

    if (args[ARG_rx_data].u_obj != mp_const_none) {
        mp_buffer_info_t rx_bufinfo;
        mp_get_buffer_raise(args[ARG_rx_data].u_obj, &rx_bufinfo, MP_BUFFER_WRITE);
        rx_size = rx_bufinfo.len * 8 / self->bits * self->bits / 8;
        rx_buf = (uint8_t *)rx_bufinfo.buf;
    }

    if (rx_size != 0 && tx_size != 0 && rx_size > tx_size) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("`rx_data` cannot be larger than `tx_data`"));
    }

    esp_err_t ret = ESP_OK;
    spi_transaction_t *spi_trans = NULL;
    spi_transaction_ext_t *spi_trans_ext = NULL;
    esp32_spi_trans_descriptor_t *spi_trans_desc = NULL;

    ret = spi_device_acquire_bus(self->spi_dev, portMAX_DELAY);
    check_esp_err(ret);


    // to fill the rx buffer we are using a DMA transfer if the supplied buffer was allocated in DMA memory
    // This is looking code in the even the buffer is larger than a single transaction is able to handle.
    size_t tx_chunk_size;
    size_t rx_chunk_size;

    do {
        tx_chunk_size = tx_size;
        rx_chunk_size = rx_size;

        if (self->num_trans_inflight < self->trans_pool_count) {
            // get the next available transaction
            spi_trans_desc = &self->trans_pool[self->num_trans_inflight];
        } else {
            // transaction pool has used up, recycle one transaction
            MP_THREAD_GIL_EXIT();
            ret = spi_device_get_trans_result(self->spi_dev, &spi_trans, portMAX_DELAY);
            MP_THREAD_GIL_ENTER();
            check_esp_err(ret);

            spi_trans_ext = __containerof(spi_trans, spi_transaction_ext_t, base);
            spi_trans_desc = __containerof(spi_trans_ext, esp32_spi_trans_descriptor_t, base);
            self->num_trans_inflight--;
        }

        memset(spi_trans_desc, 0, sizeof(esp32_spi_trans_descriptor_t));

        spi_trans_desc->base.base.flags = SPI_TRANS_VARIABLE_CMD | SPI_TRANS_VARIABLE_ADDR;

        if (tx_chunk_size > self->max_transfer_sz) {
            // cap the transfer size to the maximum supported by the bus
            tx_chunk_size = self->max_transfer_sz;
            spi_trans_desc->base.base.flags |= SPI_TRANS_CS_KEEP_ACTIVE;
        } else {
            // mark trans_done only at the last round to avoid premature completion callback
            spi_trans_desc->trans_done = true;
            spi_trans_desc->base.base.flags &= ~SPI_TRANS_CS_KEEP_ACTIVE;
        }
        if (rx_chunk_size > self->max_transfer_sz) {
            rx_chunk_size = self->max_transfer_sz;
        }

        spi_trans_desc->base.base.user = self;
        spi_trans_desc->callback = args[ARG_callback].u_obj;

        if (tx_chunk_size) {
            spi_trans_desc->base.base.flags |= SPI_TRANS_USE_TXDATA;
            spi_trans_desc->base.base.length = tx_chunk_size * 8; // transaction length is in bits
            spi_trans_desc->base.base.tx_buffer = tx_buf;
        }
        if (rx_chunk_size) {
            spi_trans_desc->base.base.flags |= SPI_TRANS_USE_RXDATA;
            spi_trans_desc->base.base.rxlength = rx_chunk_size * 8;
            spi_trans_desc->base.base.rx_buffer = rx_buf;
        }
        if (self->spi_bus->octal_mode) {
            // use 8 lines for transmitting command, address and data
            spi_trans_desc->base.base.flags |= (SPI_TRANS_MULTILINE_CMD | SPI_TRANS_MULTILINE_ADDR | SPI_TRANS_MODE_OCT);
        }

        if (cmd_bits) {
            spi_trans_desc->base.command_bits = cmd_bits;
            spi_trans_desc->base.base.cmd = cmd;
            cmd_bits = 0;
        }

        if (addr_bits) {
            spi_trans_desc->base.address_bits = addr_bits;
            spi_trans_desc->base.base.addr = addr;
            addr_bits = 0;
        }

        spi_trans_desc->callback = args[ARG_callback].u_obj;

        // data is usually large, using queue+blocking mode
        ret = spi_device_queue_trans(self->spi_dev, &spi_trans_desc->base.base, portMAX_DELAY);
        check_esp_err(ret);
        self->num_trans_inflight++;

        if (tx_chunk_size) {
            tx_buf = (uint8_t *)tx_buf + tx_chunk_size;
            tx_size -= tx_chunk_size;
        }
        // move on to the next chunk
        if (rx_chunk_size) {
            rx_buf = (uint8_t *)rx_buf + rx_chunk_size;
            rx_size -= rx_chunk_size;
        }

    } while (tx_size > 0 || cmd_bits || addr_bits); // continue while we have remaining data to transmit

    spi_device_release_bus(self->spi_dev);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(esp32_hw_spi_dev_comm_obj, 1, esp32_hw_spi_dev_comm);


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
    { MP_ROM_QSTR(MP_QSTR_comm),                    MP_ROM_PTR(&esp32_hw_spi_dev_comm_obj)           },
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

//---------------------------------------------------------------------------------------------------