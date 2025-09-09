// Copyright (c) 2024 - 2025 Kevin G. Schlosser
// Copyright (c) 2024 - 2025 Viktor Vorobjov

#ifndef _ESP32_SPI_BUS_FAST_H_
    #define _ESP32_SPI_BUS_FAST_H_

    //local_includes
    #include "lcd_types.h"
    #include "../../../micropy_updates/common/mp_spi_common.h"

    // esp-idf includes
    #include "esp_lcd_panel_io.h"
    #include "esp_heap_caps.h"
    #include "driver/spi_common.h"
    #include "driver/spi_master.h"
    #include "hal/spi_types.h"

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"
    #include "freertos/event_groups.h"
    #include "freertos/idf_additions.h"

    // micropython includes
    #include "mphalport.h"
    #include "py/obj.h"
    #include "py/objarray.h"

    typedef struct _spi_bus_fast_lock_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    } spi_bus_fast_lock_t;

    typedef struct _spi_bus_fast_event_t {
        EventGroupHandle_t handle;
        StaticEventGroup_t buffer;
    } spi_bus_fast_event_t;

    typedef struct _mp_lcd_spi_bus_fast_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        mp_obj_array_t *view1;
        mp_obj_array_t *view2;

        uint32_t buffer_flags;

        bool trans_done;
        bool rgb565_byte_swap;

        lcd_panel_io_t panel_io_handle;
        
        // Оригинальные поля из spi_bus.h
        esp_lcd_panel_io_spi_config_t panel_io_config;
        spi_bus_config_t bus_config;
        esp_lcd_spi_bus_handle_t bus_handle;
        spi_host_device_t host;
        mp_machine_hw_spi_device_obj_t spi_device;

        // Partial updates specific fields
        uint8_t *active_fb;
        uint8_t *idle_fb;
        uint8_t *partial_buf;

        int x_start;
        int y_start;
        int x_end;
        int y_end;
        uint16_t width;
        uint16_t height;
        uint8_t rotation: 2;
        uint8_t bytes_per_pixel: 2;
        uint8_t last_update: 1;
        uint8_t first_frame_received: 1;

        // Task synchronization
        spi_bus_fast_lock_t copy_lock;
        spi_bus_fast_event_t copy_task_exit;
        spi_bus_fast_lock_t tx_color_lock;
        spi_bus_fast_lock_t init_lock;

        TaskHandle_t copy_task_handle;

        // ДИАГНОСТИКА - счетчики без printf  
        volatile uint32_t task_alive_counter;
        volatile uint32_t task_loop_counter;
        volatile uint32_t copy_function_count;     // сколько раз вызвана copy_partial_to_full_buffer
        volatile uint32_t send_function_count;     // сколько раз вызвана send_full_buffer_chunked  
        volatile uint32_t send_error_count;        // сколько ошибок ESP-IDF API
        // Основные диагностические поля убраны - используем ets_printf из task

        mp_lcd_err_t init_err;
        mp_rom_error_text_t init_err_msg;

    } mp_lcd_spi_bus_fast_obj_t;

    void spi_bus_fast_event_init(spi_bus_fast_event_t *event);
    void spi_bus_fast_event_delete(spi_bus_fast_event_t *event);
    bool spi_bus_fast_event_isset(spi_bus_fast_event_t *event);
    void spi_bus_fast_event_set(spi_bus_fast_event_t *event);
    void spi_bus_fast_event_clear(spi_bus_fast_event_t *event);
    void spi_bus_fast_event_wait(spi_bus_fast_event_t *event);

    int  spi_bus_fast_lock_acquire(spi_bus_fast_lock_t *lock, int32_t wait_ms);
    void spi_bus_fast_lock_release(spi_bus_fast_lock_t *lock);
    void spi_bus_fast_lock_init(spi_bus_fast_lock_t *lock);
    void spi_bus_fast_lock_delete(spi_bus_fast_lock_t *lock);

    void spi_bus_fast_copy_task(void *self_in);
    
    // SPIBusFast functions
    mp_lcd_err_t spi_fast_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, 
                                   int x_start, int y_start, int x_end, int y_end, 
                                   uint8_t rotation, bool last_update);

    extern const mp_obj_type_t mp_lcd_spi_bus_fast_type;

    extern void mp_lcd_spi_bus_fast_deinit_all(void);

#endif /* _ESP32_SPI_BUS_FAST_H_ */