// Copyright (c) 2013-2018 Damien P. George
// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "../../../../micropy_updates/common/mp_spi_common.h"
#include "../../../../ext_mod/lcd_bus/generic/include/spi_bus.h"

#include "py/runtime.h"
#include "extmod/modmachine.h"
#include "spi.h"

#if MICROPY_PY_MACHINE_SPI

#define SPI_UNUSED(x) ((void)x)

/*
typedef enum _mp_machine_hw_spi_state_t {
    MP_SPI_STATE_STOPPED,
    MP_SPI_STATE_STARTED,
    MP_SPI_STATE_SENDING
} mp_machine_hw_spi_state_t;

typedef struct _mp_machine_hw_spi_bus_obj_t {
    uint8_t host;
    mp_obj_t sck;
    mp_obj_t mosi;
    mp_obj_t miso;
    int16_t active_devices;
    mp_machine_hw_spi_state_t state;
    void *user_data;
} mp_machine_hw_spi_bus_obj_t;


typedef struct _machine_hw_spi_obj_t {
    mp_obj_base_t base;
    uint32_t baudrate;
    uint8_t polarity;
    uint8_t phase;
    uint8_t bits;
    uint8_t firstbit;
    mp_obj_t cs;
    mp_machine_hw_spi_bus_obj_t *spi_bus;
    void *user_data;
} machine_hw_spi_obj_t;

*/

/******************************************************************************/
// Implementation of hard SPI for machine module

static mp_machine_hw_spi_bus_obj_t mp_machine_hw_spi_bus_obj[6];




static void * _get_func(size_t index)
{
    mp_rom_map_elem_t *elem = &(((mp_rom_map_elem_t *)(mp_machine_spi_locals_dict.map.table))[index]);

    if (elem->value.u32.lo == NULL) return elem->value.u32.hi;
    else return elem->value.u32.lo;
}


typedef mp_obj_t (*machine_spi_init_func_t)(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args);
static mp_obj_fun_builtin_var_t *machine_spi_init_func_obj = _get_func(0);
static machine_spi_init_func_t machine_spi_init = machine_spi_init_func_obj->fun.kw;

static MP_DEFINE_CONST_FUN_OBJ_KW(new_machine_spi_init_obj, 1, machine_spi_init);


typedef mp_obj_t (*machine_spi_deinit_func_t)(mp_obj_t self);
static mp_obj_fun_builtin_fixed_t *machine_spi_deinit_func_obj = _get_func(1);
static machine_spi_deinit_func_t machine_spi_deinit = machine_spi_deinit_func_obj->fun._1;

static MP_DEFINE_CONST_FUN_OBJ_1(new_machine_spi_deinit_obj, machine_spi_deinit);

typedef mp_obj_t (*mp_machine_spi_read_func_t)(size_t n_args, const mp_obj_t *args);
static mp_obj_fun_builtin_var_t *mp_machine_spi_read_func_obj = _get_func(2);
static mp_machine_spi_read_func_t mp_machine_spi_read = mp_machine_spi_read_func_obj->fun.var;

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(new_mp_machine_spi_read_obj, 2, 3, mp_machine_spi_read);


typedef mp_obj_t (*mp_machine_spi_readinto_func_t)(size_t n_args, const mp_obj_t *args);
static mp_obj_fun_builtin_var_t *mp_machine_spi_readinto_func_obj = _get_func(3);
static mp_machine_spi_readinto_func_t mp_machine_spi_readinto = mp_machine_spi_readinto_func_obj->fun.var;

MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(new_mp_machine_spi_readinto_obj, 2, 3, mp_machine_spi_readinto);


typedef mp_obj_t (*mp_machine_spi_write_func_t)(mp_obj_t self, mp_obj_t wr_buf);
static mp_obj_fun_builtin_fixed_t *mp_machine_spi_write_func_obj = _get_func(4);
static mp_machine_spi_write_func_t mp_machine_spi_write = mp_machine_spi_write_func_obj->fun._2;

MP_DEFINE_CONST_FUN_OBJ_2(new_mp_machine_spi_write_obj, mp_machine_spi_write);


typedef mp_obj_t (*mp_machine_spi_write_readinto_func_t)(mp_obj_t self, mp_obj_t wr_buf, mp_obj_t rd_buf);
static mp_obj_fun_builtin_fixed_t *mp_machine_spi_write_readinto_func_obj = _get_func(5);
static mp_machine_spi_write_readinto_func_t mp_machine_spi_write_readinto = mp_machine_spi_write_readinto_func_obj->fun._3;

MP_DEFINE_CONST_FUN_OBJ_3(new_mp_machine_spi_write_readinto_obj, mp_machine_spi_write_readinto);


void mp_machine_hw_spi_bus_remove_device(mp_machine_hw_spi_device_obj_t *device)
{
    uint8_t i;
    /*Find the device in the list*/
    for(i = 0; i < device->spi_bus->device_count; i++) {
        if(device->spi_bus->devices[i] == device) break;
    }

    for( ;i < device->spi_bus->device_count - 1; i++) {
        device->spi_bus->devices[i] = device->spi_bus->devices[i + 1];
    }

    device->spi_bus->device_count--;
    device->spi_bus->devices = m_realloc(device->spi_bus->devices,
                                         device->spi_bus->device_count *
                                         sizeof(mp_machine_hw_spi_device_obj_t *));

    if (device->spi_bus->device_count == 0) {
        device->spi_bus->deinit(device->spi_bus);
    }
}


void mp_machine_hw_spi_bus_add_device(mp_machine_hw_spi_device_obj_t *device)
{
    device->spi_bus->device_count++;
    device->spi_bus->devices = m_realloc(device->spi_bus->devices,
                                         device->spi_bus->device_count *
                                         sizeof(mp_machine_hw_spi_device_obj_t *));
}


void machine_hw_spi_bus_deinit_all(void) {}


void mp_machine_hw_spi_bus_initilize(mp_machine_hw_spi_bus_obj_t *bus)
{
    if (bus->state != MP_SPI_STATE_STOPPED) return;

    spi_t *spi = (spi_t *spi)bus->user_data

    SPI_InitTypeDef *init = &spi->spi->Init;

    init->Mode = SPI_MODE_MASTER;

    // these parameters are not currently configurable
    init->Direction = SPI_DIRECTION_2LINES;
    init->NSS = SPI_NSS_SOFT;
    init->TIMode = SPI_TIMODE_DISABLE;
    init->CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    init->CRCPolynomial = 0;

    bus->state = MP_SPI_STATE_STARTED;
}


static mp_obj_t machine_hw_spi_bus_make_new(const mp_obj_type_t *type, size_t n_args,
                                            size_t n_kw, const mp_obj_t *all_args)
{

    enum { ARG_host, ARG_sck, ARG_mosi, ARG_miso, ARG_dual_pins,
           ARG_quad_pins, ARG_octal_pins };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_host,       MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_sck,        MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL   } },
        { MP_QSTR_mosi,       MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL   } },
        { MP_QSTR_miso,       MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = MP_OBJ_NULL   } },
        { MP_QSTR_dual_pins,  MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_quad_pins,  MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_octal_pins, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                              MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get static peripheral object
    int spi_id = spi_find_index(args[ARG_host].u_obj);

    mp_machine_hw_spi_bus_obj_t *self = &machine_hard_spi_bus_obj[spi_id - 1];

    if (args[ARG_dual_pins].u_obj != mp_const_none ||
        args[ARG_quad_pins].u_obj != mp_const_none ||
        args[ARG_octal_pins].u_obj != mp_const_none) {

        mp_raise_ValueError(MP_ERROR_TEXT("dual, quad and octal SPI is not supported"));
    }

    // here we would check the sck/mosi/miso pins and configure them, but it's not implemented
    if (args[ARG_sck].u_obj != MP_OBJ_NULL ||
        args[ARG_mosi].u_obj != MP_OBJ_NULL ||
        args[ARG_miso].u_obj != MP_OBJ_NULL) {

        mp_raise_ValueError(MP_ERROR_TEXT("explicit choice of sck/mosi/miso is not implemented"));
    }

    if (self->base.type == NULL) self->base.type = &mp_machine_hw_spi_bus_type;

    // set the SPI configuration values
    spi_t *spi = &spi_obj[spi_id - 1];
    self->user_data = spi;

    return MP_OBJ_FROM_PTR(self);
}


static mp_obj_t machine_hw_spi_device_make_new(const mp_obj_type_t *type, size_t n_args,
                                        size_t n_kw, const mp_obj_t *all_args)
{

    enum { ARG_spi_bus, ARG_freq, ARG_cs, ARG_polarity, ARG_phase, ARG_bits,
           ARG_firstbit, ARG_dual, ARG_quad, ARG_octal, ARG_cs_high_active };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_spi_bus,        MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_OBJ     },
        { MP_QSTR_freq,           MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_INT     },
        { MP_QSTR_cs,             MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_polarity,       MP_ARG_KW_ONLY | MP_ARG_INT,       { .u_int = 0 } },
        { MP_QSTR_phase,          MP_ARG_KW_ONLY | MP_ARG_INT,       { .u_int = 0 } },
        { MP_QSTR_bits,           MP_ARG_KW_ONLY | MP_ARG_INT,       { .u_int = 8 } },
        { MP_QSTR_firstbit,       MP_ARG_KW_ONLY | MP_ARG_INT,  { .u_int = MICROPY_PY_MACHINE_SPI_MSB} },
        { MP_QSTR_dual,           MP_ARG_KW_ONLY | MP_ARG_BOOL, { .u_bool = false } },
        { MP_QSTR_quad,           MP_ARG_KW_ONLY | MP_ARG_BOOL, { .u_bool = false } },
        { MP_QSTR_octal,          MP_ARG_KW_ONLY | MP_ARG_BOOL, { .u_bool = false } },
        { MP_QSTR_cs_high_active, MP_ARG_KW_ONLY | MP_ARG_BOOL, { .u_bool = false } }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                              MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_machine_hw_spi_device_obj_t *self = m_new_obj(mp_machine_hw_spi_device_obj_t);
    self->base.type = &mp_machine_hw_spi_device_type;

    self->freq = (uint32_t)args[ARG_freq].u_int;
    self->polarity = (uint8_t)args[ARG_polarity].u_int;
    self->phase = (uint8_t)args[ARG_phase].u_int;
    self->bits =  (uint8_t)args[ARG_bits].u_int;
    self->firstbit = (uint8_t)args[ARG_firstbit].u_int;
    self->dual = 0;
    self->quad = 0;
    self->octal = 0;
    self->cs = args[ARG_cs].u_obj;
    self->cs_high_active = (uint8_t)args[ARG_cs_high_active].u_bool;
    self->spi_bus = MP_OBJ_TO_PTR(args[ARG_spi_bus].u_obj);
    self->mem_addr_size = 8;
    self->deinit = &machine_hw_spi_device_deinit_callback;

    uint8_t dual = (uint8_t)args[ARG_dual].u_bool;
    uint8_t quad = (uint8_t)args[ARG_quad].u_bool;
    uint8_t octal = (uint8_t)args[ARG_octal].u_bool;

    if (dual || quad || octal) {
        mp_raise_ValueError(MP_ERROR_TEXT("dual, quad and octal SPI is not supported"));
    }

    if (self->spi_bus->state == MP_SPI_STATE_STOPPED) {
        mp_machine_hw_spi_bus_initilize(self->spi_bus);
    }

    mp_machine_hw_spi_bus_add_device(self);
    self->active = 1;

    if (self->cs != mp_const_none) {
        mp_hal_pin_output(mp_hal_get_pin_obj(self->cs));
        mp_hal_pin_write(mp_hal_get_pin_obj(self->cs), self->cs_high_active ? 0 : 1);
    }

    return MP_OBJ_FROM_PTR(self);
}


static void machine_hw_spi_device_deinit(mp_obj_base_t *self_in) { }


static void machine_hw_spi_device_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest)
{

    mp_machine_hw_spi_device_obj_t *self;
    mp_lcd_spi_bus_obj_t *lcd_dev = NULL;

    if (self_in->base.type == &mp_machine_hw_spi_device_type) {
        self = (mp_machine_hw_spi_device_obj_t *)self_in;
    } else if (self_in->base.type == &mp_lcd_spi_bus_type) {
        lcd_dev = (mp_lcd_spi_bus_obj_t *)self_in;
        self = lcd_dev->spi_device;
    } else {
        return;
    }

    if (self->spi_bus->state == MP_SPI_STATE_STOPPED) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("transfer on deinitialized SPI"));
        return;
    }

    if (!self->active) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI Device is no longer active"));
        return;
    }

    if (lcd_dev == NULL) {
        while (self->spi_bus->state == MP_SPI_STATE_SENDING) { }
        self->spi_bus->state = MP_SPI_STATE_SENDING;
    }

    spi_set_params(
        (const spi_t *)(self->spi_bus->user_data),
        0xffffffff,
        self->freq,
        self->polarity,
        self->phase,
        self->bits,
        self->firstbit
    );

    // init the SPI bus
    int ret = spi_init((const spi_t *)(self->spi_bus->user_data), false);
    if (ret != 0) {
        mp_raise_OSError(-ret);
    }

    if (lcd_dev == NULL && self->cs != mp_const_none) {
        mp_hal_pin_write(mp_hal_get_pin_obj(self->cs), self->cs_high_active);
    }

    if (lcd_dev == NULL && self->mem_addr_set) {
        uint8_t addr_size = self->mem_addr_size / 8;
        if (addr_size * 8 < self->mem_addr_size) addr_size += 1

        uint8_t memaddr_buf[4];

        for (uint8_t i=0;i<addr_size;i++) {
            memaddr_buf[i] = (self->mem_addr >> (i * 8)) & 0xFF;
        }

        spi_transfer(self_in, addr_size, memaddr_buf, NULL, SPI_TRANSFER_TIMEOUT(len));
    }

    spi_transfer(self_in, len, src, dest, SPI_TRANSFER_TIMEOUT(len));

    if (lcd_dev == NULL) {
        if (self->cs != mp_const_none) {
            mp_hal_pin_write(mp_hal_get_pin_obj(self->cs), self->cs_high_active ? 0 : 1);
        }

        self->spi_bus->state = MP_SPI_STATE_STARTED;
    }
}


static mp_obj_t machine_spi_device_set_mem_addr_size(mp_obj_t self_in, mp_obj_t size)
{
    mp_machine_hw_spi_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t addr_size = (uint8_t)mp_obj_get_int_truncated(size);

    if ((addr_size & 7) != 0 || addr_size > 32) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid mem address size"));
    }

    self->mem_addr_size = addr_size;
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(machine_spi_device_set_mem_addr_size_obj, machine_spi_device_set_mem_addr_size);


static mp_obj_t machine_spi_device_readfrom_mem_into(mp_obj_t self_in, mp_obj_t memaddr_in, mp_obj_t buf_in)
{
    mp_machine_hw_spi_device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    self->mem_addr = (uint32_t)mp_obj_get_int_truncated(memaddr_in);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_WRITE);

    self->mem_addr_set = 1;
    machine_hw_spi_device_transfer((mp_obj_base_t *)self, bufinfo.len, NULL, bufinfo.buf)
    self->mem_addr_set = 0;
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_3(machine_spi_device_readfrom_mem_into_obj, machine_spi_device_readfrom_mem_into);


static mp_obj_t machine_spi_device_readfrom_mem(mp_obj_t self_in, mp_obj_t memaddr_in, mp_obj_t n_in)
{
    mp_machine_hw_spi_device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    self->mem_addr = (uint32_t)mp_obj_get_int_truncated(memaddr_in);

    // create the buffer to store data into
    vstr_t vstr;
    vstr_init_len(&vstr, mp_obj_get_int(n_in));

    self->mem_addr_set = 1;
    machine_hw_spi_device_transfer((mp_obj_base_t *)self, vstr.len, NULL, (uint8_t *)vstr.buf)
    self->mem_addr_set = 0;
    return mp_obj_new_bytes_from_vstr(&vstr);
}

MP_DEFINE_CONST_FUN_OBJ_3(machine_spi_device_readfrom_mem_obj, machine_spi_device_readfrom_mem);


static mp_obj_t machine_spi_device_writeto_mem(mp_obj_t self_in, mp_obj_t memaddr_in, mp_obj_t buf_in)
{
    mp_machine_hw_spi_device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    self->mem_addr = (uint32_t)mp_obj_get_int_truncated(memaddr_in);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    self->mem_addr_set = 1;
    mp_obj_t res = machine_hw_spi_device_transfer((mp_obj_base_t *)self, bufinfo.len, bufinfo.buf, NULL)
    self->mem_addr_set = 0;
    return res;
}

MP_DEFINE_CONST_FUN_OBJ_3(machine_spi_device_writeto_mem_obj, machine_spi_device_writeto_mem);


static mp_obj_t machine_hw_spi_device_get_bus(mp_obj_t self_in)
{
    mp_machine_hw_spi_device_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_FROM_PTR(self->spi_bus);
}

MP_DEFINE_CONST_FUN_OBJ_1(machine_hw_spi_device_get_bus_obj, machine_hw_spi_device_get_bus);


static const mp_rom_map_elem_t machine_spi_device_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init),              MP_ROM_PTR(&new_machine_spi_init_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_deinit),            MP_ROM_PTR(&new_machine_spi_deinit_obj)               },
    { MP_ROM_QSTR(MP_QSTR_read),              MP_ROM_PTR(&new_mp_machine_spi_read_obj)              },
    { MP_ROM_QSTR(MP_QSTR_readinto),          MP_ROM_PTR(&new_mp_machine_spi_readinto_obj)          },
    { MP_ROM_QSTR(MP_QSTR_write),             MP_ROM_PTR(&new_mp_machine_spi_write_obj)             },
    { MP_ROM_QSTR(MP_QSTR_write_readinto),    MP_ROM_PTR(&new_mp_machine_spi_write_readinto_obj)    },
    { MP_ROM_QSTR(MP_QSTR_set_mem_addr_size), MP_ROM_PTR(&machine_spi_device_set_mem_addr_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem),      MP_ROM_PTR(&machine_spi_device_readfrom_mem_obj)      },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem_into), MP_ROM_PTR(&machine_spi_device_readfrom_mem_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto_mem),       MP_ROM_PTR(&machine_spi_device_writeto_mem_obj)       },
    { MP_ROM_QSTR(MP_QSTR_get_bus),           MP_ROM_PTR(&machine_spi_device_get_bus_obj)           },
    { MP_ROM_QSTR(MP_QSTR___del__),           MP_ROM_PTR(&machine_spi_device_deinit_obj)            }
};

MP_DEFINE_CONST_DICT(mp_machine_spi_device_locals_dict, machine_spi_device_locals_dict_table);


static const mp_machine_spi_p_t machine_hw_spi_device_p = {
    .deinit = machine_hw_spi_device_deinit,
    .transfer = machine_hw_spi_device_transfer,
};


MP_DEFINE_CONST_OBJ_TYPE(
    mp_machine_hw_spi_device_type,
    MP_QSTR_Device,
    MP_TYPE_FLAG_NONE,
    make_new, machine_hw_spi_device_make_new,
    protocol, &machine_hw_spi_device_p,
    locals_dict, &mp_machine_spi_device_locals_dict
);


static const mp_rom_map_elem_t machine_spi_bus_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&machine_hw_spi_bus_deinit_obj) }
};


MP_DEFINE_CONST_DICT(machine_spi_bus_locals_dict, machine_spi_bus_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_machine_hw_spi_bus_type,
    MP_QSTR_Bus,
    MP_TYPE_FLAG_NONE,
    make_new, machine_hw_spi_bus_make_new,
    locals_dict, &machine_spi_bus_locals_dict
);


static const mp_rom_map_elem_t machine_spi_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),  MP_OBJ_NEW_QSTR(MP_QSTR_SPI)             },
    { MP_ROM_QSTR(MP_QSTR_Bus),       (mp_obj_t)&mp_machine_hw_spi_bus_type    },
    { MP_ROM_QSTR(MP_QSTR_Device),    (mp_obj_t)&mp_machine_hw_spi_device_type },
    { MP_ROM_QSTR(MP_QSTR_MSB),       MP_ROM_INT(MICROPY_PY_MACHINE_SPI_MSB)   },
    { MP_ROM_QSTR(MP_QSTR_LSB),       MP_ROM_INT(MICROPY_PY_MACHINE_SPI_LSB)   }
};

MP_DEFINE_CONST_DICT(machine_spi_locals_dict, machine_spi_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    machine_spi_type,
    MP_QSTR_SPI,
    MP_TYPE_FLAG_NONE,
    locals_dict, &machine_spi_locals_dict
);