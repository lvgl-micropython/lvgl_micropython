// Copyright (c) 2013-2018 Damien P. George
// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../../../../micropy_updates/common/mp_i2c_common.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "extmod/modmachine.h"

#include "driver/i2c_master.h"

#if MICROPY_PY_MACHINE_I2C || MICROPY_PY_MACHINE_SOFTI2C

#ifndef MICROPY_HW_I2C0_SCL
#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S3
#define MICROPY_HW_I2C0_SCL (GPIO_NUM_9)
#define MICROPY_HW_I2C0_SDA (GPIO_NUM_8)
#else
#define MICROPY_HW_I2C0_SCL (GPIO_NUM_18)
#define MICROPY_HW_I2C0_SDA (GPIO_NUM_19)
#endif
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

#if SOC_I2C_SUPPORT_XTAL
#define I2C_SCLK_FREQ XTAL_CLK_FREQ
#elif SOC_I2C_SUPPORT_APB
#define I2C_SCLK_FREQ APB_CLK_FREQ
#else
#error "unsupported I2C for ESP32 SoC variant"
#endif

#define I2C_DEFAULT_TIMEOUT_US (50000) // 50ms


static machine_hw_i2c_obj_t machine_hw_i2c_bus_obj[I2C_NUM_MAX];


static void * _get_func(size_t index)
{
    mp_rom_map_elem_t *elem = &(((mp_rom_map_elem_t *)(mp_machine_i2c_locals_dict.map.table))[index]);

    if (elem->value.u32.lo == NULL) return elem->value.u32.hi;
    else return elem->value.u32.lo;
}


typedef mp_obj_t (*machine_i2c_scan_func_t)(mp_obj_t self_in);
static mp_obj_fun_builtin_fixed_t *machine_i2c_scan_func_obj = _get_func(1);
static machine_i2c_scan_func_t machine_i2c_scan = machine_i2c_scan_func_obj->fun._1;

MP_DEFINE_CONST_FUN_OBJ_1(new_machine_i2c_scan_obj, machine_i2c_scan);


typedef mp_obj_t (*machine_i2c_readinto_func_t)(size_t n_args, const mp_obj_t *args);
static mp_obj_fun_builtin_var_t *machine_i2c_readinto_func_obj = _get_func(4);
static machine_i2c_readinto_func_t machine_i2c_readinto = machine_i2c_readinto_func_obj->fun.var;

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(new_machine_i2c_readinto_obj, 2, 3, machine_i2c_readinto);


typedef mp_obj_t (*machine_i2c_write_func_t)(mp_obj_t self_in, mp_obj_t buf_in);
static mp_obj_fun_builtin_fixed_t *machine_i2c_write_func_obj = _get_func(5);
static machine_i2c_write_func_t machine_i2c_write = machine_i2c_write_func_obj->fun._2;

MP_DEFINE_CONST_FUN_OBJ_2(new_machine_i2c_write_obj, machine_i2c_write);


typedef mp_obj_t (*machine_i2c_readfrom_func_t)(size_t n_args, const mp_obj_t *args)
static mp_obj_fun_builtin_var_t *machine_i2c_readfrom_func_obj = _get_func(6);
static machine_i2c_readfrom_func_t machine_i2c_readfrom = machine_i2c_readfrom_func_obj->fun.var;

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(new_machine_i2c_readfrom_obj, 3, 4, machine_i2c_readfrom);


typedef mp_obj_t (*machine_i2c_readfrom_into_func_t)(size_t n_args, const mp_obj_t *args);
static mp_obj_fun_builtin_var_t *machine_i2c_readfrom_into_func_obj = _get_func(7);
static machine_i2c_readfrom_into_func_t machine_i2c_readfrom_into = machine_i2c_readfrom_into_func_obj->fun.var;

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(new_machine_i2c_readfrom_into_obj, 3, 4, machine_i2c_readfrom_into);


typedef mp_obj_t (*machine_i2c_writeto_func_t)(size_t n_args, const mp_obj_t *args);
static mp_obj_fun_builtin_var_t *machine_i2c_writeto_func_obj = _get_func(8);
static machine_i2c_writeto_func_t machine_i2c_writeto = machine_i2c_writeto_func_obj->fun.var;

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(new_machine_i2c_writeto_obj, 3, 4, machine_i2c_writeto)


typedef mp_obj_t (*machine_i2c_writevto_func_t)(size_t n_args, const mp_obj_t *args);
static mp_obj_fun_builtin_var_t *machine_i2c_writevto_func_obj = _get_func(9);
static machine_i2c_writevto_func_t machine_i2c_writevto = machine_i2c_writevto_func_obj->fun.var;

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(new_machine_i2c_writevto_obj, 3, 4, machine_i2c_writevto);


typedef mp_obj_t (*machine_i2c_readfrom_mem_func_t)(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
static mp_obj_fun_builtin_var_t *machine_i2c_readfrom_mem_func_obj = _get_func(10);
static machine_i2c_readfrom_mem_func_t machine_i2c_readfrom_mem = machine_i2c_readfrom_mem_func_obj->fun.kw;

MP_DEFINE_CONST_FUN_OBJ_KW(new_machine_i2c_readfrom_mem_obj, 1, machine_i2c_readfrom_mem);


typedef mp_obj_t (*machine_i2c_readfrom_mem_into_func_t)(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
static mp_obj_fun_builtin_var_t *machine_i2c_readfrom_mem_into_func_obj = _get_func(11);
static machine_i2c_readfrom_mem_into_func_t machine_i2c_readfrom_mem_into = machine_i2c_readfrom_mem_into_func_obj->fun.kw;

MP_DEFINE_CONST_FUN_OBJ_KW(new_machine_i2c_readfrom_mem_into_obj, 1, machine_i2c_readfrom_mem_into);


typedef mp_obj_t (*machine_i2c_writeto_mem_func_t)(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
static mp_obj_fun_builtin_var_t *machine_i2c_writeto_mem_func_obj = _get_func(12);
static machine_i2c_writeto_mem_func_t machine_i2c_writeto_mem = machine_i2c_writeto_mem_func_obj->fun.kw;

static MP_DEFINE_CONST_FUN_OBJ_KW(new_machine_i2c_writeto_mem_obj, 1, machine_i2c_writeto_mem);



void mp_machine_hw_i2c_bus_add_device(mp_machine_hw_i2c_device_obj_t *device)
{
    device->i2c_bus->device_count++;
    device->i2c_bus->devices = m_realloc(device->i2c_bus->devices,
            device->i2c_bus->device_count * sizeof(mp_machine_hw_i2c_device_obj_t *));
}


void mp_machine_hw_i2c_bus_remove_device(mp_machine_hw_i2c_device_obj_t *device)
{
    uint8_t i;
    /*Find the device in the list*/
    for(i = 0; i < device->i2c_bus->device_count; i++) {
        if(device->i2c_bus->devices[i] == device) break;
    }

    for( ;i < device->i2c_bus->device_count - 1; i++) {
        device->i2c_bus->devices[i] = device->i2c_bus->devices[i + 1];
    }

    device->i2c_bus->device_count--;
    device->i2c_bus->devices = m_realloc(device->i2c_bus->devices,
            device->i2c_bus->device_count * sizeof(mp_machine_hw_i2c_device_obj_t *));

    if (device->i2c_bus->device_count == 0) {
        device->i2c_bus->deinit(device->i2c_bus);
    }
}


static void machine_hw_i2c_device_deinit_callback(mp_machine_hw_i2c_device_obj_t *self)
{
    if (self->user_data == NULL) return;

    i2c_master_bus_rm_device((i2c_master_dev_handle_t)self->user_data);
    self->user_data = NULL;
}


static void machine_hw_i2c_device_deinit_internal(mp_machine_hw_i2c_device_obj_t *self)
{
    if (self->user_data == NULL) return;

    i2c_master_bus_rm_device((i2c_master_dev_handle_t)self->user_data);
    self->user_data = NULL;
    mp_machine_hw_i2c_bus_remove_device(self);
}

static void disable_gpio(mp_obj_t gpio_num) {

    int g = (int)mp_obj_get_int(gpio_num);
    esp_rom_gpio_pad_select_gpio(g);
    esp_rom_gpio_connect_out_signal(g, SIG_GPIO_OUT_IDX, false, false);
    gpio_set_direction(g, GPIO_MODE_INPUT);
    gpio_reset_pin(g);
}


void machine_hw_i2c_bus_deinit_internal(mp_machine_hw_i2c_bus_obj_t *self)
{
    if (self->user_data == NULL) return;

    for (uint8_t i = 0; i < self->device_count; i++) {
        self->devices[i]->deinit(self->devices[i]);
    }

    self->device_count = 0;
    self->devices = m_realloc(self->devices, self->device_count * sizeof(mp_machine_hw_i2c_device_obj_t *));

    i2c_del_master_bus((i2c_master_bus_handle_t)self->user_data);
    self->user_data = NULL;

    disable_gpio(self->scl);
    disable_gpio(self->sda);
}


void mp_machine_hw_i2c_bus_initilize(machine_hw_i2c_bus_obj_t *self)
{
    mp_machine_hw_i2c_device_obj_t *device = NULL;

    if (self->user_data != NULL) {
        for (uint8_t i=0; i<self->device_count; i++) {
            device = self->devices[i];
            machine_hw_i2c_device_deinit_callback(device);
        }

        i2c_del_master_bus((i2c_master_bus_handle_t)self->user_data);
        self->user_data = NULL;
    }

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port=self->port,
        .scl_io_num=machine_pin_get_id(self->scl),
        .sda_io_num=machine_pin_get_id(self->sda),
        .clk_source=I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt=7,
        .flags.enable_internal_pullup=true,
        .trans_queue_depth=self->trans_queue_depth
    };

    i2c_master_bus_handle_t bus_handle = NULL;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));
    self->user_data = bus_handle;

    for (uint8_t i=0; i<self->device_count; i++) {
        device = self->devices[i];
        i2c_device_config_t dev_cfg = {
            .dev_addr_length=I2C_ADDR_BIT_LEN_7,
            .device_address=device->addr,
            .scl_speed_hz=device->freq,
        };

        i2c_master_dev_handle_t dev_handle = NULL;

        ESP_ERROR_CHECK(i2c_master_bus_add_device((i2c_master_bus_handle_t)self->user_data, &dev_cfg, &dev_handle));
        device->user_data = dev_handle;
    }
}


int machine_hw_i2c_transfer(mp_obj_base_t *self_in, uint16_t addr, size_t n,
                            mp_machine_i2c_buf_t *bufs, unsigned int flags)
{
    esp_err_t err = ESP_OK;
    int data_len = 0;

    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    uint16_t dst_addr;
    uint8_t trans_queue_depth;
    uint16_t timeout;

    if (self_in->type == &mp_machine_hw_i2c_device_type) {
        // device type, use address as reg location
        machine_hw_i2c_device_obj_t *self_d = (machine_hw_i2c_device_obj_t *)self_in;

        bus_handle = (i2c_master_bus_handle_t)self_d->i2c_bus->user_data;
        dst_addr = self_d->addr;
        timeout = self_d->timeout;
        trans_queue_depth = self_d->i2c_bus->trans_queue_depth;
        dev_handle = (i2c_master_dev_handle_t)self_d->user_data;
    } else {
        machine_hw_i2c_bus_obj_t *self_b = (machine_hw_i2c_bus_obj_t *)self_in;

        bus_handle = (i2c_master_bus_handle_t)self_b->user_data;
        dst_addr = addr;
        timeout = 1000;
        trans_queue_depth = self_b->trans_queue_depth;
    }

    if (!trans_queue_depth) {
        err = i2c_master_probe(bus_handle, dst_addr, timeout);

        if (err != ESP_OK) return -MP_ENODEV;
    }

    if (self_in->type == &mp_machine_hw_i2c_bus_type) {
        i2c_device_config_t dev_cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address  = dst_addr,
            .scl_speed_hz    = 400000,
        };

        err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);

        if (err != ESP_OK) {
            return -MP_ENODEV;
        }
    }

    if (flags & MP_MACHINE_I2C_FLAG_WRITE1) {
        if (bufs->len) {
            err = i2c_master_transmit(dev_handle, bufs->buf, bufs->len, timeout);
        }

        data_len += bufs->len;
        --n;
        ++bufs;
    }

    if (err == ESP_OK) {
        for (; n--; ++bufs) {
            if (bufs->len == 0) continue;

            if (flags & MP_MACHINE_I2C_FLAG_READ) {
                err = i2c_master_receive(dev_handle, bufs->buf, bufs->len, timeout);
            } else {
               err = i2c_master_transmit(dev_handle, bufs->buf, bufs->len, timeout);
            }

            if (err != ESP_OK) break;

            data_len += bufs->len;
        }
    }

    if (self_in->type == &mp_machine_hw_i2c_bus_type) i2c_master_bus_rm_device(dev_handle);

    if (err == ESP_FAIL) return -MP_ENODEV;
    if (err == ESP_ERR_TIMEOUT) return -MP_ETIMEDOUT;
    if (err != ESP_OK) return -abs(err);

    return data_len;
}


// *********************** DEVICE FUNCTIONS ***********************

mp_obj_t machine_hw_i2c_device_make_new(const mp_obj_type_t *type, size_t n_args,
                                        size_t n_kw, const mp_obj_t *all_args)
{

    enum { ARG_i2c_bus, ARG_addr, ARG_freq, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_i2c_bus, MP_ARG_KW_ONLY | MP_ARG_OBJ | MP_ARG_REQUIRED                  } },
        { MP_QSTR_addr,    MP_ARG_KW_ONLY | MP_ARG_INT | MP_ARG_REQUIRED                  } },
        { MP_QSTR_freq,    MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 400000                 } },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = I2C_DEFAULT_TIMEOUT_US } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,MP_ARRAY_SIZE(allowed_args),
                              allowed_args, args);

    mp_machine_hw_i2c_device_obj_t *self = m_new_obj(mp_machine_hw_i2c_device_obj_t);
    self->base.type = &mp_machine_hw_i2c_device_type;

    mp_machine_hw_i2c_bus_obj_t *i2c_bus = MP_OBJ_TO_PTR(args[ARG_i2c_bus].u_obj);

    self->addr = (uint8_t)args[ARG_addr].u_int;
    self->timeout = (uint16_t)args[ARG_timeout].u_int;
    self->freq = (uint32_t)args[ARG_freq].u_int;

    self->mem_addr_size = 8;

    self->deinit = &machine_hw_i2c_device_deinit_callback;
    mp_machine_hw_i2c_bus_add_device(self);

    return MP_OBJ_FROM_PTR(self);
}


static mp_obj_t machine_i2c_device_set_mem_addr_size(mp_obj_t self_in, mp_obj_t size)
{
    mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    self->mem_addr_size = (uint8_t)mp_obj_get_int_truncated(size);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(machine_i2c_device_set_mem_addr_size_obj, machine_i2c_device_set_mem_addr_size);


static mp_obj_t machine_i2c_device_readfrom(size_t n_args, const mp_obj_t *args)
{
    mp_obj_base_t *base = MP_OBJ_TO_PTR(args[0]);
    mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    mp_obj_t addr = mp_obj_new_int_from_uint(self->addr);
    mp_obj_t stop = (n_args == 3) ? mp_const_true : args[2];

    const mp_obj_t new_args[] = { args[0], addr, args[1], stop };
    return machine_i2c_readfrom(4, new_args);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_device_readfrom_obj, 2, 3, machine_i2c_device_readfrom);


static mp_obj_t machine_i2c_device_readfrom_into(size_t n_args, const mp_obj_t *args)
{

    mp_obj_base_t *base = MP_OBJ_TO_PTR(args[0]);
    mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);

    mp_obj_t addr = mp_obj_new_int_from_uint(self->addr);
    mp_obj_t stop = (n_args == 2) ? mp_const_true : args[2];

    const mp_obj_t new_args[] = { args[0], addr, args[1], stop };
    return machine_i2c_readfrom_into(4, new_args);
}

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_device_readfrom_into_obj, 2, 3, machine_i2c_device_readfrom_into);


static mp_obj_t machine_i2c_device_writeto(size_t n_args, const mp_obj_t *args)
{
    mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t addr = mp_obj_new_int_from_uint(self->addr);
    mp_obj_t stop = (n_args == 2) ? mp_const_true : args[2];

    const mp_obj_t new_args[] = { args[0], addr, args[1], stop };

    return machine_i2c_writeto(4, new_args);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_device_writeto_obj, 2, 3, machine_i2c_device_writeto);


static mp_obj_t machine_i2c_device_write(mp_obj_t self_in, mp_obj_t buf_in) {
    mp_obj_t addr = mp_obj_new_int_from_uint(self->addr);

    const mp_obj_t new_args[] = { self_in, addr, buf_in };

    return machine_i2c_device_writeto(3, new_args);
}

MP_DEFINE_CONST_FUN_OBJ_2(machine_i2c_device_write_obj, machine_i2c_device_write);


mp_obj_t mp_machine_i2c_device_writevto(size_t n_args, const mp_obj_t *args)
{
    mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t addr = mp_obj_new_int_from_uint(self->addr);
    mp_obj_t stop = (n_args == 2) ? mp_const_true : args[2];

    const mp_obj_t new_args[] = { args[0], addr, args[1], stop };

    return machine_i2c_writevto(4, new_args);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_device_writevto_obj, 2, 3, mp_machine_i2c_device_writevto);


static mp_obj_t machine_i2c_device_readfrom_mem(mp_obj_t self_in, mp_obj_t memaddr_in, mp_obj_t n_in)
{
    mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t addr = mp_obj_new_int_from_uint(self->addr);
    mp_obj_t mem_addr_size = mp_obj_new_int_from_uint(self->mem_addr_size);

    const mp_obj_t new_args[] = { self_in, addr, memaddr_in, n_in, mem_addr_size };

    return machine_i2c_readfrom_mem(5, new_args, NULL);
}

MP_DEFINE_CONST_FUN_OBJ_3(machine_i2c_device_readfrom_mem_obj, machine_i2c_device_readfrom_mem);


static mp_obj_t machine_i2c_device_readfrom_mem_into(mp_obj_t self_in, mp_obj_t memaddr_in, mp_obj_t buf_in)
{
    mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t addr = mp_obj_new_int_from_uint(self->addr);
    mp_obj_t mem_addr_size = mp_obj_new_int_from_uint(self->mem_addr_size);

    const mp_obj_t new_args[] = { self_in, addr, memaddr_in, buf_in, mem_addr_size };

    return machine_i2c_readfrom_mem_into(5, new_args, NULL);
}

MP_DEFINE_CONST_FUN_OBJ_3(machine_i2c_device_readfrom_mem_into_obj, machine_i2c_device_readfrom_mem_into);


static mp_obj_t machine_i2c_device_writeto_mem(mp_obj_t self_in, mp_obj_t memaddr_in, mp_obj_t buf_in)
{
    mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t addr = mp_obj_new_int_from_uint(self->addr);
    mp_obj_t mem_addr_size = mp_obj_new_int_from_uint(self->mem_addr_size);

    const mp_obj_t new_args[] = { self_in, addr, memaddr_in, buf_in, mem_addr_size };

    return machine_i2c_writeto_mem(5, new_args, NULL);
}

MP_DEFINE_CONST_FUN_OBJ_3(machine_i2c_device_writeto_mem_obj, machine_i2c_device_writeto_mem);


static mp_obj_t machine_hw_i2c_device_deinit(mp_obj_t self_in)
{
    mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_hw_i2c_device_deinit_internal(self);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(machine_hw_i2c_device_deinit_obj, machine_hw_i2c_device_deinit);


static mp_obj_t machine_hw_i2c_device_get_bus(mp_obj_t self_in)
{
    mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_FROM_PTR(self->i2c_bus);
}

MP_DEFINE_CONST_FUN_OBJ_1(machine_hw_i2c_device_get_bus_obj, machine_hw_i2c_device_get_bus);


static const mp_rom_map_elem_t machine_i2c_device_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_set_mem_addr_size), MP_ROM_PTR(&machine_i2c_device_set_mem_addr_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto),          MP_ROM_PTR(&machine_i2c_device_readfrom_into_obj)     },
    { MP_ROM_QSTR(MP_QSTR_write),             MP_ROM_PTR(&machine_i2c_device_write_obj)             },
    { MP_ROM_QSTR(MP_QSTR_readfrom),          MP_ROM_PTR(&machine_i2c_device_readfrom_obj)          },
    { MP_ROM_QSTR(MP_QSTR_readfrom_into),     MP_ROM_PTR(&machine_i2c_device_readfrom_into_obj)     },
    { MP_ROM_QSTR(MP_QSTR_writeto),           MP_ROM_PTR(&machine_i2c_device_writeto_obj)           },
    { MP_ROM_QSTR(MP_QSTR_writevto),          MP_ROM_PTR(&machine_i2c_device_writevto_obj)          },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem),      MP_ROM_PTR(&machine_i2c_device_readfrom_mem_obj)      },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem_into), MP_ROM_PTR(&machine_i2c_device_readfrom_mem_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto_mem),       MP_ROM_PTR(&machine_i2c_device_writeto_mem_obj)       },
    { MP_ROM_QSTR(MP_QSTR_get_bus),           MP_ROM_PTR(&machine_hw_i2c_device_get_bus_obj)        },
    { MP_ROM_QSTR(MP_QSTR___del__),           MP_ROM_PTR(&machine_hw_i2c_device_deinit_obj)         }
};

MP_DEFINE_CONST_DICT(mp_machine_i2c_device_locals_dict, machine_i2c_device_locals_dict_table);
// ****************************************************************


// ************************* BUS FUNCTIONS ************************
mp_obj_t machine_hw_i2c_bus_make_new(const mp_obj_type_t *type, size_t n_args,
                                 size_t n_kw, const mp_obj_t *all_args)
{

    enum { ARG_host, ARG_scl, ARG_sda, ARG_queue_size };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_host,       MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = I2C_NUM_0              } },
        { MP_QSTR_scl,        MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL            } },
        { MP_QSTR_sda,        MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL            } },
        { MP_QSTR_queue_size, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL            } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,MP_ARRAY_SIZE(allowed_args),
                              allowed_args, args);

    mp_int_t i2c_id = args[ARG_host].u_int;

    if (!(I2C_NUM_0 <= i2c_id && i2c_id < I2C_NUM_MAX)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%d) doesn't exist"), i2c_id);
    }

    machine_hw_i2c_bus_obj_t *self = &machine_hw_i2c_bus_obj[i2c_id];

    if (self->base.type == NULL) {
        self->base.type = &mp_machine_hw_i2c_bus_type;
        self->port = i2c_id;
        self->trans_queue_depth = 0;
        self->deinit = &machine_hw_i2c_bus_deinit_internal;

        if (i2c_id == I2C_NUM_0) {
            self->scl = mp_obj_new_int(MICROPY_HW_I2C0_SCL);
            self->sda = mp_obj_new_int(MICROPY_HW_I2C0_SDA);
        } else {
            self->scl = mp_obj_new_int(MICROPY_HW_I2C1_SCL);
            self->sda = mp_obj_new_int(MICROPY_HW_I2C1_SDA);
        }
    }

    if (args[ARG_scl].u_obj != MP_OBJ_NULL) self->scl = args[ARG_scl].u_obj;
    if (args[ARG_sda].u_obj != MP_OBJ_NULL) self->sda = args[ARG_sda].u_obj;
    if (args[ARG_queue_size].u_obj != MP_OBJ_NULL) {
        self->trans_queue_depth = mp_obj_get_int_truncated(args[ARG_queue_size].u_obj);
    }

    mp_machine_hw_i2c_bus_initilize(self);

    return MP_OBJ_FROM_PTR(self);
}


static mp_obj_t machine_hw_i2c_bus_deinit(mp_obj_t self_in)
{
    mp_machine_hw_i2c_bus_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_hw_i2c_bus_deinit_internal(self);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(machine_hw_i2c_bus_deinit_obj, machine_hw_i2c_bus_deinit);


static const mp_rom_map_elem_t machine_i2c_bus_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_scan),              MP_ROM_PTR(&new_machine_i2c_scan_obj)              },
    { MP_ROM_QSTR(MP_QSTR_readinto),          MP_ROM_PTR(&new_machine_i2c_readinto_obj)          },
    { MP_ROM_QSTR(MP_QSTR_write),             MP_ROM_PTR(&new_machine_i2c_write_obj)             },
    { MP_ROM_QSTR(MP_QSTR_readfrom),          MP_ROM_PTR(&new_machine_i2c_readfrom_obj)          },
    { MP_ROM_QSTR(MP_QSTR_readfrom_into),     MP_ROM_PTR(&new_machine_i2c_readfrom_into_obj)     },
    { MP_ROM_QSTR(MP_QSTR_writeto),           MP_ROM_PTR(&new_machine_i2c_writeto_obj)           },
    { MP_ROM_QSTR(MP_QSTR_writevto),          MP_ROM_PTR(&new_machine_i2c_writevto_obj)          },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem),      MP_ROM_PTR(&new_machine_i2c_readfrom_mem_obj)      },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem_into), MP_ROM_PTR(&new_machine_i2c_readfrom_mem_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto_mem),       MP_ROM_PTR(&new_machine_i2c_writeto_mem_obj)       },
    { MP_ROM_QSTR(MP_QSTR___del__),           MP_ROM_PTR(&machine_hw_i2c_bus_deinit_obj)         }
};

MP_DEFINE_CONST_DICT(machine_i2c_bus_locals_dict, machine_i2c_bus_locals_dict_table);
// ****************************************************************


static const mp_machine_i2c_p_t machine_hw_i2c_p = {
    .transfer_supports_write1 = true,
    .transfer = machine_hw_i2c_transfer
};

MP_DEFINE_CONST_OBJ_TYPE(
    mp_machine_hw_i2c_device_type,
    MP_QSTR_Device,
    MP_TYPE_FLAG_NONE,
    make_new, machine_hw_i2c_device_make_new,
    protocol, &machine_hw_i2c_p,
    locals_dict, &mp_machine_i2c_device_locals_dict
);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_machine_hw_i2c_bus_type,
    MP_QSTR_Bus,
    MP_TYPE_FLAG_NONE,
    make_new, machine_hw_i2c_bus_make_new,
    protocol, &machine_hw_i2c_p,
    locals_dict, &machine_i2c_bus_locals_dict
);


static const mp_rom_map_elem_t machine_i2c_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),  MP_OBJ_NEW_QSTR(MP_QSTR_SPI)   },
    { MP_ROM_QSTR(MP_QSTR_Bus),       (mp_obj_t)&mp_machine_hw_i2c_bus_type },
    { MP_ROM_QSTR(MP_QSTR_Device),    (mp_obj_t)&mp_machine_hw_i2c_device_type }
};

MP_DEFINE_CONST_DICT(machine_i2c_class_locals_dict, machine_i2c_class_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    machine_i2c_type,
    MP_QSTR_I2C,
    MP_TYPE_FLAG_NONE,
    locals_dict, &machine_i2c_class_locals_dict
);

#endif // MICROPY_PY_MACHINE_I2C || MICROPY_PY_MACHINE_SOFTI2C