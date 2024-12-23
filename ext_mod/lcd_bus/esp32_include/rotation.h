#include "bus_task.h"

// micropy includes
#include "py/obj.h"
#include "py/runtime.h"
#include "mphalport.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "rotate.h"


#ifndef __ROTATION_H__
    #define __ROTATION_H__

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
        int code;
        mp_rom_error_text_t msg;
    } rotation_init_err_t;

    typedef struct _rotation_param_data_t {
        uint8_t tx_param_count;
        int param_cmd[24];
        void *param[24];
        size_t param_size[24];
        bool param_last_cmd[24];
    } rotation_param_data_t;


    typedef int (*init_func_cb_t)(void *self_in);
    typedef void (*last_update_cb_t)(void *self_in, void *idle_buf);

    typedef struct _rotation_t {
        rotation_task_t task;
        rotation_buffer_t buf;
        rotation_data_t data;
        rotation_init_err_t init_err;
        rotation_param_data_t param_data;
        int lcd_cmd;

        init_func_cb_t init_func;
        last_update_cb_t last_update_func;

    } rotation_t;

    void rotation_task_start(void *self_in);
    int rotation_set_buffers(void *self_in);

#endif
