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
        mp_obj_t data0;
        mp_obj_t data1;
        mp_obj_t data2;
        mp_obj_t data3;
        mp_obj_t data4;
        mp_obj_t data5;
        mp_obj_t data6;
        mp_obj_t data7;
        bool dual;
        bool quad;
        bool octal;
        uint8_t device_count;
        mp_machine_hw_spi_device_obj_t **devices;
        mp_machine_hw_spi_state_t state;
        const void *user_data;
        void (*deinit)(mp_machine_hw_spi_bus_obj_t *bus);
    };

    struct _mp_machine_hw_spi_device_obj_t {
        mp_obj_base_t base;
        uint32_t freq;
        uint8_t polarity;
        uint8_t phase;
        uint8_t bits;
        uint8_t firstbit;
        bool dual;
        bool quad;
        bool octal;
        bool active;
        mp_obj_t cs;
        mp_machine_hw_spi_bus_obj_t *spi_bus;
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
