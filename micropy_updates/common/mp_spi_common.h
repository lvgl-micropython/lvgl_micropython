// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include <stdbool.h>

#include "py/obj.h"
#include "py/runtime.h"

#ifndef __MP_SPI_COMMON_H__
    #define __MP_SPI_COMMON_H__

    typedef enum _mp_machine_hw_spi_state_t {
        MP_SPI_STATE_STOPPED,
        MP_SPI_STATE_STARTED,
        MP_SPI_STATE_SENDING
    } mp_machine_hw_spi_state_t;

    typedef struct _mp_machine_hw_spi_bus_obj_t mp_machine_hw_spi_bus_obj_t;
    typedef struct _mp_machine_hw_spi_device_obj_t mp_machine_hw_spi_device_obj_t;

    struct _mp_machine_hw_spi_bus_obj_t {
        mp_obj_base_t base;
        uint8_t host;
        mp_obj_t sck;
        mp_obj_t data0; // mosi
        mp_obj_t data1; // miso
        mp_obj_t data2; // quadwp
        mp_obj_t data3; // quadhd
        // octal pins
        mp_obj_t data4;
        mp_obj_t data5;
        mp_obj_t data6;
        mp_obj_t data7;

        // flags if these types of SPI is available at the bus
        uint8_t dual: 1;
        uint8_t quad: 1;
        uint8_t octal: 1;
        uint8_t device_count: 5;
        mp_machine_hw_spi_device_obj_t **devices;
        mp_machine_hw_spi_state_t state;

        // stores the spi bus specific to a port
        const void *user_data;
        void (*deinit)(mp_machine_hw_spi_bus_obj_t *bus);
    };

    struct _mp_machine_hw_spi_device_obj_t {
        mp_obj_base_t base;
        uint32_t freq;
        uint8_t polarity: 1;
        uint8_t phase: 1;
        uint8_t bits;
        uint8_t firstbit: 1;
        uint8_t dual: 1;
        uint8_t quad: 1;
        uint8_t octal: 1;
        uint8_t active: 1;
        mp_obj_t cs;
        uint8_t cs_high_active: 1;
        mp_machine_hw_spi_bus_obj_t *spi_bus;
        uint8_t mem_addr_size;
        uint32_t mem_addr;
        uint8_t mem_addr_set: 1;
        uint8_t reduce_max_trans: 1;
        void *user_data;
        void (*deinit)(mp_machine_hw_spi_device_obj_t *device);
    };

    void mp_machine_hw_spi_bus_initilize(mp_machine_hw_spi_bus_obj_t *bus);
    void mp_machine_hw_spi_bus_add_device(mp_machine_hw_spi_device_obj_t *device);
    void mp_machine_hw_spi_bus_remove_device(mp_machine_hw_spi_device_obj_t *device);

    extern const mp_obj_type_t mp_machine_hw_spi_device_type;
    extern const mp_obj_type_t mp_machine_hw_spi_bus_type;

    void mp_machine_hw_spi_bus_deinit_all(void);

#endif /* __MP_SPI_COMMON_H__ */
