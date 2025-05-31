// Copyright (c) 2024 - 2025 Kevin G. Schlosser


// local includes
#include "lcd_bus_task.h"
#include "rotation.h"

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objarray.h"

// esp-idf includes
#ifdef ESP_IDF_VERSION
    #include "sdkconfig.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "esp_lcd_panel_io.h"
#endif


#ifndef _LCD_TYPES_H_
    #define _LCD_TYPES_H_

    #define LCD_UNUSED(x) ((void)x)
    #define LCD_DEFAULT_STACK_SIZE    (5 * 1024)

    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        #define LCD_DEBUG_PRINT(...) mp_printf(&mp_plat_print, __VA_ARGS__);
    #else
        #define LCD_DEBUG_PRINT(...)
    #endif

    #ifdef ESP_IDF_VERSION
        #define LCD_OK                 0
        #define LCD_FAIL               -1
        #define LCD_ERR_NO_MEM         0x101
        #define LCD_ERR_INVALID_ARG    0x102
        #define LCD_ERR_INVALID_STATE  0x103
        #define LCD_ERR_INVALID_SIZE   0x104
        #define LCD_ERR_NOT_SUPPORTED  0x106

        typedef int mp_lcd_err_t;

    #else
        typedef enum {
            LCD_OK = 0,
            LCD_FAIL = -1,
            LCD_ERR_NO_MEM = 0x101,
            LCD_ERR_INVALID_ARG = 0x102,
            LCD_ERR_INVALID_STATE = 0x103,
            LCD_ERR_INVALID_SIZE = 0x104,
            LCD_ERR_NOT_SUPPORTED = 0x106
        } mp_lcd_err_t;
    #endif

    typedef struct _mp_lcd_bus_obj_t mp_lcd_bus_obj_t;

    typedef void (*flush_func_t)(mp_lcd_bus_obj_t *self_in, rotation_data_t *r_data, rotation_data_t *original_r_data, uint8_t *idle_fb, uint8_t last_update);
    typedef bool (*init_func_t)(mp_lcd_bus_obj_t *self_in);

    typedef mp_lcd_err_t (*init_t)(mp_lcd_bus_obj_t *self_in, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, uint8_t cmd_bits, uint8_t param_bits);
    typedef mp_lcd_err_t (*deinit_t)(mp_lcd_bus_obj_t *self_in);

    typedef struct _internal_cb_funcs_t {
        init_t init;
        deinit_t deinit;
    } internal_cb_funcs_t;


    typedef struct _lcd_bufs_t {
        uint8_t *active;
        uint8_t *idle;
        uint8_t *partial;
    } lcd_bufs_t;


    typedef struct _lcd_task_t {
        TaskHandle_t handle;
        lcd_bus_event_t exit;
        lcd_bus_lock_t lock;
    } lcd_task_t;


    typedef struct _lcd_init_t {
        lcd_bus_lock_t lock;
        mp_lcd_err_t err;
        mp_rom_error_text_t err_msg;
        init_func_t init_func;
    } lcd_init_t;


    typedef struct _lcd_tx_data_t {
        lcd_bus_lock_t lock;
        lcd_bus_event_t swap_bufs;
        flush_func_t flush_func;
    } lcd_tx_data_t;


    typedef void (*send_func_t)(mp_lcd_bus_obj_t *self, int cmd, uint8_t *params, size_t params_len);


    typedef struct _lcd_command_t {
        int cmd;
        uint8_t *params;
        size_t params_len;
        uint8_t flush_is_next: 1;
    } lcd_cmd_t;


    typedef struct _lcd_tx_commands_t {
        lcd_bus_lock_t lock;
        lcd_cmd_t **cmds;
        size_t cmds_len;
        send_func_t send_func;
    } lcd_tx_cmds_t;


    struct _mp_lcd_bus_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        mp_obj_array_t *view1;
        mp_obj_array_t *view2;

        uint32_t buffer_flags;

        uint8_t trans_done: 1;
        uint8_t num_lanes: 5;

        lcd_task_t task;
        lcd_init_t init;
        lcd_bufs_t bufs;

        lcd_tx_data_t tx_data;
        lcd_tx_cmds_t tx_cmds;

        rotation_data_t r_data;

        internal_cb_funcs_t internal_cb_funcs;
    };

#endif /* _LCD_TYPES_H_ */
