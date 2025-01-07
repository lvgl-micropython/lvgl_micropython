// Copyright (c) 2013-2018 Damien P. George
// Copyright (c) 2024 - 2025 Kevin G. Schlosser


#include "../../../../micropy_updates/common/mp_spi_common.h"
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

static mp_machine_hw_spi_bus_obj_t machine_hard_spi_bus_obj[] = {
    {
        .host = 1,
        .sck = MP_OBJ_NULL,
        .mosi = MP_OBJ_NULL,
        .miso = MP_OBJ_NULL,
        .active_devices = 0,
        .state = 0,
        .user_data = &spi_obj[0]
    },
    {
        .host = 2,
        .sck = MP_OBJ_NULL,
        .mosi = MP_OBJ_NULL,
        .miso = MP_OBJ_NULL,
        .active_devices = 0,
        .state = 0,
        .user_data = &spi_obj[1]
    },
    {
        .host = 3,
        .sck = MP_OBJ_NULL,
        .mosi = MP_OBJ_NULL,
        .miso = MP_OBJ_NULL,
        .active_devices = 0,
        .state = 0,
        .user_data = &spi_obj[2]
    },
    {
        .host = 4,
        .sck = MP_OBJ_NULL,
        .mosi = MP_OBJ_NULL,
        .miso = MP_OBJ_NULL,
        .active_devices = 0,
        .state = 0,
        .user_data = &spi_obj[3]
    },
    {
        .host = 5,
        .sck = MP_OBJ_NULL,
        .mosi = MP_OBJ_NULL,
        .miso = MP_OBJ_NULL,
        .active_devices = 0,
        .state = 0,
        .user_data = &spi_obj[4]
    },
    {
        .host = 6,
        .sck = MP_OBJ_NULL,
        .mosi = MP_OBJ_NULL,
        .miso = MP_OBJ_NULL,
        .active_devices = 0,
        .state = 0,
        .user_data = &spi_obj[5]
    }
};


void machine_hw_spi_bus_deinit_all(void) {}


static void machine_hard_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_hw_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    spi_print(print, (const spi_t *)(self->spi_bus->user_data), false);
}

mp_obj_t machine_hard_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    MP_MACHINE_SPI_CHECK_FOR_LEGACY_SOFTSPI_CONSTRUCTION(n_args, n_kw, all_args);

    enum { ARG_id, ARG_baudrate, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit, ARG_sck, ARG_mosi, ARG_miso, ARG_cs };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 500000} },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = SPI_FIRSTBIT_MSB} },
        { MP_QSTR_sck,      MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_mosi,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_miso,     MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_cs,       MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none } },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get static peripheral object
    int spi_id = spi_find_index(args[ARG_id].u_obj);

    mp_machine_hw_spi_bus_obj_t *spi_bus = &machine_hard_spi_bus_obj[spi_id - 1];
    machine_hw_spi_obj_t *self = m_new_obj(machine_hw_spi_obj_t);
    self->base.type = &machine_spi_type;
    self->spi_bus = spi_bus;

    // here we would check the sck/mosi/miso pins and configure them, but it's not implemented
    if (args[ARG_sck].u_obj != MP_OBJ_NULL
        || args[ARG_mosi].u_obj != MP_OBJ_NULL
        || args[ARG_miso].u_obj != MP_OBJ_NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("explicit choice of sck/mosi/miso is not implemented"));
    }

    // set the SPI configuration values

    spi_t *spi = (spi_t *)self->spi_bus->user_data;
    SPI_InitTypeDef *init = &spi->spi->Init;

    init->Mode = SPI_MODE_MASTER;

    // these parameters are not currently configurable
    init->Direction = SPI_DIRECTION_2LINES;
    init->NSS = SPI_NSS_SOFT;
    init->TIMode = SPI_TIMODE_DISABLE;
    init->CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    init->CRCPolynomial = 0;

    self->polarity = (uint8_t)args[ARG_polarity].u_int;
    self->phase = (uint8_t)args[ARG_phase].u_int;
    self->bits = (uint8_t)args[ARG_bits].u_int;
    self->firstbit = (uint8_t)args[ARG_firstbit].u_int;
    self->baudrate = (uint32_t)args[ARG_baudrate].u_int;

    self->cs = args[ARG_cs].u_obj;
    if (self->spi_bus->state == MP_SPI_STATE_STOPPED) {
        // set configurable parameters
        spi_set_params(
            (const spi_t *)self->spi_bus->user_data,
            0xffffffff,
            self->baudrate,
            self->polarity,
            self->phase,
            self->bits,
            self->firstbit
        );

        // init the SPI bus
        int ret = spi_init((const spi_t *)self->spi_bus->user_data, false);
        if (ret != 0) {
            mp_raise_OSError(-ret);
        }

        self->spi_bus->state = MP_SPI_STATE_STARTED;
    }

    if (self->cs != mp_const_none) {
        mp_hal_pin_output(mp_hal_get_pin_obj(self->cs));
        mp_hal_pin_write(mp_hal_get_pin_obj(self->cs), 1);
    }

    self->spi_bus->active_devices++;
    self->active = true;

    return MP_OBJ_FROM_PTR(self);
}

static void machine_hard_spi_init(mp_obj_base_t *self_in, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    SPI_UNUSED(self_in);
    SPI_UNUSED(n_args);
    SPI_UNUSED(pos_args);
    SPI_UNUSED(kw_args);
}

static void machine_hard_spi_deinit(mp_obj_base_t *self_in) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *)self_in;

    if (self->active) {
        self->active = false;
        self->spi_bus->active_devices--;

        if (self->spi_bus->active_devices == 0) {
          spi_deinit((const spi_t *)(self->spi_bus->user_data));
          self->spi_bus->state = MP_SPI_STATE_STOPPED;
        }
    }
}

static void machine_hard_spi_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest) {
    machine_hw_spi_obj_t *self = (machine_hw_spi_obj_t *)self_in;

    if (self->spi_bus->state == MP_SPI_STATE_STOPPED) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("transfer on deinitialized SPI"));
        return;
    }

    if (!self->active) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI Device is no longer active"));
        return;
    }

    while (self->spi_bus->state == MP_SPI_STATE_SENDING) {}

    self->spi_bus->state = MP_SPI_STATE_SENDING;

    spi_set_params(
        (const spi_t *)(self->spi_bus->user_data),
        0xffffffff,
        self->baudrate,
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

    if (self->cs != mp_const_none) {
        mp_hal_pin_write(mp_hal_get_pin_obj(self->cs), 0);
    }

    spi_transfer((const spi_t *)(self->spi_bus->user_data), len, src, dest, SPI_TRANSFER_TIMEOUT(len));

    if (self->cs != mp_const_none) {
        mp_hal_pin_write(mp_hal_get_pin_obj(self->cs), 1);
    }

    self->spi_bus->state = MP_SPI_STATE_STARTED;
}

static const mp_machine_spi_p_t machine_hard_spi_p = {
    .init = machine_hard_spi_init,
    .deinit = machine_hard_spi_deinit,
    .transfer = machine_hard_spi_transfer,
};

MP_DEFINE_CONST_OBJ_TYPE(
    machine_spi_type,
    MP_QSTR_SPI,
    MP_TYPE_FLAG_NONE,
    make_new, machine_hard_spi_make_new,
    print, machine_hard_spi_print,
    protocol, &machine_hard_spi_p,
    locals_dict, &mp_machine_spi_locals_dict
    );

#endif // MICROPY_PY_MACHINE_SPI
