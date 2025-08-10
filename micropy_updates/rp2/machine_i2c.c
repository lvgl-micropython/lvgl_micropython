// Copyright (c) 2013-2018 Damien P. George
// Copyright (c) 2024 - 2025 Kevin G. Schlosser

 #include "../../../../micropy_updates/common/mp_i2c_common.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "extmod/modmachine.h"

#include "hardware/i2c.h"

#define DEFAULT_I2C_FREQ (400000)
#define DEFAULT_I2C_TIMEOUT (50000)

#ifdef MICROPY_HW_I2C_NO_DEFAULT_PINS

// With no default I2C, need to require the pin args.
#define MICROPY_HW_I2C0_SCL (0)
#define MICROPY_HW_I2C0_SDA (0)
#define MICROPY_HW_I2C1_SCL (0)
#define MICROPY_HW_I2C1_SDA (0)
#define MICROPY_I2C_PINS_ARG_OPTS MP_ARG_REQUIRED

#else

// Most boards do not require pin args.
#define MICROPY_I2C_PINS_ARG_OPTS 0

#ifndef MICROPY_HW_I2C0_SCL
#if PICO_DEFAULT_I2C == 0
#define MICROPY_HW_I2C0_SCL (PICO_DEFAULT_I2C_SCL_PIN)
#define MICROPY_HW_I2C0_SDA (PICO_DEFAULT_I2C_SDA_PIN)
#else
#define MICROPY_HW_I2C0_SCL (9)
#define MICROPY_HW_I2C0_SDA (8)
#endif
#endif

#ifndef MICROPY_HW_I2C1_SCL
#if PICO_DEFAULT_I2C == 1
#define MICROPY_HW_I2C1_SCL (PICO_DEFAULT_I2C_SCL_PIN)
#define MICROPY_HW_I2C1_SDA (PICO_DEFAULT_I2C_SDA_PIN)
#else
#define MICROPY_HW_I2C1_SCL (7)
#define MICROPY_HW_I2C1_SDA (6)
#endif
#endif
#endif

// SDA/SCL on even/odd pins, I2C0/I2C1 on even/odd pairs of pins.
#define IS_VALID_SCL(i2c, pin) (((pin) & 1) == 1 && (((pin) & 2) >> 1) == (i2c))
#define IS_VALID_SDA(i2c, pin) (((pin) & 1) == 0 && (((pin) & 2) >> 1) == (i2c))


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
}


void mp_machine_hw_i2c_bus_initilize(machine_hw_i2c_bus_obj_t *self)
{
    i2c_inst_t *const i2c_inst = NULL;
    i2c_init(i2c_inst, 400000);

    gpio_set_function(mp_hal_get_pin_obj(self->scl), GPIO_FUNC_I2C);
    gpio_set_function(mp_hal_get_pin_obj(self->sda), GPIO_FUNC_I2C);
    gpio_set_pulls(mp_hal_get_pin_obj(self->scl), true, 0);
    gpio_set_pulls(mp_hal_get_pin_obj(self->sda), true, 0);

    self->user_data = i2c_inst;
}


mp_obj_t machine_hw_i2c_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_host, ARG_scl, ARG_sda };
    static const mp_arg_t allowed_args[] = {
    #ifdef PICO_DEFAULT_I2C
        { MP_QSTR_host, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = PICO_DEFAULT_I2C} },
    #else
        { MP_QSTR_host, MP_ARG_KW_ONLY | MP_ARG_INT | MP_ARG_REQUIRED },
    #endif
        { MP_QSTR_scl, MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_sda, MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_queue_size, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
    };

    // Parse args.
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                              MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    int i2c_id = args[ARG_host].u_int;

    // Check if the I2C bus is valid
    if (i2c_id < 0 || i2c_id >= 2) {
        mp_raise_ValueError(MP_ERROR_TEXT("I2C: host doesn't exist"));
    }

    // Get static peripheral object.
    mp_machine_hw_i2c_bus_obj_t *self = m_new_obj(mp_machine_hw_i2c_bus_obj_t);
    self->base.type = &mp_machine_hw_i2c_bus_type;

    self->port = (uint8_t)i2c_id;

    // Set SCL/SDA pins if configured.
    int scl = mp_hal_get_pin_obj(args[ARG_scl].u_obj);
    if (!IS_VALID_SCL(self->i2c_id, scl)) {
        mp_raise_ValueError(MP_ERROR_TEXT("bad SCL pin"));
    }
    self->scl = args[ARG_scl].u_obj;

    int sda = mp_hal_get_pin_obj(args[ARG_sda].u_obj);
    if (!IS_VALID_SDA(self->i2c_id, sda)) {
        mp_raise_ValueError(MP_ERROR_TEXT("bad SDA pin"));
    }
    self->sda = args[ARG_sda].u_obj;

    return MP_OBJ_FROM_PTR(self);
}

// *********************** DEVICE FUNCTIONS ***********************

mp_obj_t machine_hw_i2c_device_make_new(const mp_obj_type_t *type, size_t n_args,
                                        size_t n_kw, const mp_obj_t *all_args)
{

    enum { ARG_i2c_bus, ARG_addr, ARG_freq, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_i2c_bus, MP_ARG_KW_ONLY | MP_ARG_OBJ | MP_ARG_REQUIRED                  } },
        { MP_QSTR_addr,    MP_ARG_KW_ONLY | MP_ARG_INT | MP_ARG_REQUIRED                  } },
        { MP_QSTR_freq,    MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = DEFAULT_I2C_FREQ       } },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = DEFAULT_I2C_TIMEOUT    } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,MP_ARRAY_SIZE(allowed_args),
                              allowed_args, args);

    mp_machine_hw_i2c_device_obj_t *self = m_new_obj(mp_machine_hw_i2c_device_obj_t);
    self->base.type = &mp_machine_hw_i2c_device_type;

    self->i2c_bus = MP_OBJ_TO_PTR(args[ARG_i2c_bus].u_obj);
    self->addr = (uint8_t)args[ARG_addr].u_int;
    self->timeout = (uint16_t)args[ARG_timeout].u_int;
    self->freq = (uint32_t)args[ARG_freq].u_int;

    self->mem_addr_size = 8;

    self->deinit = &machine_hw_i2c_device_deinit_callback;

    if (self->i2c_bus->user_data == NULL) {
        mp_machine_hw_i2c_bus_initilize(self->i2c_bus)
    }

    mp_machine_hw_i2c_bus_add_device(self);
    return MP_OBJ_FROM_PTR(self);
}


static int machine_i2c_transfer_single(mp_obj_base_t *self_in, uint16_t addr, size_t len, uint8_t *buf, unsigned int flags)
{

    mp_machine_hw_i2c_bus_obj_t *self;
    mp_machine_hw_i2c_device_obj_t *dev_self;
    uint32_t freq = DEFAULT_I2C_FREQ;
    uint32_t timeout = DEFAULT_I2C_TIMEOUT;

    if (self_in->type == &mp_machine_hw_i2c_device_type) {
        dev_self = (mp_machine_hw_i2c_device_obj_t *)self_in;
        self = dev_self->i2c_bus;
        freq = dev_self->freq;
        timeout = dev_self->timeout;
    } else {
        self = (mp_machine_hw_i2c_bus_obj_t *)self_in;
        dev_self = NULL;
    }

    i2c_inst_t *const i2c_inst = (i2c_inst_t *const)self->user_data;

    int ret;
    bool nostop = !(flags & MP_MACHINE_I2C_FLAG_STOP);

    if (flags & MP_MACHINE_I2C_FLAG_READ) {
        i2c_set_baudrate(i2c_inst, freq);
        ret = i2c_read_timeout_us(i2c_inst, addr, buf, len, nostop, timeout);
    } else {
        if (len == 0) {
            // Workaround issue with hardware I2C not accepting zero-length writes.
            mp_machine_soft_i2c_obj_t soft_i2c = {
                .base = { &mp_machine_soft_i2c_type },
                .us_delay = 500000 / freq + 1,
                .us_timeout = timeout,
                .scl = mp_hal_get_pin_obj(self->scl),
                .sda = mp_hal_get_pin_obj(self->sda),
            };

            mp_machine_i2c_buf_t bufs = {
                .len = len,
                .buf = buf,
            };

            mp_hal_pin_open_drain(mp_hal_get_pin_obj(self->scl));
            mp_hal_pin_open_drain(mp_hal_get_pin_obj(self->sda));
            ret = mp_machine_soft_i2c_transfer(&soft_i2c.base, addr, 1, &bufs, flags);
            gpio_set_function(mp_hal_get_pin_obj(self->scl), GPIO_FUNC_I2C);
            gpio_set_function(mp_hal_get_pin_obj(self->sda), GPIO_FUNC_I2C);
            return ret;
        } else {
            i2c_set_baudrate(i2c_inst, freq);
            ret = i2c_write_timeout_us(i2c_inst, addr, buf, len, nostop, timeout);
        }
    }

    if (ret < 0) {
        if (ret == PICO_ERROR_TIMEOUT) return -MP_ETIMEDOUT;
        else return -MP_EIO;
    } else {
        return ret;
    }
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
    mp_obj_t stop = (n_args == 2) ? mp_const_true : args[2];

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


static const mp_machine_i2c_p_t machine_hw_i2c_p = {
    .transfer = mp_machine_i2c_transfer_adaptor,
    .transfer_single = machine_i2c_transfer_single,
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


