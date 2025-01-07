// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include <stdbool.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#ifndef __MP_I2C_COMMON_H__
    #define __MP_I2C_COMMON_H__

    #define I2C_MAX_DEVICES  (31)

    typedef struct _mp_machine_hw_i2c_bus_obj_t mp_machine_hw_i2c_bus_obj_t;
    typedef struct _mp_machine_hw_i2c_device_obj_t mp_machine_hw_i2c_device_obj_t;

    typedef struct _i2c_bus_lock_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    } i2c_bus_lock_t;


    struct _mp_machine_hw_i2c_bus_obj_t {
        mp_obj_base_t base;
        i2c_port_t port : 8;
        gpio_num_t scl : 8;
        gpio_num_t sda : 8;
        uint8_t active : 1;
        uint8_t device_count : 5;
        mp_machine_hw_i2c_device_obj_t **devices;
        uint32_t freq;
        uint32_t timeout_us;
        uint8_t use_locks : 1;
        uint8_t pullup : 1;
        i2c_bus_lock_t lock;
    };

    struct _mp_machine_hw_i2c_device_obj_t {
        mp_obj_base_t base;
        uint16_t device_id;
        uint8_t reg_bits;
        mp_machine_hw_i2c_bus_obj_t *bus;
    };

    extern const mp_obj_type_t mp_machine_hw_i2c_device_type;
    extern const mp_obj_type_t mp_machine_hw_i2c_bus_type;

    void mp_machine_hw_i2c_bus_deinit_all(void);

#endif /* __MP_I2C_COMMON_H__ */

