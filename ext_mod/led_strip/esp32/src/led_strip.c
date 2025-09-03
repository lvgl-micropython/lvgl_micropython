
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/idf_additions.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "esp_heap_caps.h"
#include "rom/ets_sys.h"
#include "esp_system.h"
#include "esp_cpu.h"


#include "led_strip.h"
#include "led_strip_encoder.h"

#include "py/obj.h"
#include "py/runtime.h"
#include "py/objarray.h"
#include "py/gc.h"
#include "py/stackctrl.h"
#include "mphalport.h"


#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)


static void _lock_release_from_isr(lock_t *lock)
{
   xSemaphoreGiveFromISR(lock->handle, pdFALSE);
}


static void _lock_init(lock_t *lock)
{
    lock->handle = xSemaphoreCreateBinaryStatic(&lock->buffer);
    xSemaphoreGive(lock->handle);
}

static void _lock_delete(lock_t *lock)
{
    vSemaphoreDelete(lock->handle);
}


static void _lock_acquire(lock_t *lock)
{
    xSemaphoreTake(lock->handle, portMAX_DELAY);
}


static void _lock_release(lock_t *lock)
{
    xSemaphoreGive(lock->handle);
}



static bool get_led_strip_timing(led_strip_ic_t ic, *led_strip_encoder_config_t timing)
{
    switch(ic) {
        case APA105:
        case APA109:
        case SK6805:
        case SK6812:
        case SK6818:
            timing->bit0_duration0 = 300;
            timing->bit0_duration1 = -900;
            timing->bit1_duration0 = 600;
            timing->bit1_duration1 = -600;
            timing->reset_duration = -80000;
            timing->color_order = GRB;
            break;

        case WS2813:
            timing->bit0_duration0 = 300;
            timing->bit0_duration1 = -300;
            timing->bit1_duration0 = 750;
            timing->bit1_duration1 = -300;
            timing->reset_duration = -300000;
            timing->color_order = GRB;
            break;

        case APA104:
            timing->bit0_duration0 = 350;
            timing->bit0_duration1 = -1360;
            timing->bit1_duration0 = 1360;
            timing->bit1_duration1 = -350;
            timing->reset_duration = -24000;
            timing->color_order = RGB;
            break;

        case SK6822:
            timing->bit0_duration0 = 350;
            timing->bit0_duration1 = -1360;
            timing->bit1_duration0 = 1360;
            timing->bit1_duration1 = -350;
            timing->reset_duration = -50000;
            timing->color_order = RGB;
            break;

        case WS2812:
            timing->bit0_duration0 = 350;
            timing->bit0_duration1 = -800;
            timing->bit1_duration0 = 700;
            timing->bit1_duration1 = -600;
            timing->reset_duration = -50000;
            timing->color_order = GRB;
            break;

        case WS2818A:
        case WS2818B:
        case WS2851:
        case WS2815B:
        case WS2815:
        case WS2811:
        case WS2814:
            timing->bit0_duration0 = 220;
            timing->bit0_duration1 = -580;
            timing->bit1_duration0 = 580;
            timing->bit1_duration1 = -220;
            timing->reset_duration = -280000;
            timing->color_order = RGB;
            break;

        case WS2818:
            timing->bit0_duration0 = 220;
            timing->bit0_duration1 = -750;
            timing->bit1_duration0 = 750;
            timing->bit1_duration1 = -220;
            timing->reset_duration = -300000;
            timing->color_order = RGB;
            break;

        case WS2816A:
            timing->bit0_duration0 = 200;
            timing->bit0_duration1 = -580;
            timing->bit1_duration0 = 580;
            timing->bit1_duration1 = -580;
            timing->reset_duration = -280000;
            timing->color_order = GRB;
            break;


        case WS2816B:
        case WS2816C:
            timing->bit0_duration0 = 200;
            timing->bit0_duration1 = -800;
            timing->bit1_duration0 = 520;
            timing->bit1_duration1 = -480;
            timing->reset_duration = -280000;
            timing->color_order = GRB;
            break;

        case WS2812B:
            timing->bit0_duration0 = 400;
            timing->bit0_duration1 = -850;
            timing->bit1_duration0 = 800;
            timing->bit1_duration1 = -450;
            timing->reset_duration = -50000;
            timing->color_order = GRB;
            break;

        case SK6813:
            timing->bit0_duration0 = 240;
            timing->bit0_duration1 = -800;
            timing->bit1_duration0 = 740;
            timing->bit1_duration1 = -200;
            timing->reset_duration = -80000;
            timing->color_order = GRB;
            break;

        default:
            return false;
    };

    return true;
}


static bool tx_done_callback(rmt_channel_handle_t tx_chan, const rmt_tx_done_event_data_t *edata, void *user_ctx)
{
    mp_led_strip_obj_t *self = (mp_led_strip_obj_t *)user_ctx;
    
    
    volatile uint32_t sp = (uint32_t)esp_cpu_get_sp();

    // Calling micropython from ISR
    // See: https://github.com/micropython/micropython/issues/4895
    void *old_state = mp_thread_get_state();

    mp_state_thread_t ts; // local thread state for the ISR
    mp_thread_set_state(&ts);
    mp_stack_set_top((void*)sp); // need to include in root-pointer scan
    mp_stack_set_limit(CONFIG_FREERTOS_IDLE_TASK_STACKSIZE - 1024); // tune based on ISR thread stack size
    mp_locals_set(mp_state_ctx.thread.dict_locals); // use main thread's locals
    mp_globals_set(mp_state_ctx.thread.dict_globals); // use main thread's globals

    mp_sched_lock(); // prevent VM from switching to another MicroPython thread
    gc_lock(); // prevent memory allocation

    _lock_release_from_isr(&self->task_lock);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t args[1];
        args[0] = MP_OBJ_FROM_PTR(self->buffers_in_queue[0]);
        _lock_release_from_isr(&self->task_lock);
        mp_call_function_n_kw(self->callback, 1, 0, args);
        nlr_pop();
    } else {
        ets_printf("Uncaught exception in IRQ callback handler!\n");
        mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));  // changed to &mp_plat_print to fit this context
    }

    gc_unlock();
    mp_sched_unlock();

    mp_thread_set_state(old_state);
    mp_hal_wake_main_task_from_isr();

    return false;
}


void led_strip_queue_task(void *self_in)
{
    mp_led_strip_obj_t *self = (mp_led_strip_obj_t *)self_in;

    while 1 {
        _lock_acquire(&self->task_lock);
        _lock_acquire(&self->tx_lock);
        self->queue_buffer_count--;
        memcpy(self->buffers_in_queue, self->buffers_in_queue + 1, sizeof(mp_obj_array_t *) * self->queue_buffer_count);
        self->buffers_in_queue = realloc(self->buffers_in_queue, sizeof(mp_obj_array_t *) * self->queue_buffer_count);
        _lock_release(&self->tx_lock);
    }
}


static mp_obj_t led_strip_make_new(const mp_obj_type_t *type, size_t n_args,
                                        size_t n_kw, const mp_obj_t *all_args)
{
     enum { ARG_gpio, ARG_ic, ARG_num_pixels, ARG_tx_done_cb, ARG_bit0_dur0, ARG_bit0_dur1,
            ARG_bit1_dur0, ARG_bit0_dur1, ARG_reset_dur, ARG_color_order, ARG_byte_order };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_gpio,             MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
        { MP_QSTR_ic,               MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
        { MP_QSTR_num_pixels,       MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
        { MP_QSTR_tx_done_cb,       MP_ARG_OBJ | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
        { MP_QSTR_bpp,              MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = 3   } },
        { MP_QSTR_bit0_dur0,        MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = 0   } },
        { MP_QSTR_bit0_dur1,        MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = 0   } },
        { MP_QSTR_bit1_dur0,        MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = 0   } },
        { MP_QSTR_bit1_dur1,        MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = 0   } },
        { MP_QSTR_reset_dur,        MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = 0   } },
        { MP_QSTR_color_order,      MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = 1   } },
        { MP_QSTR_byte_order,       MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = 1   } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(make_new_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args,
                              MP_ARRAY_SIZE(make_new_args), make_new_args, args);

    // create new object
    mp_led_strip_obj_t *self = m_new_obj(mp_led_strip_obj_t);
    self->base.type = &mp_led_strip_type;

    self->callback = args[ARG_tx_done_cb].u_obj;

    rmt_tx_channel_config_t channel_config = self->channel_config;

    channel_config.gpio_num = (int)args[ARG_gpio].u_int;
    channel_config.clk_src = RMT_CLK_SRC_DEFAULT;
    channel_config.resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ;
    channel_config.mem_block_symbols = 64;
    channel_config.intr_priority = 1;
    channel_config.flags.with_dma = 1;

    uint8_t bpp = 0;

    if (color_order == LED_STRIP_RGBW || color_order == LED_STRIP_GRBW) bpp = 4;
    else bpp = 3;

    self->num_pixels = (uint16_t)args[ARG_num_pixels].u_int;
    self->bpp = (uint16_t)bpp;
    
    led_strip_encoder_config_t encoder_config = self->encoder_config;
    encoder_config.resolution = RMT_LED_STRIP_RESOLUTION_HZ;

    led_strip_ic_t ic = (led_strip_ic_t)args[ARG_ic].u_int;

    led_strip_color_order_t color_order = (led_strip_color_order_t)args[ARG_color_order].u_int;

    _lock_init(&self->tx_lock);
    _lock_init(&self->task_lock);
    _lock_acquire(&self->task_lock);

    if (ic == CUSTOM) {
        encoder_config.bit0_duration0 = (int32_t)args[ARG_bit0_dur0].u_int;
        encoder_config.bit0_duration1 = (int32_t)args[ARG_bit0_dur1].u_int;
        encoder_config.bit1_duration0 = (int32_t)args[ARG_bit1_dur0].u_int;
        encoder_config.bit1_duration1 = (int32_t)args[ARG_bit1_dur1].u_int;
        encoder_config.reset_duration = (int32_t)args[ARG_reset_dur].u_int;
        encoder_config.color_order = color_order;
        encoder_config.byte_order = (led_strip_byte_order_t)args[ARG_byte_order].u_int;
    } else if (!get_led_strip_timing(ic, &encoder_config)) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(get_led_strip_timing)"), ic);
    }

    return MP_OBJ_FROM_PTR(self);
}


static mp_obj_t led_strip_tx(mp_obj_t self_in, mp_obj_t buf_in)
{
    mp_led_strip_obj_t *self = (mp_led_strip_obj_t *)MP_OBJ_TO_PTR(self_in);

    esp_err_t err;

    if (!self->is_init) {
        self->is_init = 1;
        
        self->channel_config.trans_queue_depth = self->buffer_count;
        self->channel_handle = NULL;

        err = rmt_new_tx_channel(&self->channel_config, &self->channel_handle);
        if (err != ESP_OK) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(rmt_new_tx_channel)"), err);
            return mp_const_none;
        }
        
        rmt_tx_event_callbacks_t callback;
        callback.on_trans_done = &tx_done_callback;

        err = rmt_tx_register_event_callbacks(self->channel_handle, &callback, self);
        if (err != ESP_OK) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(rmt_tx_register_event_callbacks)"), err);
            return mp_const_none;
        }
        
        self->encoder_handle = NULL;

        err = rmt_new_led_strip_encoder(&self->encoder_config, &self->encoder_handle);
        if (err != ESP_OK) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(rmt_new_led_strip_encoder)"), err);
            return mp_const_none;
        }
        
        err = rmt_enable(self->channel_handle);
        if (err != ESP_OK) {
            mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(rmt_enable)"), err);
            return mp_const_none;
        }

        xTaskCreatePinnedToCore(rgb_bus_copy_task, "rgb_task",
                                1024 / sizeof(StackType_t),
                                self, ESP_TASK_PRIO_MAX - 1, &self->task_handle, 1);
    }
    
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);

    _lock_acquire(&self->tx_lock);
    self->queue_buffer_count++;
    self->buffers_in_queue = realloc(self->buffers_in_queue, sizeof(mp_obj_array_t *) * self->queue_buffer_count);
    self->buffers_in_queue[self->queue_buffer_count - 1] = (mp_obj_array_t *)MP_OBJ_TO_PTR(buf_in);
    _lock_release(&self->tx_lock);

    rmt_transmit_config_t tx_config = {
        .loop_count = 0, // no transfer loop
    };

    err = rmt_transmit(self->channel_handle, self->encoder_handle, bufinfo.buf, bufinfo.len, &tx_config));
    if (err != ESP_OK) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%d(rmt_transmit)"), err);
    }
    
    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_2(led_strip_tx_obj, led_strip_tx);


static mp_obj_t led_strip_free_buffer(mp_obj_t self_in, mp_obj_t buf_in)
{
    mp_led_strip_obj_t *self = (mp_led_strip_obj_t *)self_in;
    mp_obj_array_t *array_buf = (mp_obj_array_t *)MP_OBJ_TO_PTR(buf_in);

    _lock_acquire(&self->tx_lock);

    for (uint8_t c;c<self->queue_buffer_count;c++) {
        if (self->buffers_in_queue[c] == array_buf) {
            _lock_release(&self->tx_lock);
            mp_raise_msg(
               &mp_type_MemoryError,
               MP_ERROR_TEXT("buffer is in queue to be transmitted")
            );
            return mp_const_none;

        }
    }
    _lock_release(&self->tx_lock);

    for (uint8_t i=0;i<self->buffer_count;i++) {
        if (self->buffers[i] == array_buf) {
            self->buffer_count--;
            memcpy(self->buffers + i, self->buffers + i + 1, sizeof(mp_obj_array_t *) * self->buffer_count - i);
            self->buffers = realloc(self->buffers, sizeof(mp_obj_array_t *) * self->buffer_count);
            heap_caps_free(array_buf->items);
            array_buf->items = NULL;
            array_buf->len = 0;
            break;
        }
    }

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_2(led_strip_free_buffer_obj, led_strip_free_buffer);


static mp_obj_t led_strip_allocate_buffer(mp_obj_t self_in)
{
    mp_led_strip_obj_t *self = (mp_led_strip_obj_t *)self_in;

    if (self->is_init) {
        mp_raise_msg(
           &mp_type_MemoryError,
           MP_ERROR_TEXT("Driver has already started, not able to add more frame buffers")
       );
    }

    size_t size = (size_t)(self->num_pixels * self->bpp);

    void *buf = heap_caps_calloc(1, size, MALLOC_CAP_DMA);

    if (buf == NULL) {
       mp_raise_msg_varg(
           &mp_type_MemoryError,
           MP_ERROR_TEXT("Not enough memory available (%d)"),
           size
       );
       return mp_const_none;
    }

    mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, buf));
    view->typecode |= 0x80; // used to indicate writable buffer

    self->buffer_count++;
    self->buffers = realloc(self->buffers, sizeof(mp_obj_array_t *) * self->buffer_count);

    self->buffers[self->buffer_count - 1] = view;

    return MP_OBJ_FROM_PTR(view);
}

static MP_DEFINE_CONST_FUN_OBJ_1(led_strip_allocate_buffer_obj, led_strip_allocate_buffer);


static const mp_rom_map_elem_t led_strip_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_allocate_buffer), MP_ROM_PTR(&led_strip_allocate_buffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_free_buffer),     MP_ROM_PTR(&led_strip_free_buffer_obj)     },
    { MP_ROM_QSTR(MP_QSTR_tx),              MP_ROM_PTR(&led_strip_tx_obj)              }
};

MP_DEFINE_CONST_DICT(led_strip_locals_dict, led_strip_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_led_strip_type,
    MP_QSTR_LEDStrip,
    MP_TYPE_FLAG_NONE,
    make_new, led_strip_make_new,
    locals_dict, (mp_obj_dict_t *)&led_strip_locals_dict
);


static const mp_rom_map_elem_t led_strip_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_led_strip) },
    { MP_ROM_QSTR(MP_QSTR_LEDStrip), MP_ROM_PTR(&mp_led_strip_type)     },
    { MP_ROM_QSTR(MP_QSTR_RGB),      MP_ROM_INT(LED_STRIP_RGB)          },
    { MP_ROM_QSTR(MP_QSTR_GRB),      MP_ROM_INT(LED_STRIP_GRB)          },
    { MP_ROM_QSTR(MP_QSTR_RGBW),     MP_ROM_INT(LED_STRIP_RGBW)         },
    { MP_ROM_QSTR(MP_QSTR_GRBW),     MP_ROM_INT(LED_STRIP_GRBW)         },
    { MP_ROM_QSTR(MP_QSTR_LSB),      MP_ROM_INT(LED_STRIP_LSB)          },
    { MP_ROM_QSTR(MP_QSTR_MSB),      MP_ROM_INT(LED_STRIP_MSB)          },
    { MP_ROM_QSTR(MP_QSTR_CUSTOM),   MP_ROM_INT(CUSTOM)                 },
    { MP_ROM_QSTR(MP_QSTR_APA105),   MP_ROM_INT(APA105)                 },
    { MP_ROM_QSTR(MP_QSTR_APA109),   MP_ROM_INT(APA109)                 },
    { MP_ROM_QSTR(MP_QSTR_SK6805),   MP_ROM_INT(SK6805)                 },
    { MP_ROM_QSTR(MP_QSTR_SK6812),   MP_ROM_INT(SK6812)                 },
    { MP_ROM_QSTR(MP_QSTR_SK6818),   MP_ROM_INT(SK6818)                 },
    { MP_ROM_QSTR(MP_QSTR_WS2813),   MP_ROM_INT(WS2813)                 },
    { MP_ROM_QSTR(MP_QSTR_APA104),   MP_ROM_INT(APA104)                 },
    { MP_ROM_QSTR(MP_QSTR_SK6822),   MP_ROM_INT(SK6822)                 },
    { MP_ROM_QSTR(MP_QSTR_WS2812),   MP_ROM_INT(WS2812)                 },
    { MP_ROM_QSTR(MP_QSTR_WS2818A),  MP_ROM_INT(WS2818A)                },
    { MP_ROM_QSTR(MP_QSTR_WS2818B),  MP_ROM_INT(WS2818B)                },
    { MP_ROM_QSTR(MP_QSTR_WS2851),   MP_ROM_INT(WS2851)                 },
    { MP_ROM_QSTR(MP_QSTR_WS2815B),  MP_ROM_INT(WS2815B)                },
    { MP_ROM_QSTR(MP_QSTR_WS2815),   MP_ROM_INT(WS2815)                 },
    { MP_ROM_QSTR(MP_QSTR_WS2811),   MP_ROM_INT(WS2811)                 },
    { MP_ROM_QSTR(MP_QSTR_WS2814),   MP_ROM_INT(WS2814)                 },
    { MP_ROM_QSTR(MP_QSTR_WS2818),   MP_ROM_INT(WS2818)                 },
    { MP_ROM_QSTR(MP_QSTR_WS2816A),  MP_ROM_INT(WS2816A)                },
    { MP_ROM_QSTR(MP_QSTR_WS2816B),  MP_ROM_INT(WS2816B)                },
    { MP_ROM_QSTR(MP_QSTR_WS2816C),  MP_ROM_INT(WS2816C)                },
    { MP_ROM_QSTR(MP_QSTR_WS2812B),  MP_ROM_INT(WS2812B)                },
    { MP_ROM_QSTR(MP_QSTR_SK6813),   MP_ROM_INT(SK6813)                 }
};

static MP_DEFINE_CONST_DICT(led_strip_globals, led_strip_globals_table);


const mp_obj_module_t led_strip_module = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&led_strip_globals,
};

MP_REGISTER_MODULE(MP_QSTR_led_strip, led_strip_module);