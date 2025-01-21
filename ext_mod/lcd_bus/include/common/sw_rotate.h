#ifdef ESP_IDF_VERSION
    #include "esp32/bus_task.h"
#elif defined(MP_PORT_UNIX)
    #include "posix/bus_task.h"
#else
    #include "other_mcus/bus_task.h"

#ifndef __SW_ROTATE_H__
    #define __SW_ROTATE_H__

    typedef struct _mp_lcd_sw_rotation_data_t {
        uint32_t x_start: 16;
        uint32_t y_start: 16;
        uint32_t x_end: 16;
        uint32_t y_end: 16;
        uint32_t dst_width: 16;
        uint32_t dst_height: 16;
        uint32_t bytes_per_pixel: 3;
        uint8_t rotation: 3;
        uint8_t last_update: 1;
        uint8_t rgb565_dither: 1;
    } mp_lcd_sw_rotation_data_t;


    typedef struct _mp_lcd_sw_rotation_buffers_t {
        uint8_t *active;
        uint8_t *idle;
        uint8_t *partial;
    } mp_lcd_sw_rotation_buffers_t;


    typedef struct _mp_lcd_sw_rotation_handles_t {
        mp_lcd_lock_t copy_lock;
        mp_lcd_event_t copy_task_exit;
        mp_lcd_lock_t tx_color_lock;
        mp_lcd_event_t swap_bufs;
        mp_lcd_lock_t init_lock;
        mp_lcd_task_handle_t task_handle;
    } mp_lcd_sw_rotation_handles_t;


    typedef struct _mp_lcd_sw_rotate_tx_param_t {
        int cmd;
        uint8_t *params;
        size_t params_len;
        bool flush_next;
    } mp_lcd_sw_rotate_tx_param_t;

    typedef void (*mp_lcd_tx_param_cb_t)(void* self_in, int cmd, uint8_t *params, size_t params_len);

    typedef struct _mp_lcd_sw_rotate_tx_params_t {
        mp_lcd_sw_rotate_tx_param_t *params;
        uint8_t len;
        mp_lcd_lock_t lock;
        mp_lcd_tx_param_cb_t tx_param_cb;
    } mp_lcd_sw_rotate_tx_params_t;


    typedef bool (*mp_lcd_sw_rotation_init_cb_t)(void *self_in)

    typedef struct _mp_lcd_sw_rotation_init_t {
         mp_lcd_err_t err;
         mp_rom_error_text_t err_msg;
         mp_lcd_sw_rotation_init_cb_t cb;
    } mp_lcd_sw_rotation_init_t;


    typedef void (*mp_lcd_sw_rotate_flush_cb_t)(void *self_in, uint8_t last_update, uint8_t *idle_fb);


    typedef struct _mp_lcd_sw_rotation_t {
         mp_lcd_sw_rotation_init_t init;
         mp_lcd_sw_rotation_handles_t handles;
         mp_lcd_sw_rotation_buffers_t buffers;
         mp_lcd_sw_rotation_data_t data;
         mp_lcd_sw_rotate_flush_cb_t flush_cb;
         mp_lcd_sw_rotate_tx_params_t tx_params;
    } mp_lcd_sw_rotation_t;


    static void mp_lcd_sw_rotation(void *dst, void *src, mp_lcd_sw_rotation_data_t *data);

#endif /* __SW_ROTATE_H__ */