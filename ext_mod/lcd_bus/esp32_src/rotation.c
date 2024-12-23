#include "rotation.h"
#include "lcd_types.h"
#include "bus_task.h"
#include "rotate.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_types.h"
#include "rom/ets_sys.h"
#include "esp_system.h"
#include "esp_cpu.h"
#include "esp_task.h"


// micropy includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objarray.h"
#include "py/binary.h"
#include "py/gc.h"
#include "py/stackctrl.h"
#include "mphalport.h"


// stdlib includes
#include <string.h>


int rotation_set_buffers(void *self_in)
{
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)self_in;

    rotation_buffer_t *bufs = &self->rotation->buf;
    
    bool is_dma = (bool)(self->buffer_flags & MALLOC_CAP_DMA);
    // bool is_internal = (bool)(self->buffer_flags & MALLOC_CAP_INTERNAL);

    if (self->view2 == NULL) {
        if (is_dma) {
            bufs->active = self->view1->items;
            bufs->idle = heap_caps_malloc(self->view1->len, self->buffer_flags);
            self->view1->items = heap_caps_malloc(self->view1->len, MALLOC_CAP_INTERNAL);

            if (bufs->idle == NULL) {
                return LCD_ERR_NO_MEM;
            }

            if (self->view1->items == NULL) {
                self->view1->items = heap_caps_malloc(self->view1->len, MALLOC_CAP_SPIRAM);
                if (self->view1->items == NULL) {
                    return LCD_ERR_NO_MEM;
                }
            }
        } else {
            bufs->active = heap_caps_malloc(self->view1->len, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
            bufs->idle = heap_caps_malloc(self->view1->len, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

            if (bufs->active == NULL || bufs->idle == NULL) {
                if (bufs->active != NULL) heap_caps_free(bufs->active);
                bufs->active = heap_caps_malloc(self->view1->len, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
                bufs->idle = heap_caps_malloc(self->view1->len, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
            }

            if (bufs->active == NULL || bufs->idle == NULL) {
                if (bufs->active != NULL) heap_caps_free(bufs->active);
                return LCD_ERR_NO_MEM;
            }
        }
    } else if (is_dma) {
        bufs->active = self->view1->items; 
        bufs->idle = self->view2->items; 
        
        self->view1->items = heap_caps_malloc(self->view1->len, MALLOC_CAP_INTERNAL);
        self->view2->items = heap_caps_malloc(self->view1->len, MALLOC_CAP_INTERNAL);
        
        if (self->view1->items == NULL || self->view2->items == NULL) {
            if (self->view1->items != NULL) heap_caps_free(self->view1->items);
            
            self->view1->items = heap_caps_malloc(self->view1->len, MALLOC_CAP_SPIRAM);
            self->view2->items = heap_caps_malloc(self->view1->len, MALLOC_CAP_SPIRAM);
        }   
        
        if (self->view1->items == NULL || self->view2->items == NULL) {
            if (self->view1->items != NULL) heap_caps_free(self->view1->items);
            return LCD_ERR_NO_MEM;
        }
    } else {
        bufs->active = heap_caps_malloc(self->view1->len, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        bufs->idle = heap_caps_malloc(self->view1->len, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

        if (bufs->active == NULL || bufs->idle == NULL) {
            if (bufs->active != NULL) heap_caps_free(bufs->active);
            bufs->active = heap_caps_malloc(self->view1->len, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
            bufs->idle = heap_caps_malloc(self->view1->len, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
        }

        if (bufs->active == NULL || bufs->idle == NULL) {
            if (bufs->active != NULL) heap_caps_free(bufs->active);
            return LCD_ERR_NO_MEM;
        }
    }

    return LCD_OK;
}

static void rotate_task(void *self_in)
{
    LCD_DEBUG_PRINT("rotate_task - STARTED\n")

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)self_in;

    rotation_t *rotation = self->rotation;
    rotation_task_t *task = &rotation->task;
    rotation_buffer_t *buf = &rotation->buf;
    rotation_data_t *data = &rotation->data;
    rotation_param_data_t *param_data = &rotation->param_data;
    
    mp_lcd_err_t err = rotation->init_func(self_in);
    bus_lock_release(&task->init_lock);
    if (err != LCD_OK) return;

    void *idle_fb;
    bool last_update;
    uint32_t color_size;

    bus_lock_acquire(&task->lock, -1);

    bool exit = bus_event_isset(&task->exit);
    while (!exit) {
        bus_lock_acquire(&task->lock, -1);

        if (buf->partial == NULL) break;

        idle_fb = buf->idle;
        last_update = (bool)data->last_update;
        color_size = rotate(buf->partial, idle_fb, data);

        bus_lock_release(&task->tx_color_lock);

        if (self->callback != mp_const_none) {
            volatile uint32_t sp = (uint32_t)esp_cpu_get_sp();

            void *old_state = mp_thread_get_state();

            mp_state_thread_t ts;
            mp_thread_set_state(&ts);
            mp_stack_set_top((void*)sp);
            mp_stack_set_limit(CONFIG_FREERTOS_IDLE_TASK_STACKSIZE - 1024);
            mp_locals_set(mp_state_ctx.thread.dict_locals);
            mp_globals_set(mp_state_ctx.thread.dict_globals);

            mp_sched_lock();
            gc_lock();

            nlr_buf_t nlr;
            if (nlr_push(&nlr) == 0) {
                mp_call_function_n_kw(self->callback, 0, 0, NULL);
                nlr_pop();
            } else {
                ets_printf("Uncaught exception in IRQ callback handler!\n");
                mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
            }

            gc_unlock();
            mp_sched_unlock();

            mp_thread_set_state(old_state);
        }

        if (last_update && self->rotation->last_update_func != NULL) {
            self->rotation->last_update_func(self_in, idle_fb);
        } else if (self->rotation->last_update_func == NULL) {
            bus_lock_acquire(&self->rotation->task.tx_param_lock, -1);

            uint8_t tx_param_count = param_data->tx_param_count;
            uint8_t i = 0;

            for (;i<tx_param_count;i++) {
                esp_lcd_panel_io_tx_param(
                    self->panel_io_handle.panel_io,
                    param_data->param_cmd[i],
                    param_data->param[i],
                    param_data->param_size[i]
                );

                if (param_data->param_last_cmd[i]) break;
            }

            i++;

            for (uint8_t j=i;j<tx_param_count;j++) {
                param_data->param_cmd[j - i] = param_data->param_cmd[j];
                param_data->param[j - i] = param_data->param[j];
                param_data->param_size[j - i] = param_data->param_size[j];
                param_data->param_last_cmd[j - i] = param_data->param_last_cmd[j];
                param_data->param_cmd[j] = 0;
                param_data->param[j] = NULL;
                param_data->param_size[j] = 0;
                param_data->param_last_cmd[j] = false;
            }
        
            param_data->tx_param_count -= i;

            bus_lock_release(&self->rotation->task.tx_param_lock);

            err = esp_lcd_panel_io_tx_color(
                self->panel_io_handle.panel_io,
                rotation->lcd_cmd,
                idle_fb,
                color_size
            );

            if (err != 0) {
                mp_printf(&mp_plat_print, "esp_lcd_panel_io_tx_color error (%d)\n", err);
            } else {
                bus_event_clear(&task->swap_bufs);
                bus_event_wait(&task->swap_bufs);

                buf->idle = buf->active;
                buf->active = idle_fb;
            }
        }

        exit = bus_event_isset(&task->exit);
    }

    LCD_DEBUG_PRINT(&mp_plat_print, "rotate_task - STOPPED\n")
}


void rotation_task_start(void *self_in)
{
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)self_in;
    rotation_t *rotation = self->rotation;

    bus_lock_init(&rotation->task.lock);
    bus_lock_init(&rotation->task.tx_param_lock);
    bus_lock_init(&rotation->task.tx_color_lock);
    bus_event_init(&rotation->task.exit);
    bus_event_init(&rotation->task.swap_bufs);
    bus_event_set(&rotation->task.swap_bufs);
    bus_lock_init(&rotation->task.init_lock);
    bus_lock_acquire(&rotation->task.init_lock, -1);

    xTaskCreatePinnedToCore(
        rotate_task, "rotate_task", DEFAULT_STACK_SIZE / sizeof(StackType_t),
        self_in, ESP_TASK_PRIO_MAX - 1, &rotation->task.handle, 0);

    bus_lock_acquire(&rotation->task.init_lock, -1);
    bus_lock_release(&rotation->task.init_lock);
}
