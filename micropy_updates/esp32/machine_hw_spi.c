// Copyright (c) 2017 "Eric Poulsen" <eric@zyxod.com>
// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "../../../../micropy_updates/common/mp_spi_common.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mphal.h"
#include "extmod/modmachine.h"
#include "mphalport.h"

#include "driver/spi_master.h"
#include "soc/gpio_sig_map.h"
#include "soc/spi_pins.h"
#include "hal/spi_ll.h"

// SPI mappings by device, naming used by IDF old/new
// upython   | ESP32     | ESP32S2   | ESP32S3 | ESP32C3
// ----------+-----------+-----------+---------+---------
// SPI(id=1) | HSPI/SPI2 | FSPI/SPI2 | SPI2    | SPI2
// SPI(id=2) | VSPI/SPI3 | HSPI/SPI3 | SPI3    | err

// Number of available hardware SPI peripherals.
#if SOC_SPI_PERIPH_NUM > 2
#define MICROPY_HW_SPI_MAX (2)
#else
#define MICROPY_HW_SPI_MAX (1)
#endif

// Default pins for SPI(id=1) aka IDF SPI2, can be overridden by a board
#ifndef MICROPY_HW_SPI1_SCK
// Use IO_MUX pins by default.
// If SPI lines are routed to other pins through GPIO matrix
// routing adds some delay and lower limit applies to SPI clk freq
#define MICROPY_HW_SPI1_SCK SPI2_IOMUX_PIN_NUM_CLK
#define MICROPY_HW_SPI1_MOSI SPI2_IOMUX_PIN_NUM_MOSI
#define MICROPY_HW_SPI1_MISO SPI2_IOMUX_PIN_NUM_MISO
#endif

#ifndef MICROPY_HW_SPI1_CS
#define MICROPY_HW_SPI1_CS SPI2_IOMUX_PIN_NUM_CS
#endif

// Default pins for SPI(id=2) aka IDF SPI3, can be overridden by a board
#ifndef MICROPY_HW_SPI2_SCK
#if CONFIG_IDF_TARGET_ESP32
// ESP32 has IO_MUX pins for VSPI/SPI3 lines, use them as defaults
#define MICROPY_HW_SPI2_SCK SPI3_IOMUX_PIN_NUM_CLK      // pin 18
#define MICROPY_HW_SPI2_MOSI SPI3_IOMUX_PIN_NUM_MOSI    // pin 23
#define MICROPY_HW_SPI2_MISO SPI3_IOMUX_PIN_NUM_MISO    // pin 19
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
// ESP32S2 and S3 uses GPIO matrix for SPI3 pins, no IO_MUX possible
// Set defaults to the pins used by SPI2 in Octal mode
#define MICROPY_HW_SPI2_SCK SPI2_IOMUX_PIN_NUM_CLK_OCT
#define MICROPY_HW_SPI2_MOSI SPI2_IOMUX_PIN_NUM_MOSI_OCT
#define MICROPY_HW_SPI2_MISO SPI2_IOMUX_PIN_NUM_MISO_OCT
#endif
#endif

#ifdef MICROPY_HW_SPI2_SCK
#ifndef MICROPY_HW_SPI2_CS
#if CONFIG_IDF_TARGET_ESP32
#define MICROPY_HW_SPI2_CS SPI3_IOMUX_PIN_NUM_CS
#elif CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define MICROPY_HW_SPI2_CS SPI2_IOMUX_PIN_NUM_CS_OCT
#endif
#endif
#endif

#define SPI_UNUSED(x) ((void)x)

#define MP_HW_SPI_MAX_XFER_BYTES (4092)
#define MP_HW_SPI_MAX_XFER_BITS (MP_HW_SPI_MAX_XFER_BYTES * 8) // Has to be an even multiple of 8


static void machine_hw_spi_bus_deinit_internal(mp_machine_hw_spi_bus_obj_t *self);


typedef struct _machine_hw_spi_default_pins_t {
    int8_t sck;
    int8_t mosi;
    int8_t miso;
    int8_t cs;
} machine_hw_spi_default_pins_t;


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

// Static objects mapping to SPI2 (and SPI3 if available) hardware peripherals.
static mp_machine_hw_spi_bus_obj_t *machine_hw_spi_bus_objs[MICROPY_HW_SPI_MAX];


void mp_machine_hw_spi_bus_deinit_all(void)
{
    for (int i=0;i<MICROPY_HW_SPI_MAX;i++) {
        if (machine_hw_spi_bus_objs[i] != NULL) {
            machine_hw_spi_bus_deinit_internal(machine_hw_spi_bus_objs[i]);
            m_del_obj(mp_machine_hw_spi_bus_obj_t, machine_hw_spi_bus_objs[i]);
            machine_hw_spi_bus_objs[i] = NULL;
        }
    }
}


void mp_machine_hw_spi_bus_add_device(mp_machine_hw_spi_device_obj_t *device)
{
    device->spi_bus->device_count++;
    device->spi_bus->devices = m_realloc(device->spi_bus->devices, device->spi_bus->device_count * sizeof(mp_machine_hw_spi_device_obj_t *));
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
    device->spi_bus->devices = m_realloc(device->spi_bus->devices, device->spi_bus->device_count * sizeof(mp_machine_hw_spi_device_obj_t *));

    if (device->spi_bus->device_count == 0) {
        device->spi_bus->deinit(device->spi_bus);
    }
}


static void machine_hw_spi_device_deinit_callback(mp_machine_hw_spi_device_obj_t *self)
{
    if (!self->active) return;

    switch (spi_bus_remove_device((spi_device_handle_t)self->user_data)) {
        case ESP_ERR_INVALID_ARG:
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("invalid configuration"));
            return;

        case ESP_ERR_INVALID_STATE:
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("SPI device already freed"));
            return;
    }
    int cs = (int)mp_obj_get_int(self->cs);

    if (cs != -1) {
        esp_rom_gpio_pad_select_gpio(cs);
        esp_rom_gpio_connect_out_signal(cs, SIG_GPIO_OUT_IDX, false, false);
        gpio_set_direction(cs, GPIO_MODE_INPUT);
    }
}


static void machine_hw_spi_device_deinit_internal(mp_machine_hw_spi_device_obj_t *self)
{
    if (!self->active) return;

    switch (spi_bus_remove_device((spi_device_handle_t)self->user_data)) {
        case ESP_ERR_INVALID_ARG:
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("invalid configuration"));
            return;

        case ESP_ERR_INVALID_STATE:
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("SPI device already freed"));
            return;
    }
    int cs = (int)mp_obj_get_int(self->cs);

    if (cs != -1) {
        esp_rom_gpio_pad_select_gpio(cs);
        esp_rom_gpio_connect_out_signal(cs, SIG_GPIO_OUT_IDX, false, false);
        gpio_set_direction(cs, GPIO_MODE_INPUT);
    }

    mp_machine_hw_spi_bus_remove_device(self);
}


static void machine_hw_spi_device_deinit(mp_obj_base_t *self_in)
{
    mp_machine_hw_spi_device_obj_t *self = (mp_machine_hw_spi_device_obj_t *)self_in;
    machine_hw_spi_device_deinit_internal(self);
}


static mp_uint_t gcd(mp_uint_t x, mp_uint_t y)
{
    while (x != y) {
        if (x > y) {
            x -= y;
        } else {
            y -= x;
        }
    }
    return x;
}

static void disable_gpio(int gpio_num) {
    if (gpio_num != -1) {
        esp_rom_gpio_pad_select_gpio(gpio_num);
        esp_rom_gpio_connect_out_signal(gpio_num, SIG_GPIO_OUT_IDX, false, false);
        gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
        gpio_reset_pin(gpio_num);
    }
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

    switch (spi_bus_free((spi_host_device_t)self->host)) {
        case ESP_ERR_INVALID_ARG:
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("invalid configuration"));
            return;

        case ESP_ERR_INVALID_STATE:
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI bus already freed"));
            return;
    }

    disable_gpio((int)mp_obj_get_int(self->data0));
    disable_gpio((int)mp_obj_get_int(self->data1));
    disable_gpio((int)mp_obj_get_int(self->sck));
    disable_gpio((int)mp_obj_get_int(self->data2));
    disable_gpio((int)mp_obj_get_int(self->data3));
    disable_gpio((int)mp_obj_get_int(self->data4));
    disable_gpio((int)mp_obj_get_int(self->data5));
    disable_gpio((int)mp_obj_get_int(self->data6));
    disable_gpio((int)mp_obj_get_int(self->data7));

    self->state = MP_SPI_STATE_STOPPED;
}



static mp_obj_t machine_hw_spi_bus_deinit(mp_obj_t self_in)
{
    mp_machine_hw_spi_bus_obj_t *self = MP_OBJ_TO_PTR(self_in);
    machine_hw_spi_bus_deinit_internal(self);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(machine_hw_spi_bus_deinit_obj, machine_hw_spi_bus_deinit);


static void machine_hw_spi_device_transfer(mp_obj_base_t *self_in, size_t len, const uint8_t *src, uint8_t *dest)
{
    mp_machine_hw_spi_device_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self->active) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI device is no longer attached to a bus"));
        return;
    }
    if (self->spi_bus->state == MP_SPI_STATE_STOPPED) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("SPI bus is no longer active"));
        return;
    }

    spi_device_handle_t spi_device = (spi_device_handle_t)self->user_data;
    spi_device_acquire_bus(spi_device, portMAX_DELAY);

    // Round to nearest whole set of bits
    int bits_to_send = len * 8 / self->bits * self->bits;

    if (!bits_to_send) {
        mp_raise_ValueError(MP_ERROR_TEXT("buffer too short"));
    }

    if (len <= 4) {
        spi_transaction_t transaction = { 0 };

        if (src != NULL) {
            memcpy(&transaction.tx_data, src, len);
        }

        transaction.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
        transaction.length = bits_to_send;
        if (self->dual) {
            transaction.flags |= SPI_TRANS_MODE_DIO | SPI_TRANS_MULTILINE_ADDR;
        } else if (self->quad) {
            transaction.flags |= SPI_TRANS_MODE_QIO | SPI_TRANS_MULTILINE_ADDR;
        } else if (self->octal) {
            transaction.flags |= SPI_TRANS_MODE_OCT | SPI_TRANS_MULTILINE_ADDR;
        }

        spi_device_transmit(spi_device, &transaction);

        if (dest != NULL) {
            memcpy(dest, &transaction.rx_data, len);
        }
    } else {
        int offset = 0;
        int bits_remaining = bits_to_send;
        int optimum_word_size = 8 * self->bits / gcd(8, self->bits);
        int max_transaction_bits = SPI_LL_DMA_MAX_BIT_LEN / optimum_word_size * optimum_word_size;
        spi_transaction_t *transaction, *result, transactions[2];
        int i = 0;


        while (bits_remaining) {
            transaction = transactions + i++ % 2;
            memset(transaction, 0, sizeof(spi_transaction_t));

            if (bits_remaining > max_transaction_bits) {
                transaction->length = max_transaction_bits;
                transaction->flags |= SPI_TRANS_CS_KEEP_ACTIVE;
            } else {
                transaction->length = bits_remaining;
            }

            if (src != NULL) {
                transaction->tx_buffer = src + offset;
                transaction->flags |= SPI_TRANS_USE_TXDATA;
            }
            if (dest != NULL) {
                transaction->rx_buffer = dest + offset;
                transaction->flags |= SPI_TRANS_USE_RXDATA;
            }

            if (self->dual) {
                transaction->flags |= SPI_TRANS_MODE_DIO | SPI_TRANS_MULTILINE_ADDR;
            } else if (self->quad) {
                transaction->flags |= SPI_TRANS_MODE_QIO | SPI_TRANS_MULTILINE_ADDR;
            } else if (self->octal) {
                transaction->flags |= SPI_TRANS_MODE_OCT | SPI_TRANS_MULTILINE_ADDR;
            }

            spi_device_queue_trans(spi_device, transaction, portMAX_DELAY);
            bits_remaining -= transaction->length;

            if (offset > 0) {
                // wait for previously queued transaction
                MP_THREAD_GIL_EXIT();
                spi_device_get_trans_result(spi_device, &result, portMAX_DELAY);
                MP_THREAD_GIL_ENTER();
            }

            // doesn't need ceil(); loop ends when bits_remaining is 0
            offset += transaction->length / 8;
        }

        // wait for last transaction
        MP_THREAD_GIL_EXIT();
        spi_device_get_trans_result(spi_device, &result, portMAX_DELAY);
        MP_THREAD_GIL_ENTER();
    }

    spi_device_release_bus(spi_device);
}

/******************************************************************************/
// MicroPython bindings for hw_spi

mp_obj_t machine_hw_spi_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    enum { ARG_host, ARG_sck, ARG_mosi, ARG_miso, ARG_dual_pins, ARG_quad_pins, ARG_octal_pins };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_host,       MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_sck,        MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_mosi,       MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = -1 } },
        { MP_QSTR_miso,       MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = -1 } },
        { MP_QSTR_dual_pins,  MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_quad_pins,  MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
        { MP_QSTR_octal_pins, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_obj = mp_const_none } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint8_t host = (uint8_t)args[ARG_host].u_int;
    int sck = (int)args[ARG_sck].u_int;
    int data0 = -1;
    int data1 = -1;
    int data2 = -1;
    int data3 = -1;
    int data4 = -1;
    int data5 = -1;
    int data6 = -1;
    int data7 = -1;

    bool dual = false;
    bool quad = false;
    bool octal = false;

    mp_machine_hw_spi_bus_obj_t *self;

    if (args[ARG_dual_pins].u_obj != mp_const_none) {
        mp_obj_tuple_t *dual_data_pins = MP_OBJ_TO_PTR(args[ARG_dual_pins].u_obj);

        if (dual_data_pins->len != 2) {
            mp_raise_msg_varg(
                &mp_type_ValueError,
                MP_ERROR_TEXT("2 pins needed for quad SPI not %d"),
                dual_data_pins->len
            );
            return mp_const_none;
        }

        data0 = (int)mp_obj_get_int(dual_data_pins->items[0]);
        data1 = (int)mp_obj_get_int(dual_data_pins->items[1]);

        dual = true;

    } else if (args[ARG_quad_pins].u_obj != mp_const_none) {
        mp_obj_tuple_t *quad_data_pins = MP_OBJ_TO_PTR(args[ARG_quad_pins].u_obj);

        if (quad_data_pins->len != 4) {
            mp_raise_msg_varg(
                &mp_type_ValueError,
                MP_ERROR_TEXT("4 pins needed for quad SPI not %d"),
                quad_data_pins->len
            );
            return mp_const_none;
        }

        data0 = (int)mp_obj_get_int(quad_data_pins->items[0]);
        data1 = (int)mp_obj_get_int(quad_data_pins->items[1]);
        data2 = (int)mp_obj_get_int(quad_data_pins->items[2]);
        data3 = (int)mp_obj_get_int(quad_data_pins->items[3]);

        quad = true;

    } else if (args[ARG_octal_pins].u_obj != mp_const_none) {
        mp_obj_tuple_t *octal_data_pins = MP_OBJ_TO_PTR(args[ARG_octal_pins].u_obj);

        if (octal_data_pins->len != 8) {
            mp_raise_msg_varg(
                &mp_type_ValueError,
                MP_ERROR_TEXT("8 pins are needed for octal SPI not %d"),
                octal_data_pins->len
            );
            return mp_const_none;
        }

        data0 = (int)mp_obj_get_int(octal_data_pins->items[0]);
        data1 = (int)mp_obj_get_int(octal_data_pins->items[1]);
        data2 = (int)mp_obj_get_int(octal_data_pins->items[2]);
        data3 = (int)mp_obj_get_int(octal_data_pins->items[3]);
        data4 = (int)mp_obj_get_int(octal_data_pins->items[4]);
        data5 = (int)mp_obj_get_int(octal_data_pins->items[5]);
        data6 = (int)mp_obj_get_int(octal_data_pins->items[6]);
        data7 = (int)mp_obj_get_int(octal_data_pins->items[7]);

        octal = true;
    } else {
        data0 = (int)args[ARG_mosi].u_int;
        data1 = (int)args[ARG_miso].u_int;
    }

    if (1 <= host && host <= MICROPY_HW_SPI_MAX) {
        self = machine_hw_spi_bus_objs[host - 1];
    } else {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%d) doesn't exist"), host);
        return mp_const_none;
    }

    bool reconfigure = false;

    if (self == NULL) {
        reconfigure = true;
        self = m_new_obj(mp_machine_hw_spi_bus_obj_t);
        self->base.type = &mp_machine_hw_spi_bus_type;
        self->host = host;
        self->deinit = &machine_hw_spi_bus_deinit_internal;
        machine_hw_spi_bus_objs[host - 1] = self;
    } else {
        if ((int)mp_obj_get_int(self->data0) != data0) reconfigure = true;
        if ((int)mp_obj_get_int(self->data1) != data1) reconfigure = true;
        if ((int)mp_obj_get_int(self->sck) != sck) reconfigure = true;
        if ((int)mp_obj_get_int(self->data2) != data2) reconfigure = true;
        if ((int)mp_obj_get_int(self->data3) != data3) reconfigure = true;
        if ((int)mp_obj_get_int(self->data4) != data4) reconfigure = true;
        if ((int)mp_obj_get_int(self->data5) != data5) reconfigure = true;
        if ((int)mp_obj_get_int(self->data6) != data6) reconfigure = true;
        if ((int)mp_obj_get_int(self->data7) != data7) reconfigure = true;
    }

    if (reconfigure) {
        if (self->device_count > 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%d) has active devices, unable to change pins"), host);
            return mp_const_none;
        }

        self->data0 = mp_obj_new_int((mp_int_t)data0);
        self->data1 = mp_obj_new_int((mp_int_t)data1);
        self->sck = mp_obj_new_int((mp_int_t)sck);
        self->data2 = mp_obj_new_int((mp_int_t)data2);
        self->data3 = mp_obj_new_int((mp_int_t)data3);
        self->data4 = mp_obj_new_int((mp_int_t)data4);
        self->data5 = mp_obj_new_int((mp_int_t)data5);
        self->data6 = mp_obj_new_int((mp_int_t)data6);
        self->data7 = mp_obj_new_int((mp_int_t)data7);
        self->dual = dual;
        self->quad = quad;
        self->octal = octal;
    }

    return MP_OBJ_FROM_PTR(self);
}


void mp_machine_hw_spi_bus_initilize(mp_machine_hw_spi_bus_obj_t *bus)
{
    if (bus->state != MP_SPI_STATE_STOPPED) return;

    uint32_t flags = SPICOMMON_BUSFLAG_MASTER;

    if (bus->dual) flags |= SPICOMMON_BUSFLAG_DUAL;
    if (bus->quad) flags |= SPICOMMON_BUSFLAG_QUAD;
    if (bus->octal) flags |= SPICOMMON_BUSFLAG_OCTAL;

    spi_bus_config_t buscfg = {
        .data0_io_num = (int)mp_obj_get_int(bus->data0),
        .data1_io_num = (int)mp_obj_get_int(bus->data1),
        .sclk_io_num = (int)mp_obj_get_int(bus->sck),
        .data2_io_num = (int)mp_obj_get_int(bus->data2),
        .data3_io_num = (int)mp_obj_get_int(bus->data3),
        .data4_io_num = (int)mp_obj_get_int(bus->data4),
        .data5_io_num = (int)mp_obj_get_int(bus->data5),
        .data6_io_num = (int)mp_obj_get_int(bus->data6),
        .data7_io_num = (int)mp_obj_get_int(bus->data7),
        .flags = flags,
        .max_transfer_sz = SPI_LL_DMA_MAX_BIT_LEN / 8
    };

    esp_err_t ret;

#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6
    ret = spi_bus_initialize((spi_host_device_t)bus->host, &buscfg, SPI_DMA_CH_AUTO);
#else
    if (bus->host == SPI2_HOST) {
        ret = spi_bus_initialize((spi_host_device_t)bus->host, &buscfg, 1);
    } else {
        ret = spi_bus_initialize((spi_host_device_t)bus->host, &buscfg, 2);
    }
#endif

    switch (ret) {
        case ESP_ERR_INVALID_ARG:
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%d) invalid configuration"), bus->host);
            return;

        case ESP_ERR_INVALID_STATE:
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%d) sanity check"), bus->host);
            return;
    }

    bus->state = MP_SPI_STATE_STARTED;
}



spi_host_device_t machine_hw_spi_get_host(mp_obj_t in) {
    if (mp_obj_get_type(in) != &mp_machine_hw_spi_bus_type) {
        mp_raise_ValueError(MP_ERROR_TEXT("expecting a SPI object"));
    }
    mp_machine_hw_spi_bus_obj_t *self = (mp_machine_hw_spi_bus_obj_t *)in;
    return (spi_host_device_t)self->host;
}


mp_obj_t machine_hw_spi_device_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    enum { ARG_spi_bus, ARG_freq, ARG_cs, ARG_polarity, ARG_phase, ARG_bits, ARG_firstbit, ARG_dual, ARG_quad, ARG_octal };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_spi_bus,  MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_OBJ },
        { MP_QSTR_freq,     MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_cs,       MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_INT },
        { MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT,  { .u_int = 0 } },
        { MP_QSTR_phase,    MP_ARG_KW_ONLY | MP_ARG_INT,  { .u_int = 0 } },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY | MP_ARG_INT,  { .u_int = 8 } },
        { MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT,  { .u_int = MICROPY_PY_MACHINE_SPI_MSB} },
        { MP_QSTR_dual,     MP_ARG_KW_ONLY | MP_ARG_BOOL, { .u_bool = false } },
        { MP_QSTR_quad,     MP_ARG_KW_ONLY | MP_ARG_BOOL, { .u_bool = false } },
        { MP_QSTR_octal,    MP_ARG_KW_ONLY | MP_ARG_BOOL, { .u_bool = false } }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_machine_hw_spi_device_obj_t *self = m_new_obj(mp_machine_hw_spi_device_obj_t);
    self->base.type = &mp_machine_hw_spi_device_type;

    int cs =  (int)args[ARG_cs].u_int;

    self->spi_bus = MP_OBJ_TO_PTR(args[ARG_spi_bus].u_obj);
    self->cs = mp_obj_new_int((mp_int_t)cs);
    self->bits =  (uint8_t)args[ARG_bits].u_int;
    self->deinit = &machine_hw_spi_device_deinit_callback;
    bool dual = (bool)args[ARG_dual].u_bool;
    bool quad = (bool)args[ARG_quad].u_bool;
    bool octal = (bool)args[ARG_octal].u_bool;


    if (!self->spi_bus->dual) dual = false;
    if (!self->spi_bus->quad) quad = false;
    if (!self->spi_bus->octal) octal = false;

    self->dual = dual;
    self->quad = quad;
    self->octal = octal;

    uint8_t flags = 0;

    if (self->firstbit == MICROPY_PY_MACHINE_SPI_LSB) flags |= SPI_DEVICE_TXBIT_LSBFIRST | SPI_DEVICE_RXBIT_LSBFIRST;

    if (dual || quad || octal) flags |= SPI_DEVICE_HALFDUPLEX;

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = (uint32_t)spi_get_actual_clock(APB_CLK_FREQ, args[ARG_freq].u_int, 0),
        .mode = self->phase | (self->polarity << 1),
        .spics_io_num = cs,
        .queue_size = 2,
        .flags = flags,
        .pre_cb = NULL
    };

    spi_device_handle_t spi_device = NULL;

    if (self->spi_bus->state == MP_SPI_STATE_STOPPED) {
        mp_machine_hw_spi_bus_initilize(self->spi_bus);
    }

    esp_err_t ret = spi_bus_add_device((spi_host_device_t)self->spi_bus->host, &devcfg, &spi_device);
    self->user_data = spi_device;

    switch (ret) {
        case ESP_ERR_INVALID_ARG:
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("invalid configuration"));
            return mp_const_none;

        case ESP_ERR_NO_MEM:
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("out of memory"));
            return mp_const_none;

        case ESP_ERR_NOT_FOUND:
            mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("no free slots"));
            return mp_const_none;
    }

    mp_machine_hw_spi_bus_add_device(self);
    self->active = true;

    return MP_OBJ_FROM_PTR(self);
}


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
    locals_dict, &mp_machine_spi_locals_dict
);


static const mp_rom_map_elem_t machine_spi_bus_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__),       MP_ROM_PTR(&machine_hw_spi_bus_deinit_obj)       }
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
    { MP_ROM_QSTR(MP_QSTR___name__),  MP_OBJ_NEW_QSTR(MP_QSTR_SPI)   },
    { MP_ROM_QSTR(MP_QSTR_Bus),       (mp_obj_t)&mp_machine_hw_spi_bus_type },
    { MP_ROM_QSTR(MP_QSTR_Device),    (mp_obj_t)&mp_machine_hw_spi_device_type }
};

MP_DEFINE_CONST_DICT(machine_spi_locals_dict, machine_spi_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    machine_spi_type,
    MP_QSTR_SPI,
    MP_TYPE_FLAG_NONE,
    locals_dict, &machine_spi_locals_dict
);