/*
|  Name   |   Bit 0    |   Bit 0    |   Bit 1    |   Bit 1    |  Reset    | Color |
|         | Duration 1 | Duration 2 | Duration 1 | Duration 2 | Duration  | Order |
|:--------|:----------:|:----------:|:----------:|:----------:|:---------:|:-----:|
| APA105  |    300     |    -900    |     600    |    -600    |  -80000   |  GRB  |
| APA109  |    300     |    -900    |     600    |    -600    |  -80000   |  GRB  |
| SK6805  |    300     |    -900    |     600    |    -600    |  -80000   |  GRB  |
| SK6812  |    300     |    -900    |     600    |    -600    |  -80000   |  GRB  |
| SK6818  |    300     |    -900    |     600    |    -600    |  -80000   |  GRB  |
| WS2813  |    300     |    -300    |     750    |    -300    |  -300000  |  GRB  |
| APA104  |    350     |   -1360    |    1360    |    -350    |  -24000   |  RGB  |
| SK6822  |    350     |   -1360    |    1360    |    -350    |  -50000   |  RGB  |
| WS2812  |    350     |    -800    |     700    |    -600    |  -50000   |  GRB  |
| WS2818A |    220     |    -580    |     580    |    -220    |  -280000  |  RGB  |
| WS2818B |    220     |    -580    |     580    |    -220    |  -280000  |  RGB  |
| WS2851  |    220     |    -580    |     580    |    -220    |  -280000  |  RGB  |
| WS2815B |    220     |    -580    |     580    |    -220    |  -280000  |  RGB  |
| WS2815  |    220     |    -580    |     580    |    -220    |  -280000  |  RGB  |
| WS2811  |    220     |    -580    |     580    |    -220    |  -280000  |  RGB  |
| WS2814  |    220     |    -580    |     580    |    -220    |  -280000  |  RGB  |
| WS2818  |    220     |    -750    |     750    |    -220    |  -300000  |  RGB  |
| WS2816A |    220     |    -580    |     580    |    -580    |  -280000  |  GRB  |
| WS2816B |    200     |    -800    |     520    |    -480    |  -280000  |  GRB  |
| WS2816C |    200     |    -800    |     520    |    -480    |  -280000  |  GRB  |
| WS2812B |    400     |    -850    |     800    |    -450    |  -50000   |  GRB  |
| SK6813  |    240     |    -800    |     740    |    -200    |  -80000   |  GRB  |
*/

#ifndef __LED_STRIP_H__
    #define __LED_STRIP_H__

    #include "driver/rmt_tx.h"
    #include "led_strip_encoder.h"

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"
    #include "freertos/idf_additions.h"

    // micropython includes
    #include "py/obj.h"
    #include "py/runtime.h"
    #include "py/objarray.h"

    typedef enum _led_strip_ic_t {
        CUSTOM = 0,

        APA105 = 1,
        APA109 = 1,
        SK6805 = 1,
        SK6812 = 1,
        SK6818 = 1,

        WS2813 = 2,
        APA104 = 3,
        SK6822 = 4,
        WS2812 = 5,

        WS2818A = 6,
        WS2818B = 6,
        WS2851 = 6,
        WS2815B = 6,
        WS2815 = 6,
        WS2811 = 6,
        WS2814 = 6,

        WS2818 = 7,
        WS2816A = 8,

        WS2816B = 9,
        WS2816C = 9,

        WS2812B = 10,
        SK6813 = 11,

    } led_strip_ic_t;

    typedef struct {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    } lock_t;

    typedef struct _mp_led_strip_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        mp_obj_array_t **buffers;
        uint8_t buffer_count: 4;

        mp_obj_array_t **buffers_in_queue;
        uint8_t queue_buffer_count: 4;

        uint16_t num_pixels: 12;
        uint16_t is_init: 1;
        uint16_t bpp: 3;

        rmt_encoder_handle_t encoder_handle;
        rmt_channel_handle_t channel_handle;
        led_strip_encoder_config_t encoder_config;
        rmt_tx_channel_config_t channel_config;
        lock_t tx_lock;
        lock_t task_lock;
        TaskHandle_t task_handle;

    } mp_led_strip_obj_t;

    extern const mp_obj_type_t mp_led_strip_type;

#endif