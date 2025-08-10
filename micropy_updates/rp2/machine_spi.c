// Copyright (c) 2020-2021 Damien P. George
// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "py/runtime.h"
#include "py/mphal.h"
#include "py/mperrno.h"
#include "extmod/modmachine.h"

#include "../../../../micropy_updates/common/mp_spi_common.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

#define SPI_UNUSED(x) ((void)x)

#define DEFAULT_SPI_BAUDRATE    (1000000)
#define DEFAULT_SPI_POLARITY    (0)
#define DEFAULT_SPI_PHASE       (0)
#define DEFAULT_SPI_BITS        (8)
#define DEFAULT_SPI_FIRSTBIT    (SPI_MSB_FIRST)
#define DEFAULT_SPI_CS_PIN      (-1)

#ifdef MICROPY_HW_SPI_NO_DEFAULT_PINS

// With no default SPI, need to require the pin args.
#define MICROPY_HW_SPI0_SCK     (0)
#define MICROPY_HW_SPI0_MOSI    (0)
#define MICROPY_HW_SPI0_MISO    (0)
#define MICROPY_HW_SPI1_SCK     (0)
#define MICROPY_HW_SPI1_MOSI    (0)
#define MICROPY_HW_SPI1_MISO    (0)
#define MICROPY_SPI_PINS_ARG_OPTS MP_ARG_REQUIRED

#else

// Most boards do not require pin args.
#define MICROPY_SPI_PINS_ARG_OPTS 0

#ifndef MICROPY_HW_SPI0_SCK
#if PICO_DEFAULT_SPI == 0
#define MICROPY_HW_SPI0_SCK     (PICO_DEFAULT_SPI_SCK_PIN)
#define MICROPY_HW_SPI0_MOSI    (PICO_DEFAULT_SPI_TX_PIN)
#define MICROPY_HW_SPI0_MISO    (PICO_DEFAULT_SPI_RX_PIN)
#else
#define MICROPY_HW_SPI0_SCK     (6)
#define MICROPY_HW_SPI0_MOSI    (7)
#define MICROPY_HW_SPI0_MISO    (4)
#endif
#endif

#ifndef MICROPY_HW_SPI1_SCK
#if PICO_DEFAULT_SPI == 1
#define MICROPY_HW_SPI1_SCK     (PICO_DEFAULT_SPI_SCK_PIN)
#define MICROPY_HW_SPI1_MOSI    (PICO_DEFAULT_SPI_TX_PIN)
#define MICROPY_HW_SPI1_MISO    (PICO_DEFAULT_SPI_RX_PIN)
#else
#define MICROPY_HW_SPI1_SCK     (10)
#define MICROPY_HW_SPI1_MOSI    (11)
#define MICROPY_HW_SPI1_MISO    (8)
#endif
#endif

#endif

// SPI0 can be GP{0..7,16..23}, SPI1 can be GP{8..15,24..29}.
#define IS_VALID_PERIPH(spi, pin)   ((((pin) & 8) >> 3) == (spi))
// GP{2,6,10,14,...}
#define IS_VALID_SCK(spi, pin)      (((pin) & 3) == 2 && IS_VALID_PERIPH(spi, pin))
// GP{3,7,11,15,...}
#define IS_VALID_MOSI(spi, pin)     (((pin) & 3) == 3 && IS_VALID_PERIPH(spi, pin))
// GP{0,4,8,10,...}
#define IS_VALID_MISO(spi, pin)     (((pin) & 3) == 0 && IS_VALID_PERIPH(spi, pin))


static mp_machine_hw_spi_bus_obj_t machine_hw_spi_bus_objs[2];


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


void mp_machine_hw_spi_bus_add_device(mp_machine_hw_spi_device_obj_t *device)
{
    device->spi_bus->device_count++;
    device->spi_bus->devices = m_realloc(device->spi_bus->devices,
                                         device->spi_bus->device_count *
                                         sizeof(mp_machine_hw_spi_device_obj_t *));
}

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


void mp_machine_hw_spi_bus_initilize(mp_machine_hw_spi_bus_obj_t *bus)
{
    if (bus->state != MP_SPI_STATE_STOPPED) return;

    spi_inst_t *const spi_inst = (spi_inst_t *const)bus->user_data;
    spi_init(spi_inst, 10000000);

    gpio_set_function(mp_hal_get_pin_obj(bus->sck), GPIO_FUNC_SPI);

    if (bus->miso != mp_const_none) {
        gpio_set_function(mp_hal_get_pin_obj(bus->miso), GPIO_FUNC_SPI);
    }
    if (bus->mosi != mp_const_none) {
        gpio_set_function(mp_hal_get_pin_obj(bus->mosi), GPIO_FUNC_SPI);
    }

    bus->state = MP_SPI_STATE_STARTED;
}


static void machine_hw_spi_device_deinit_callback(mp_machine_hw_spi_device_obj_t *self)
{
    if (!self->active) return;
}


static void machine_hw_spi_device_deinit_internal(mp_machine_hw_spi_device_obj_t *self)
{
    if (!self->active) return;

    mp_machine_hw_spi_bus_remove_device(self);
}


void machine_hw_spi_bus_deinit_internal(mp_machine_hw_spi_bus_obj_t *self)
{
    if (self->state == MP_SPI_STATE_STOPPED) return;

    for (uint8_t i = 0; i < self->device_count; i++) {
        self->devices[i]->deinit(self->devices[i]);
        self->devices[i]->active = false;
    }

    self->device_count = 0;
    self->devices = m_realloc(self->devices, self->device_count * sizeof(mp_machine_hw_spi_device_obj_t *));


    self->state = MP_SPI_STATE_STOPPED;
}


static void machine_hw_spi_device_deinit(mp_obj_base_t *self_in)
{
    mp_machine_hw_spi_device_obj_t *self = (mp_machine_hw_spi_device_obj_t *)self_in;
    machine_hw_spi_device_deinit_internal(self);
}


void machine_hw_spi_bus_deinit_all(void) { }


mp_obj_t machine_spi_bus_make_new(const mp_obj_type_t *type, size_t n_args,
                                  size_t n_kw, const mp_obj_t *all_args)
{

    enum { ARG_host, ARG_sck, ARG_mosi, ARG_miso, ARG_dual_pins,
           ARG_quad_pins, ARG_octal_pins };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_host,       MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_sck,        MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_mosi,       MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_int = mp_const_none } },
        { MP_QSTR_miso,       MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_int = mp_const_none } },
        { MP_QSTR_dual_pins,  MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_quad_pins,  MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_octal_pins, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                              MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint8_t host = (uint8_t)args[ARG_host].u_int;

    if (args[ARG_miso].u_obj == mp_const_none && args[ARG_mosi] == mp_const_none) {
        mp_raise_ValueError(MP_ERROR_TEXT("You must supply miso, mosi or miso & mosi pins"));
    }

    int sck = mp_hal_get_pin_obj(args[ARG_sck].u_obj);
    if (!IS_VALID_SCK(self->spi_id, sck)) mp_raise_ValueError(MP_ERROR_TEXT("bad SCK pin"));

    int mosi = -1
    if (args[ARG_mosi].u_obj != mp_const_none) {
        mosi = mp_hal_get_pin_obj(args[ARG_mosi].u_obj);

        if (!IS_VALID_MOSI(host, mosi)) mp_raise_ValueError(MP_ERROR_TEXT("bad MOSI pin"));
    }

    int miso = -1
    if (args[ARG_miso].u_obj != mp_const_none) {
        miso = mp_hal_get_pin_obj(args[ARG_miso].u_obj);

        if (!IS_VALID_MISO(host, miso)) mp_raise_ValueError(MP_ERROR_TEXT("bad MISO pin"));
    }

    if (args[ARG_dual_pins].u_obj != mp_const_none ||
        args[ARG_quad_pins].u_obj != mp_const_none ||
        args[ARG_octal_pins].u_obj != mp_const_none) {

        mp_raise_ValueError(MP_ERROR_TEXT("Dual, quad and octal SPI is not supported"));
    }

    if (host < 0 || host >= 2) mp_raise_ValueError(MP_ERROR_TEXT("SPI host doesn't exist"));

    mp_machine_hw_spi_bus_obj_t *self = &machine_hw_spi_bus_objs[host];

    bool reconfigure = false;

    if (self->base.type == NULL) {
        reconfigure = true;
        self->base.type = &mp_machine_hw_spi_bus_type;
        self->host = host;
        self->deinit = &machine_hw_spi_bus_deinit_internal;

        self->dual = 0;
        self->quad = 0;
        self->octal = 0;

        if (host == 0) self->user_data = spi0;
        else self->user_data = spi1;

    } else {
        if ((int)mp_hal_get_pin_obj(self->data0) != mosi) reconfigure = true;
        if ((int)mp_hal_get_pin_obj(self->data1) != miso) reconfigure = true;
        if ((int)mp_hal_get_pin_obj(self->sck) != sck) reconfigure = true;
    }

    if (reconfigure) {
        if (self->device_count > 0) {
            mp_raise_ValueError(MP_ERROR_TEXT("SPI has active devices, unable to change pins"));
            return mp_const_none;
        }

        self->data0 = mp_obj_new_int(mosi);
        self->data1 = mp_obj_new_int(miso);
        self->sck = mp_obj_new_int(sck);
    }

    return MP_OBJ_FROM_PTR(self);
}


static mp_obj_t machine_hw_spi_bus_deinit(mp_obj_t self_in)
{
    mp_machine_hw_spi_bus_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_hw_spi_bus_deinit_internal(self);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(machine_hw_spi_bus_deinit_obj, machine_hw_spi_bus_deinit);


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


    if (self->firstbit == SPI_LSB_FIRST) {
        mp_raise_NotImplementedError(MP_ERROR_TEXT("LSB"));
    }

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


static int chan_tx = -1;
static mp_lcd_spi_bus_obj_t *lcd_dev = NULL;


static void dma_handler(void)
{
    if (dma_channel_get_irq0_status(chan_tx))
    {
        dma_channel_acknowledge_irq0(chan_tx);
        lcd_dev->dma_callback(lcd_dev);
        lcd_dev = NULL;
        dma_channel_unclaim(chan_tx);
        chan_tx = -1;
    }
}


static void machine_spi_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest)
{
    mp_machine_hw_spi_device_obj_t *self;

    if (self_in->base.type == &mp_machine_hw_spi_device_type) {
        self = (mp_machine_hw_spi_device_obj_t *)self_in;
    } else if (self_in->base.type == &mp_lcd_spi_bus_type) {
        lcd_dev = (mp_lcd_spi_bus_obj_t *)self_in;
        self = lcd_dev->spi_device;
    } else {
        return;
    }

    if (!self->active) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI device is no longer attached to a bus"));
        return;
    }
    if (self->spi_bus->state == MP_SPI_STATE_STOPPED) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI bus is no longer active"));
        return;
    }

    if (lcd_dev == NULL) {
        while (self->spi_bus->state == MP_SPI_STATE_SENDING) {}

        self->spi_bus->state = MP_SPI_STATE_SENDING;
    }

    spi_inst_t *const spi_inst = (spi_inst_t *const spi_inst)self->spi_bus->user_data;

    spi_set_baudrate(spi_inst, self->freq);
    spi_set_format(spi_inst, self->bits, self->polarity, self->phase, 1)

    int chan_rx = -1;

    if (lcd_dev != NULL && lcd_dev->use_dma) {
        // DMA Config
        chan_tx = dma_claim_unused_channel(true);
        dma_channel_config c = dma_channel_get_default_config(chan_tx);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
        channel_config_set_dreq(&c, spi_get_index(spi_inst) ? DREQ_SPI1_TX : DREQ_SPI0_TX);

        dma_channel_set_irq0_enabled(chan_tx, true);
        irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
        irq_set_enabled(DMA_IRQ_0, true);
        dma_channel_configure(chan_tx, &c, &spi_get_hw(spi_inst)->dr, src, len, true);
    } else {
        if (self->cs != mp_const_none) {
            mp_hal_pin_write(mp_hal_get_pin_obj(self->cs), self->cs_high_active);
        }

        if (lcd_dev == NULL && self->mem_addr_set) {
            uint8_t addr_size = self->mem_addr_size / 8;
            if (addr_size * 8 < self->mem_addr_size) addr_size += 1

            uint8_t memaddr_buf[4];

            for (uint8_t i=0;i<addr_size;i++) {
                memaddr_buf[i] = (self->mem_addr >> (i * 8)) & 0xFF;
            }
            spi_write_blocking(spi_inst, memaddr_buf, addr_size);
        }

        // Use DMA for large transfers if channels are available
        const size_t dma_min_size_threshold = 32;

        if (len >= dma_min_size_threshold) {
            // Use two DMA channels to service the two FIFOs
            chan_tx = dma_claim_unused_channel(false);
            chan_rx = dma_claim_unused_channel(false);
        }

        bool use_dma = chan_rx >= 0 && chan_tx >= 0;
        // note src is guaranteed to be non-NULL

        bool write_only = dest == NULL;

        if (use_dma) {
            uint8_t dev_null;
            dma_channel_config c = dma_channel_get_default_config(chan_tx);
            channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
            channel_config_set_dreq(&c, spi_get_index(spi_inst) ? DREQ_SPI1_TX : DREQ_SPI0_TX);
            dma_channel_configure(chan_tx, &c, &spi_get_hw(spi_inst)->dr, src, len, false);

            c = dma_channel_get_default_config(chan_rx);
            channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
            channel_config_set_dreq(&c, spi_get_index(spi_inst) ? DREQ_SPI1_RX : DREQ_SPI0_RX);
            channel_config_set_read_increment(&c, false);
            channel_config_set_write_increment(&c, !write_only);
            dma_channel_configure(chan_rx, &c, write_only ? &dev_null : dest, &spi_get_hw(spi_inst)->dr, len, false);

            dma_start_channel_mask((1u << chan_rx) | (1u << chan_tx));
            dma_channel_wait_for_finish_blocking(chan_rx);
            dma_channel_wait_for_finish_blocking(chan_tx);
        }

        // If we have claimed only one channel successfully, we should release immediately
        if (chan_rx >= 0) dma_channel_unclaim(chan_rx);
        if (chan_tx >= 0) dma_channel_unclaim(chan_tx);

        chan_tx = -1;

        if (!use_dma) {
            // Use software for small transfers, or if couldn't claim two DMA channels
            if (write_only) spi_write_blocking(spi_inst, src, len);
            else spi_write_read_blocking(spi_inst, src, dest, len);
        }

        if (self->cs != mp_const_none && (int)mp_obj_get_int(self->cs) != -1) {
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

MP_DEFINE_CONST_DICT(mp_machine_spi_device_locals_dict,
                     machine_spi_device_locals_dict_table);


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

mp_obj_base_t *mp_hal_get_spi_obj(mp_obj_t o) {
    if (mp_obj_is_type(o, &mp_machine_hw_spi_bus_type)) {
        return MP_OBJ_TO_PTR(o);
    } else if (mp_obj_is_type(o, &mp_machine_hw_spi_device_type)) {
        return MP_OBJ_TO_PTR(o);
    }
    #if MICROPY_PY_MACHINE_SOFTSPI
    else if (mp_obj_is_type(o, &mp_machine_soft_spi_type)) {
        return MP_OBJ_TO_PTR(o);
    }
    #endif
    else {
        mp_raise_TypeError(MP_ERROR_TEXT("expecting an SPI object"));
    }
}

