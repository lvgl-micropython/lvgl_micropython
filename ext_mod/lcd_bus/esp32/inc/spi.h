// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef __SPI_H__
    #define __SPI_H__

    //local_includes
    #include "lcd_types.h"
    #include "../../../../micropy_updates/common/mp_spi_common.h"

    // esp-idf includes
    #include "esp_lcd_panel_io.h"
    #include "driver/spi_common.h"
    #include "driver/spi_master.h"
    #include "hal/spi_types.h"

    // micropython includes
    #include "py/objarray.h"
    #include "mphalport.h"
    #include "py/obj.h"

    typedef struct _mp_lcd_spi_bus_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        mp_obj_array_t *view1;
        mp_obj_array_t *view2;

        lcd_funcs_t funcs;

        lcd_rotate_buffers_t buffers;
        lcd_rotate_data_t rot_data;
        lcd_rotate_task_t task;
        lcd_rotate_task_init_t *task_init;

        // port & bus specific fields below
        esp_lcd_panel_io_handle_t panel_io;
        TaskHandle_t task_handle;

        esp_lcd_panel_io_spi_config_t panel_io_config;
        spi_bus_config_t bus_config;
        esp_lcd_spi_bus_handle_t bus_handle;

        spi_host_device_t host;
        mp_machine_hw_spi_device_obj_t spi_device;
    } mp_lcd_spi_bus_obj_t;

#endif

