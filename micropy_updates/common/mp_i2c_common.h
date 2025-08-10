// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include <stdbool.h>

#include "py/obj.h"
#include "py/runtime.h"

#ifndef __MP_I2C_COMMON_H__
    #define __MP_I2C_COMMON_H__

    typedef struct _mp_machine_hw_i2c_bus_obj_t mp_machine_hw_i2c_bus_obj_t;
    typedef struct _mp_machine_hw_i2c_device_obj_t mp_machine_hw_i2c_device_obj_t;

    typedef struct _machine_hw_i2c_obj_t {

} machine_hw_i2c_obj_t;


    struct _mp_machine_hw_i2c_bus_obj_t {
        mp_obj_base_t base;

        uint8_t port;
        mp_obj_t scl;
        mp_obj_t sda;
        uint8_t trans_queue_depth;

        uint8_t device_count: 5;
        mp_machine_hw_i2c_device_obj_t **devices;

        uint8_t soft_i2c: 1;

        void *user_data;
        void (*deinit)(mp_machine_hw_i2c_bus_obj_t *bus);
    };

    struct _mp_machine_hw_i2c_device_obj_t {
        mp_obj_base_t base;
        uint32_t freq;
        uint16_t timeout;
        uint8_t addr;
        uint8_t mem_addr_size;

        mp_machine_hw_i2c_bus_obj_t *i2c_bus;

        void *user_data;
        void (*deinit)(mp_machine_hw_i2c_device_obj_t *device);
    };

    void mp_machine_hw_i2c_bus_initilize(mp_machine_hw_i2c_bus_obj_t *bus);
    void mp_machine_hw_i2c_bus_add_device(mp_machine_hw_i2c_device_obj_t *device);
    void mp_machine_hw_i2c_bus_remove_device(mp_machine_hw_i2c_device_obj_t *device);

    extern const mp_obj_type_t mp_machine_hw_i2c_device_type;
    extern const mp_obj_type_t mp_machine_hw_i2c_bus_type;

    void mp_machine_hw_i2c_bus_deinit_all(void);

    mp_obj_t mp_machine_i2c_device_writevto(size_t n_args, const mp_obj_t *args);

#endif /* __MP_SPI_COMMON_H__ */