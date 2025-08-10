// Copyright (c) 2013-2018 Damien P. George
// Copyright (c) 2024 - 2025 Kevin G. Schlosser

 #include "../../../../micropy_updates/common/mp_i2c_common.h"

#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "extmod/modmachine.h"
#include "i2c.h"

#if MICROPY_HW_ENABLE_HW_I2C || MICROPY_PY_MACHINE_I2C

#if defined(STM32F0) || defined(STM32F7) || defined(STM32H7) || defined(STM32L4)
#define MACHINE_I2C_TIMINGR (1)
#else
#define MACHINE_I2C_TIMINGR (0)
#endif

#define I2C_POLL_DEFAULT_TIMEOUT_US (50000) // 50ms


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


#if defined(STM32F0) || defined(STM32F4) || defined(STM32F7) || defined(STM32H7) || defined(STM32L1) || defined(STM32L4)
void machine_hard_i2c_init(mp_machine_hw_i2c_device_obj_t *self)
{
    uint32_t timeout_ms = (self->timeout + 999) / 1000;
    i2c_init((i2c_t *)self->i2c_bus->user_data, self->scl, self->sda, self->freq, timeout_ms);
}
#endif


static void machine_soft_i2c_init(mp_machine_soft_i2c_obj_t *self,
                                  uint32_t freq, uint32_t timeout)
{
    // set parameters
    if (freq >= 1000000) {
        // allow fastest possible bit-bang rate
        self->us_delay = 0;
    } else {
        self->us_delay = 500000 / freq;
        if (self->us_delay == 0) {
            self->us_delay = 1;
        }
    }

    self->us_timeout = timeout;

    // init pins
    mp_hal_pin_open_drain(self->scl);
    mp_hal_pin_open_drain(self->sda);
}



mp_obj_t machine_hw_i2c_bus_make_new(const mp_obj_type_t *type, size_t n_args,
                                     size_t n_kw, const mp_obj_t *all_args)
{

    // parse args
    enum { ARG_host, ARG_scl, ARG_sda, ARG_queue_size, ARG_timingr };
    static const mp_arg_t allowed_args[] = {
    #if defined(STM32F0) || defined(STM32F4) || defined(STM32F7) || defined(STM32H7) || defined(STM32L1) || defined(STM32L4)
        { MP_QSTR_host,       MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_OBJ         },
        { MP_QSTR_scl,        MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
        { MP_QSTR_sda,        MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
    #if MACHINE_I2C_TIMINGR
        { MP_QSTR_timingr, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    #endif
    #else
        { MP_QSTR_host,       MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT         },
        { MP_QSTR_scl,        MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_OBJ         },
        { MP_QSTR_sda,        MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_OBJ         },
    #endif
        { MP_QSTR_queue_size, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                              MP_ARRAY_SIZE(allowed_args), allowed_args, args);


    mp_machine_hw_i2c_bus_obj_t *self = m_new_obj(mp_machine_hw_i2c_bus_obj_t);
    self->base.type = &mp_machine_hw_i2c_bus_type;

    self->user_data = NULL;

#if defined(STM32F0) || defined(STM32F4) || defined(STM32F7) || defined(STM32H7) || defined(STM32L1) || defined(STM32L4)
    int i2c_id = i2c_find_peripheral(args[ARG_host].u_obj);
    self->port = (uint8_t)i2c_id;

    // if scl/sda pins are supplied we use soft i2c instead of hardware,
    // both pins MUST be supplied otherwise it will throw an error.
    if (args[ARG_scl].u_obj != MP_OBJ_NULL && args[ARG_sda].u_obj != MP_OBJ_NULL) {
        self->scl = args[ARG_scl].u_obj;
        self->sda = args[ARG_sda].u_obj;

        mp_machine_soft_i2c_obj_t *soft_self = m_new_obj(mp_machine_soft_i2c_obj_t);
        soft_self->base.type = &mp_machine_soft_i2c_type;

        soft_self->scl = mp_hal_get_pin_obj(self->scl)
        soft_self->sda = mp_hal_get_pin_obj(self->sda)

        self->soft_i2c = 1;
        self->user_data = soft_self;

    } else if (args[ARG_scl].u_obj != MP_OBJ_NULL || args[ARG_sda].u_obj != MP_OBJ_NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("supplying BOTH pins will use software I2C"));
    } else {

    #if defined(MICROPY_HW_I2C1_SCL)
        if (i2c_id == 1) {
            self->user_data = I2C1;
            self->scl = mp_obj_new_int_from_uint((uint32_t)MICROPY_HW_I2C1_SCL);
            self->sda = mp_obj_new_int_from_uint((uint32_t)MICROPY_HW_I2C1_SDA)
        }
    #endif

    #if defined(MICROPY_HW_I2C2_SCL)
        if (i2c_id == 2) {
            self->user_data = I2C2;
            self->scl = mp_obj_new_int_from_uint((uint32_t)MICROPY_HW_I2C2_SCL);
            self->sda = mp_obj_new_int_from_uint((uint32_t)MICROPY_HW_I2C2_SDA);
        }
    #endif

    #if defined(MICROPY_HW_I2C3_SCL)
        if (i2c_id == 3) {
            self->user_data = I2C3;
            self->scl = mp_obj_new_int_from_uint((uint32_t)MICROPY_HW_I2C3_SCL);
            self->sda = mp_obj_new_int_from_uint((uint32_t)MICROPY_HW_I2C3_SDA);
        }
    #endif

    #if defined(MICROPY_HW_I2C4_SCL)
        if (i2c_id == 4) {
            self->user_data = I2C4;
            self->scl = mp_obj_new_int_from_uint((uint32_t)MICROPY_HW_I2C4_SCL);
            self->sda = mp_obj_new_int_from_uint((uint32_t)MICROPY_HW_I2C4_SDA);
        }
    #endif

        if (self->user_data == NULL) {
            mp_raise_ValueError(MP_ERROR_TEXT("unable to locate hardware I2C for the given host"));
            return mp_const_none;
        }

    #if MACHINE_I2C_TIMINGR
        // If given, explicitly set the TIMINGR value
        if (args[ARG_timingr].u_obj != mp_const_none) {
            i2c_t *handle = (i2c_t *)self->user_data;

            handle->TIMINGR = (uint32_t)mp_obj_get_int_truncated(args[ARG_timingr].u_obj);
        }
    #endif
    }

#else
    self->port = (uint8_t)args[ARG_host].u_int;
    self->scl = args[ARG_scl].u_obj;
    self->sda = args[ARG_sda].u_obj;

    mp_machine_soft_i2c_obj_t *soft_self = m_new_obj(mp_machine_soft_i2c_obj_t);
    soft_self->base.type = &mp_machine_soft_i2c_type;

    soft_self->scl = mp_hal_get_pin_obj(self->scl)
    soft_self->sda = mp_hal_get_pin_obj(self->sda)

    self->soft_i2c = 1;
    self->user_data = soft_self;
#endif

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
        { MP_QSTR_freq,    MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = 400000                 } },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = I2C_DEFAULT_TIMEOUT_US } }
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

    mp_machine_hw_i2c_bus_add_device(self);
    return MP_OBJ_FROM_PTR(self);
}


int machine_hard_i2c_transfer(mp_obj_base_t *self_in, uint16_t addr, size_t n,
                              mp_machine_i2c_buf_t *bufs, unsigned int flags)
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

    if (self->soft_i2c) {
        mp_machine_soft_i2c_obj_t *soft_self = self->user_data;

        machine_soft_i2c_init(soft_self, freq, timeout)

        return mp_machine_soft_i2c_transfer((mp_obj_base_t *)soft_self, addr, n, bufs, flags);
    } else {
#if defined(STM32F0) || defined(STM32F4) || defined(STM32F7) || defined(STM32H7) || defined(STM32L1) || defined(STM32L4)

        i2c_t *handle = (i2c_t *)self->user_data;
        uint32_t timeout_ms = (timeout + 999) / 1000;

        i2c_init(handle, self->scl, self->sda, freq, timeout_ms);

        size_t remain_len = 0;
        for (size_t i = 0; i < n; ++i) remain_len += bufs[i].len;

        int ret = i2c_start_addr(handle, flags & MP_MACHINE_I2C_FLAG_READ, addr,
                                 remain_len, flags & MP_MACHINE_I2C_FLAG_STOP);

        if (ret < 0) return ret;

        int num_acks = 0;
        for (; n--; ++bufs) {
            remain_len -= bufs->len;
            if (flags & MP_MACHINE_I2C_FLAG_READ) ret = i2c_read(handle, bufs->buf, bufs->len, remain_len);
            else ret = i2c_write(handle, bufs->buf, bufs->len, remain_len);

            if (ret < 0) return ret;

            num_acks += ret;
        }

        return num_acks;
#else
        // this is a sanity check and it should never run.
        mp_raise_ValueError(MP_ERROR_TEXT("I2C: unrecoverable failure"));
        return -1;
#endif
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


static mp_obj_t machine_i2c_device_writevto(size_t n_args, const mp_obj_t *args)
{
    mp_machine_hw_i2c_device_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_t addr = mp_obj_new_int_from_uint(self->addr);
    mp_obj_t stop = (n_args == 2) ? mp_const_true : args[2];

    const mp_obj_t new_args[] = { args[0], addr, args[1], stop };

    return machine_i2c_writevto(4, new_args);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_i2c_device_writevto_obj, 2, 3, machine_i2c_device_writevto);


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


#endif // MICROPY_HW_ENABLE_HW_I2C
