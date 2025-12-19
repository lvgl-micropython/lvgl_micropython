// Copyright (c) 2024 - 2025 Kevin G. Schlosser
// Copyright (c) 2024 - 2025 Viktor Vorobjov

// local includes
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "spi_bus_fast.h"
#include "../../../micropy_updates/common/mp_spi_common.h"

// esp-idf includes
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "soc/gpio_sig_map.h"
#include "soc/spi_pins.h"
#include "soc/soc_caps.h"
#include "rom/gpio.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_heap_caps.h"
#include "hal/spi_types.h"
#include "esp_task.h"
#include "rom/ets_sys.h"  // for ets_printf

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>

#define DEFAULT_STACK_SIZE    (8 * 1024)  // Larger than RGB due to SPI init overhead
#define SPI_FAST_BIT_0 (1 << 0)

// Event management functions
void spi_bus_fast_event_init(spi_bus_fast_event_t *event)
{
    event->handle = xEventGroupCreateStatic(&event->buffer);
}

void spi_bus_fast_event_delete(spi_bus_fast_event_t *event)
{
    xEventGroupSetBits(event->handle, SPI_FAST_BIT_0);
    vEventGroupDelete(event->handle);
}

void spi_bus_fast_event_wait(spi_bus_fast_event_t *event)
{
    xEventGroupWaitBits(event->handle, SPI_FAST_BIT_0, pdFALSE, pdTRUE, portMAX_DELAY);
}

bool spi_bus_fast_event_isset(spi_bus_fast_event_t *event)
{
    return (bool)(xEventGroupGetBits(event->handle) & SPI_FAST_BIT_0);
}

void spi_bus_fast_event_set(spi_bus_fast_event_t *event)
{
    xEventGroupSetBits(event->handle, SPI_FAST_BIT_0);
}

void spi_bus_fast_event_clear(spi_bus_fast_event_t *event)
{
    xEventGroupClearBits(event->handle, SPI_FAST_BIT_0);
}

// Lock management functions
int spi_bus_fast_lock_acquire(spi_bus_fast_lock_t *lock, int32_t wait_ms)
{
    return pdTRUE == xSemaphoreTake(lock->handle, wait_ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS((uint16_t)wait_ms));
}

void spi_bus_fast_lock_release(spi_bus_fast_lock_t *lock)
{
    xSemaphoreGive(lock->handle);
}

void spi_bus_fast_lock_init(spi_bus_fast_lock_t *lock)
{
    lock->handle = xSemaphoreCreateBinaryStatic(&lock->buffer);
    xSemaphoreGive(lock->handle);
}

void spi_bus_fast_lock_delete(spi_bus_fast_lock_t *lock)
{
    xSemaphoreGive(lock->handle);
    vSemaphoreDelete(lock->handle);
}

mp_lcd_err_t spi_fast_del(mp_obj_t obj);
mp_lcd_err_t spi_fast_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
mp_lcd_err_t spi_fast_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
void spi_fast_deinit_callback(mp_machine_hw_spi_device_obj_t *device);

static uint8_t spi_fast_bus_count = 0;
static mp_lcd_spi_bus_fast_obj_t **spi_fast_bus_objs;

void mp_lcd_spi_bus_fast_deinit_all(void)
{
    // we need to copy the existing array to a new one so the order doesn't
    // get all mucked up when objects get removed.
    mp_lcd_spi_bus_fast_obj_t *objs[spi_fast_bus_count];

    for (uint8_t i=0;i<spi_fast_bus_count;i++) {
        objs[i] = spi_fast_bus_objs[i];
    }

    for (uint8_t i=0;i<spi_fast_bus_count;i++) {
        spi_fast_del(MP_OBJ_FROM_PTR(objs[i]));
    }
}

static mp_obj_t mp_lcd_spi_bus_fast_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    LCD_DEBUG_PRINT("SPIBusFast: constructor called!\n");

     enum {
        ARG_spi_bus,
        ARG_dc,
        ARG_freq,
        ARG_cs,
        ARG_dc_low_on_data,
        ARG_lsb_first,
        ARG_cs_high_active,
        ARG_spi_mode,
        ARG_dual,
        ARG_quad,
        ARG_octal
    };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_spi_bus,          MP_ARG_OBJ  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_dc,               MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_freq,             MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED      },
        { MP_QSTR_cs,               MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1       } },
        { MP_QSTR_dc_low_on_data,   MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_lsb_first,        MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_cs_high_active,   MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_spi_mode,         MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0        } },
        { MP_QSTR_dual,             MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_quad,             MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
        { MP_QSTR_octal,            MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false   } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(make_new_args)];
    mp_arg_parse_all_kw_array(
        n_args,
        n_kw,
        all_args,
        MP_ARRAY_SIZE(make_new_args),
        make_new_args,
        args
    );

    // create new object - use mp_obj_malloc like in original spi_bus.c
    mp_lcd_spi_bus_fast_obj_t *self = mp_obj_malloc(mp_lcd_spi_bus_fast_obj_t, &mp_lcd_spi_bus_fast_type);

    mp_machine_hw_spi_bus_obj_t *spi_bus = MP_OBJ_TO_PTR(args[ARG_spi_bus].u_obj);

    self->callback = mp_const_none;

    // Copy all fields from original spi_bus.c
    self->host = (spi_host_device_t)spi_bus->host;
    self->panel_io_handle.panel_io = NULL;
    self->bus_handle = (esp_lcd_spi_bus_handle_t)self->host;

    self->panel_io_config.cs_gpio_num = (int)args[ARG_cs].u_int;
    self->panel_io_config.dc_gpio_num = (int)args[ARG_dc].u_int;
    self->panel_io_config.spi_mode = (int)args[ARG_spi_mode].u_int;
    self->panel_io_config.pclk_hz = (unsigned int)args[ARG_freq].u_int;
    self->panel_io_config.on_color_trans_done = &bus_trans_done_cb;
    self->panel_io_config.user_ctx = self;
    self->panel_io_config.flags.dc_low_on_data = (unsigned int)args[ARG_dc_low_on_data].u_bool;
    self->panel_io_config.flags.lsb_first = (unsigned int)args[ARG_lsb_first].u_bool;
    self->panel_io_config.flags.cs_high_active = (unsigned int)args[ARG_cs_high_active].u_bool;
    self->panel_io_config.flags.sio_mode = (unsigned int)args[ARG_dual].u_bool;
    self->panel_io_config.flags.quad_mode = (unsigned int)args[ARG_quad].u_bool;
    self->panel_io_config.flags.octal_mode = (unsigned int)args[ARG_octal].u_bool;

    if (!spi_bus->dual) self->panel_io_config.flags.sio_mode = 0;
    if (!spi_bus->quad) self->panel_io_config.flags.quad_mode = 0;
    if (!spi_bus->octal) self->panel_io_config.flags.octal_mode = 0;

    // Set up functions - key change! tx_color is now our fast version
    LCD_DEBUG_PRINT("SPIBusFast: setting function pointers\n");
    self->panel_io_handle.del = &spi_fast_del;
    self->panel_io_handle.init = &spi_fast_init;
    self->panel_io_handle.get_lane_count = &spi_fast_get_lane_count;
    self->panel_io_handle.tx_color = &spi_fast_tx_color;  // MAIN CHANGE!
    LCD_DEBUG_PRINT("SPIBusFast: tx_color function set\n");

    self->spi_device.active = true;
    self->spi_device.base.type = &mp_machine_hw_spi_device_type;
    self->spi_device.spi_bus = spi_bus;
    self->spi_device.deinit = &spi_fast_deinit_callback;
    self->spi_device.user_data = self;

    // Initialize new fields for partial updates
    self->active_fb = NULL;
    self->idle_fb = NULL;
    self->partial_buf = NULL;
    self->width = 0;
    self->height = 0;
    self->bytes_per_pixel = 0;
    self->rotation = 0;
    self->last_update = 0;
    self->first_frame_received = 0;
    
    
    // Synchronization will be initialized in spi_fast_init (like RGB)
    
    self->copy_task_handle = NULL;
    self->init_err = LCD_OK;

    LCD_DEBUG_PRINT("host=%d\n", self->host)
    LCD_DEBUG_PRINT("cs_gpio_num=%d\n", self->panel_io_config.cs_gpio_num)
    LCD_DEBUG_PRINT("dc_gpio_num=%d\n", self->panel_io_config.dc_gpio_num)
    LCD_DEBUG_PRINT("spi_mode=%d\n", self->panel_io_config.spi_mode)
    LCD_DEBUG_PRINT("pclk_hz=%i\n", self->panel_io_config.pclk_hz)
    LCD_DEBUG_PRINT("dc_low_on_data=%d\n", self->panel_io_config.flags.dc_low_on_data)
    LCD_DEBUG_PRINT("lsb_first=%d\n", self->panel_io_config.flags.lsb_first)
    LCD_DEBUG_PRINT("cs_high_active=%d\n", self->panel_io_config.flags.cs_high_active)
    LCD_DEBUG_PRINT("dual=%d\n", self->panel_io_config.flags.sio_mode)
    LCD_DEBUG_PRINT("quad=%d\n", self->panel_io_config.flags.quad_mode)
    LCD_DEBUG_PRINT("octal=%d\n", self->panel_io_config.flags.octal_mode)
    
    LCD_DEBUG_PRINT("SPIBusFast: constructor completed successfully\n");

    return MP_OBJ_FROM_PTR(self);
}

void spi_fast_deinit_callback(mp_machine_hw_spi_device_obj_t *device)
{
    mp_lcd_spi_bus_fast_obj_t *self = (mp_lcd_spi_bus_fast_obj_t *)device->user_data;
    spi_fast_del(MP_OBJ_FROM_PTR(self));
}

mp_lcd_err_t spi_fast_del(mp_obj_t obj)
{
    LCD_DEBUG_PRINT("spi_fast_del(self)\n")

    mp_lcd_spi_bus_fast_obj_t *self = (mp_lcd_spi_bus_fast_obj_t *)obj;

    // Stop copy task if it's running
    if (self->copy_task_handle != NULL) {
        spi_bus_fast_event_set(&self->copy_task_exit);
        vTaskDelete(self->copy_task_handle);
        self->copy_task_handle = NULL;
    }
    
    // Free frame buffers
    if (self->active_fb != NULL) {
        heap_caps_free(self->active_fb);
        self->active_fb = NULL;
    }
    
    if (self->idle_fb != NULL) {
        heap_caps_free(self->idle_fb);
        self->idle_fb = NULL;
    }

    if (self->panel_io_handle.panel_io != NULL) {
        mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle.panel_io);
        if (ret != ESP_OK) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
            return ret;
        }

        self->panel_io_handle.panel_io = NULL;

        if (self->view1 != NULL) {
            heap_caps_free(self->view1->items);
            self->view1->items = NULL;
            self->view1->len = 0;
            self->view1 = NULL;
            LCD_DEBUG_PRINT("spi_free_framebuffer(self, buf=1)\n")
        }

        if (self->view2 != NULL) {
            heap_caps_free(self->view2->items);
            self->view2->items = NULL;
            self->view2->len = 0;
            self->view2 = NULL;
            LCD_DEBUG_PRINT("spi_free_framebuffer(self, buf=1)\n")
        }

        uint8_t i= 0;
        for (;i<spi_fast_bus_count;i++) {
            if (spi_fast_bus_objs[i] == self) {
                spi_fast_bus_objs[i] = NULL;
                break;
            }
        }

        for (uint8_t j=i + 1;j<spi_fast_bus_count;j++) {
            spi_fast_bus_objs[j - i + 1] = spi_fast_bus_objs[j];
        }

        spi_fast_bus_count--;
        spi_fast_bus_objs = m_realloc(spi_fast_bus_objs, spi_fast_bus_count * sizeof(mp_lcd_spi_bus_fast_obj_t *));

        mp_machine_hw_spi_bus_remove_device(&self->spi_device);
        self->spi_device.active = false;

        if (self->spi_device.spi_bus->device_count == 0) {
            self->spi_device.spi_bus->deinit(self->spi_device.spi_bus);
        }

        // Clean up synchronization
        spi_bus_fast_lock_delete(&self->copy_lock);
        spi_bus_fast_lock_delete(&self->tx_color_lock);
        spi_bus_fast_lock_delete(&self->init_lock);
        spi_bus_fast_event_delete(&self->copy_task_exit);

        return ret;
    } else {
        return LCD_FAIL;
    }
}

mp_lcd_err_t spi_fast_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits)
{
    LCD_DEBUG_PRINT("spi_fast_init(self, width=%i, height=%i, bpp=%i, buffer_size=%lu, rgb565_byte_swap=%i, cmd_bits=%i, param_bits=%i)\n", width, height, bpp, buffer_size, (uint8_t)rgb565_byte_swap, cmd_bits, param_bits)
    mp_lcd_spi_bus_fast_obj_t *self = (mp_lcd_spi_bus_fast_obj_t *)obj;

    if (self->panel_io_handle.panel_io != NULL) {
        return LCD_FAIL;
    }

    if (self->spi_device.spi_bus->state == MP_SPI_STATE_STOPPED) {
        mp_machine_hw_spi_bus_initilize(self->spi_device.spi_bus);
    }

    if (bpp == 16) {
        self->rgb565_byte_swap = rgb565_byte_swap;
    } else {
        self->rgb565_byte_swap = false;
    }

    // Save display parameters for partial updates
    self->width = width;
    self->height = height;
    self->bytes_per_pixel = bpp / 8;

    // Allocate two full frame buffers for double buffering
    uint32_t fb_size = width * height * self->bytes_per_pixel;
    
    self->active_fb = (uint8_t*)heap_caps_malloc(fb_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (self->active_fb == NULL) {
        return LCD_FAIL;
    }
    // Do NOT initialize to zero - leave uninitialized for performance
    
    self->idle_fb = (uint8_t*)heap_caps_malloc(fb_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (self->idle_fb == NULL) {
        heap_caps_free(self->active_fb);
        self->active_fb = NULL;
        return LCD_FAIL;
    }
    // Do NOT initialize to zero - leave uninitialized for performance

    self->panel_io_config.trans_queue_depth = 10;
    self->panel_io_config.lcd_cmd_bits = (int)cmd_bits;
    self->panel_io_config.lcd_param_bits = (int)param_bits;

    LCD_DEBUG_PRINT("lcd_cmd_bits=%d\n", self->panel_io_config.lcd_cmd_bits)
    LCD_DEBUG_PRINT("lcd_param_bits=%d\n", self->panel_io_config.lcd_param_bits)
    LCD_DEBUG_PRINT("rgb565_byte_swap=%i\n",  (uint8_t)self->rgb565_byte_swap)
    LCD_DEBUG_PRINT("trans_queue_depth=%i\n", (uint8_t)self->panel_io_config.trans_queue_depth)

    // Panel IO will be created in copy task (like in RGB driver)
    mp_machine_hw_spi_bus_add_device(&self->spi_device);

    // Initialize synchronization (exactly like RGB)
    spi_bus_fast_lock_init(&self->copy_lock);
    spi_bus_fast_lock_init(&self->tx_color_lock);
    spi_bus_fast_event_init(&self->copy_task_exit);
    spi_bus_fast_lock_init(&self->init_lock);
    
    // Start copy task (exactly like RGB)
    spi_bus_fast_lock_acquire(&self->init_lock, -1);
    LCD_DEBUG_PRINT("width=%i\n", width);
    LCD_DEBUG_PRINT("height=%i\n", height);
    LCD_DEBUG_PRINT("bytes_per_pixel=%d\n", self->bytes_per_pixel);
    LCD_DEBUG_PRINT("rgb565_byte_swap=%d\n", self->rgb565_byte_swap);
    LCD_DEBUG_PRINT("SPIBusFast: creating copy task...\n");
    BaseType_t task_result = xTaskCreatePinnedToCore(
        spi_bus_fast_copy_task,
        "spi_fast_copy", 
        DEFAULT_STACK_SIZE / sizeof(StackType_t),
        self,
        ESP_TASK_PRIO_MAX - 1, 
        &self->copy_task_handle,
        tskNO_AFFINITY  // Do not pin to specific CPU core
    );
    
    LCD_DEBUG_PRINT("SPIBusFast: xTaskCreatePinnedToCore returned: %d\n", task_result);
    
    if (task_result != pdPASS) {
        LCD_DEBUG_PRINT("SPIBusFast: FAILED to create copy task!\n");
        spi_bus_fast_lock_release(&self->init_lock);
        return LCD_ERR_NO_MEM;
    }
    
    LCD_DEBUG_PRINT("SPIBusFast: copy task created, waiting for init...\n");
    spi_bus_fast_lock_acquire(&self->init_lock, -1);
    spi_bus_fast_lock_release(&self->init_lock);
    spi_bus_fast_lock_delete(&self->init_lock);
    
    if (self->init_err != LCD_OK) {
        LCD_DEBUG_PRINT("SPIBusFast: copy task initialization failed: %d\n", self->init_err);
        return self->init_err;
    }
    
    LCD_DEBUG_PRINT("SPIBusFast: copy task initialized successfully\n");

    // add the new bus ONLY after successfull initilization of the bus
    spi_fast_bus_count++;
    spi_fast_bus_objs = m_realloc(spi_fast_bus_objs, spi_fast_bus_count * sizeof(mp_lcd_spi_bus_fast_obj_t *));
    spi_fast_bus_objs[spi_fast_bus_count - 1] = self;

    return LCD_OK;
}

mp_lcd_err_t spi_fast_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
{
    mp_lcd_spi_bus_fast_obj_t *self = (mp_lcd_spi_bus_fast_obj_t *)obj;

    if (self->panel_io_config.flags.sio_mode) {
        *lane_count = 2;
    } else if (self->panel_io_config.flags.quad_mode) {
        *lane_count = 4;
    } else if (self->panel_io_config.flags.octal_mode) {
        *lane_count = 8;
    } else {
        *lane_count = 1;
    }

    LCD_DEBUG_PRINT("spi_fast_get_lane_count(self) -> %i\n", (uint8_t)(*lane_count))

    return LCD_OK;
}

// KEY FUNCTION! tx_color with copy task support
mp_lcd_err_t spi_fast_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, 
                                int x_start, int y_start, int x_end, int y_end, 
                                uint8_t rotation, bool last_update)
{
    mp_lcd_spi_bus_fast_obj_t *self = (mp_lcd_spi_bus_fast_obj_t *)obj;
    
    // Acquire tx_color lock to prevent race conditions
    spi_bus_fast_lock_acquire(&self->tx_color_lock, -1);
    
    // Save partial update parameters - ORDER IS CRITICAL!
    // The copy task reads these atomically, so we must set them in correct order:
    // 1. Set coordinates and rotation first
    // 2. Set last_update flag 
    // 3. Set partial_buf LAST (this signals the task that data is ready)
    self->x_start = x_start;
    self->y_start = y_start;
    self->x_end = x_end;
    self->y_end = y_end;
    self->rotation = rotation;
    self->last_update = (uint8_t)last_update;  // EXACTLY LIKE RGB - BEFORE partial_buf!
    self->partial_buf = (uint8_t *)color;      // LAST - signal for task
    
    LCD_DEBUG_PRINT("spi_fast_tx_color: set partial_buf=%p, x=%d-%d, y=%d-%d\n", 
                    self->partial_buf, x_start, x_end, y_start, y_end)
    
    // wake copy task
    LCD_DEBUG_PRINT("spi_fast_tx_color: waking copy task, partial_buf=%p, size=%d\n", self->partial_buf, color_size)
    
    
    // Lock ordering is critical for thread safety:
    // - tx_color_lock prevents race conditions during parameter updates
    // - copy task will release tx_color_lock after reading all parameters
    // - copy_lock controls task wake-up
    // DO NOT release tx_color_lock here - task will release after reading!
    
    // Release copy_lock to wake up copy task
    spi_bus_fast_lock_release(&self->copy_lock);
    
    return LCD_OK;
}

mp_obj_t mp_spi_bus_fast_get_host(mp_obj_t obj)
{
    mp_lcd_spi_bus_fast_obj_t *self = (mp_lcd_spi_bus_fast_obj_t *)obj;

    LCD_DEBUG_PRINT("mp_spi_bus_fast_get_host(self) -> %i\n", (uint8_t)self->host)
    return mp_obj_new_int((uint8_t)self->host);
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_spi_bus_fast_get_host_obj, mp_spi_bus_fast_get_host);

static const mp_rom_map_elem_t mp_lcd_spi_bus_fast_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_host),             MP_ROM_PTR(&mp_spi_bus_fast_get_host_obj)    },
    { MP_ROM_QSTR(MP_QSTR_get_lane_count),       MP_ROM_PTR(&mp_lcd_bus_get_lane_count_obj)       },
    { MP_ROM_QSTR(MP_QSTR_allocate_framebuffer), MP_ROM_PTR(&mp_lcd_bus_allocate_framebuffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_free_framebuffer),     MP_ROM_PTR(&mp_lcd_bus_free_framebuffer_obj)     },
    { MP_ROM_QSTR(MP_QSTR_register_callback),    MP_ROM_PTR(&mp_lcd_bus_register_callback_obj)    },
    { MP_ROM_QSTR(MP_QSTR_tx_param),             MP_ROM_PTR(&mp_lcd_bus_tx_param_obj)             },
    { MP_ROM_QSTR(MP_QSTR_tx_color),             MP_ROM_PTR(&mp_lcd_bus_tx_color_obj)             },
    { MP_ROM_QSTR(MP_QSTR_rx_param),             MP_ROM_PTR(&mp_lcd_bus_rx_param_obj)             },
    { MP_ROM_QSTR(MP_QSTR_init),                 MP_ROM_PTR(&mp_lcd_bus_init_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_deinit),               MP_ROM_PTR(&mp_lcd_bus_deinit_obj)               },
    { MP_ROM_QSTR(MP_QSTR___del__),              MP_ROM_PTR(&mp_lcd_bus_deinit_obj)               },
};

static MP_DEFINE_CONST_DICT(mp_lcd_spi_bus_fast_locals_dict, mp_lcd_spi_bus_fast_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_spi_bus_fast_type,
    MP_QSTR_SPIBusFast,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_spi_bus_fast_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_spi_bus_fast_locals_dict
);

