#include "bus_task.h"
#include "lcd_types.h"

#include "py/misc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#ifndef __ROTATION_H__
    #define __ROTATION_H__

#if LCD_RGB_OPTIMUM_FB_SIZE
    typedef struct _rotation_optimum_t {
        uint16_t flush_count;
        uint8_t sample_count;
        uint8_t curr_index;
        uint16_t *samples;
        bus_lock_t lock;
    } rotation_optimum_t;
#endif


    typedef struct _rotation_task_t {
        bus_lock_t lock;
        bus_event_t exit;
        bus_lock_t tx_color_lock;
        bus_event_t swap_bufs;
        bus_lock_t init_lock;
        bus_lock_t tx_param_lock;

        TaskHandle_t handle;
    } rotation_task_t;


    typedef struct _rotation_buffer_t {
        void *active;
        void *idle;
        void *partial;
    } rotation_buffer_t;


    typedef struct _rotation_init_err_t {
        mp_lcd_err_t code;
        mp_rom_error_text_t msg;
    } rotation_init_err_t;


    typedef struct _rotation_data_t {
        int x_start;
        int y_start;
        int x_end;
        int y_end;

        uint16_t width;
        uint16_t height;

        uint8_t rotation: 2;
        uint8_t bytes_per_pixel: 2;
        uint8_t last_update: 1;

        uint8_t tx_param_count;
        int param_cmd[24];
        void *param[24];
        size_t param_size[24];
        bool param_last_cmd[24]

    } rotation_data_t;


    typedef mp_lcd_err_t (*init_func_cb_t)(void *self_in);


    typedef struct _rotation_t {
        rotation_task_t task;
        rotation_buffer_t buf;
        rotation_data_t data;
        rotation_init_err_t init_err;
        int lcd_cmd;

        init_func_cb_t init_func;


    #if LCD_RGB_OPTIMUM_FB_SIZE
        rotation_optimum_t optimum;
    #endif
    } rotation_t;

    void rotation_task_start(void *self_in);
    mp_lcd_err_t rotation_set_buffers(void *self_in);

    uint32_t rotate(void *src, void *dst, rotation_data_t *data);


#endif
