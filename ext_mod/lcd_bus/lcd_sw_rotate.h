// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "lcd_types.h"
#include "lcd_os.h"

#ifndef __LCD_SW_ROTATE_H__
    #define __LCD_SW_ROTATE_H__

    #define LCD_ROTATION_0    (0)
    #define LCD_ROTATION_90   (1)
    #define LCD_ROTATION_180  (2)
    #define LCD_ROTATION_270  (3)

    typedef struct _lcd_rotate_buffers_t {
        uint8_t *active_fb;
        uint8_t *idle_fb;
        uint8_t *partial_buf;
        lcd_lock_t copy_lock;
        lcd_event_t swap_bufs;
    } lcd_rotate_buffers_t;


    typedef struct _lcd_rotate_data_t {
        int x_start;
        int y_start;
        int x_end;
        int y_end;

        uint16_t dst_width;
        uint16_t dst_height;

        uint8_t rotation: 2;
        uint8_t bytes_per_pixel: 2;
        uint8_t last_update: 1;
        uint8_t dither: 1;
        uint8_t byteswap: 1;
        uint8_t sw_rotate: 1;
        int write_cmd;
        size_t write_buf_size;
    } lcd_rotate_data_t;

    typedef struct _lcd_rotate_task_t {
        lcd_lock_t tx_color_lock;
        lcd_event_t copy_task_exit;
        mp_lcd_err_t (*write_display)(mp_lcd_bus_obj_t *self_in, int cmd, int x_start, int y_start, int x_end, int y_end, uint8_t *buf, size_t buf_size);
    ) lcd_rotate_task_t;

    typedef struct _lcd_rotate_task_init_t {
        lcd_lock_t init_lock;
        mp_lcd_err_t init_err;
        mp_rom_error_text_t init_err_msg;
        bool (*init_func)(mp_lcd_bus_obj_t *self_in);
    } lcd_rotate_task_init_t;


    typedef struct _lcd_tx_cmd_t {
        bool flush_is_next;
        size_t param_size;
        void *params;
        int cmd;
    } lcd_tx_cmd_t;

    typedef struct _lcd_tx_cmds_t {
        uint16_t num_cmds;
        lcd_tx_cmd_t **cmds;
        lcd_lock_t lock;
        void (*write_cmd)(mp_lcd_bus_obj_t *self_in, int cmd, void *param, size_t param_size);
    } lcd_tx_cmds_t;


    void rotate(void *dst, void *src,
                uint32_t x_start, uint32_t y_start,
                uint32_t x_end, uint32_t y_end,
                uint32_t dst_width, uint32_t dst_height,
                uint32_t bytes_per_pixel, uint8_t rotate,
                uint8_t dither, uint8_t byteswap);