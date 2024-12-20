#include "rotation.h"
#include "lcd_types.h"
#include "bus_task.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_types.h"
#include "rom/ets_sys.h"
#include "esp_system.h"
#include "esp_cpu.h"


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


#define ROTATION_0    (0)
#define ROTATION_90   (1)
#define ROTATION_180  (2)
#define ROTATION_270  (3)


static void rotate0(uint8_t *src, uint8_t *dst, rotation_data_t *data);
static void rotate_8bpp(uint8_t *src, uint8_t *dst, rotation_data_t *data);
static void rotate_16bpp(uint16_t *src, uint16_t *dst, rotation_data_t *data);
static void rotate_24bpp(uint8_t *src, uint8_t *dst, rotation_data_t *data);
static void rotate_32bpp(uint32_t *src, uint32_t *dst, rotation_data_t *data);


mp_lcd_err_t rotation_set_buffers(void *self_in)
{

    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)self_in;

    rotation_buffer_t *bufs = &self->rotation->buf;
    
    bool is_dma = (bool)(self->buffer_flags ^ MALLOC_CAP_DMA);
    bool is_internal = (bool)(self->buffer_flags ^ MALLOC_CAP_INTERNAL);

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
    rotation_init_err_t *init_err = &rotation->init_err;
    rotation_data_t *data = &rotation->data;
    
    if (rotation->init_func(self_in) != LCD_OK) return;
    bus_lock_release(&task->init_lock);

    void *idle_fb;
    uint32_t color_size;
    mp_lcd_err_t err;

    bus_lock_acquire(&task->lock, -1);

    bool exit = bus_event_isset(&task->exit);
    while (!exit) {
        bus_lock_acquire(&task->lock, -1);

        if (buf->partial == NULL) break;

        idle_fb = buf->idle;

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

        bus_lock_acquire(&self->rotation->task.tx_param_lock);

        uint8_t tx_param_count = self->rotation->data.tx_param_count;
        uint8_t i = 0;

        for (;i<tx_param_count;i++) {
            esp_lcd_panel_io_tx_param(
                self->panel_io_handle.panel_io,
                data->param_cmd[i],
                data->param[i],
                data->param_size[i]
            );

            if (data->param_last_cmd[i]) break;
        }

        i++;

        for (uint8_t j=i, j<tx_param_count;j++) {
            data->param_cmd[j - i] = data->param_cmd[j];
            data->param[j - i] = data->param[j];
            data->param_size[j - i] = data->param_size[j];
            data->param_last_cmd[j - i] = data->param_last_cmd[j];
            data->param_cmd[j] = 0
            data->param[j] = NULL
            data->param_size[j] = 0
            data->param_last_cmd[j] = false;
        }
        
        self->rotation->data.tx_param_count -= i;

        bus_lock_release(&self->rotation->task.tx_param_lock);

        err = esp_lcd_panel_io_tx_color(
            self->panel_io_handle.panel_io,
            rotation->lcd_cmd,
            idle_fb,
            color_size
        );

        if (err != 0) {
            mp_printf(&mp_plat_print, "esp_lcd_panel_io_tx_color error (%d)\n", ret);
        } else {
            bus_event_clear(&task->swap_bufs);
            bus_event_wait(&task->swap_bufs);

            buf->idle = buf->active;
            buf->active = idle_fb;
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


__attribute__((always_inline))
static inline void copy_8bpp(uint8_t *from, uint8_t *to)
{
    *to++ = *from++;
}


__attribute__((always_inline))
static inline void copy_16bpp(uint16_t *from, uint16_t *to)
{
    *to++ = *from++;
}


__attribute__((always_inline))
static inline void copy_24bpp(uint8_t *from, uint8_t *to)
{
    *to++ = *from++;
    *to++ = *from++;
    *to++ = *from++;
}


__attribute__((always_inline))
static inline void copy_32bpp(uint32_t *from, uint32_t *to)
{
    *to++ = *from++;
}


uint32_t rotate(void *src, void *dst, rotation_data_t *data)
{
    rotation_data_t rot_data;

    if (data->rotation != ROTATION_0) data->y_end += 1;

    if (data->rotation == ROTATION_90 || data->rotation == ROTATION_270) {
        rot_data.x_start = MIN(data->x_start, data->height);
        rot_data.x_end = MIN(data->x_end, data->height);
        rot_data.y_start = MIN(data->y_start, data->width);
        rot_data.y_end = MIN(data->y_end, data->width);
    } else {
        rot_data.x_start = MIN(data->x_start, data->width);
        rot_data.x_end = MIN(data->x_end, data->width);
        rot_data.y_start = MIN(data->y_start, data->height);
        rot_data.y_end = MIN(data->y_end, data->height);
    }

    rot_data.height = data->height;
    rot_data.width = data->width;
    rot_data.rotation = data->rotation;
    rot_data.bytes_per_pixel = data->bytes_per_pixel;

    if (data->rotation == ROTATION_0) {
        rotate0(src, dst, &rot_data);
    } else {
        if (data->bytes_per_pixel == 1) rotate_8bpp(src, dst, &rot_data);
        else if (data->bytes_per_pixel == 2) rotate_16bpp(src, dst, &rot_data);
        else if (data->bytes_per_pixel == 3) rotate_24bpp(src, dst, &rot_data);
        else if (data->bytes_per_pixel == 4) rotate_32bpp(src, dst, &rot_data);
    }

    return (rot_data.x_end - rot_data.x_start + 1) * (rot_data.y_end - rot_data.y_start) * rot_data.bytes_per_pixel;
}


void rotate0(uint8_t *src, uint8_t *dst, rotation_data_t *data)
{
    uint32_t x_start = data->x_start;
    uint32_t y_start = data->y_start;
    uint32_t x_end = data->x_end;
    uint32_t y_end = data->y_end;

    uint32_t dst_width = data->width;
    uint32_t dst_height = data->height;

    uint8_t bytes_per_pixel = data->bytes_per_pixel;

    dst += ((y_start * dst_width + x_start) * bytes_per_pixel);

    if(x_start == 0 && x_end == (dst_width - 1)){
        memcpy(dst, src, dst_width * (y_end - y_start + 1) * bytes_per_pixel);
    } else {
        uint32_t src_bytes_per_line = (x_end - x_start + 1) * bytes_per_pixel;
        uint32_t dst_bytes_per_line = dst_width * bytes_per_pixel;

        for(uint32_t y = y_start; y < y_end; y++){
            memcpy(dst, src, src_bytes_per_line);
            dst += dst_bytes_per_line;
            src += src_bytes_per_line;
        }
    }
}

void rotate_8bpp(uint8_t *src, uint8_t *dst, rotation_data_t *data)
{
    uint32_t i;
    uint32_t j;

    uint32_t x_start = data->x_start;
    uint32_t y_start = data->y_start;
    uint32_t x_end = data->x_end;
    uint32_t y_end = data->y_end;

    uint32_t dst_width = data->width;
    uint32_t dst_height = data->height;

    uint32_t src_bytes_per_line = x_end - x_start + 1;
    uint32_t offset = y_start * src_bytes_per_line + x_start;

    switch (data->rotation) {
        case ROTATION_90:
            dst_height--;

            for (uint32_t y = y_start; y < y_end; y++) {
                i = y * src_bytes_per_line - offset;

                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_8bpp(src + (i + x),
                            dst + (((dst_height - x) * dst_width) + y));
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case ROTATION_180:
            LCD_UNUSED(j);
            LCD_UNUSED(src_bytes_per_line);
            dst_height--;
            offset = dst_width - 1 - x_start;

            for (uint32_t y = y_start; y < y_end; y++) {
                i = ((dst_height - y) * dst_width) + offset;

                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_8bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case ROTATION_270:
            for (uint32_t y = y_start; y < y_end; y++) {
                i = y * src_bytes_per_line - offset;
                j = dst_width - 1 - y;

                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_8bpp(src + (i + x),
                            dst + ((x * dst_width) + j));
                }
            }
            break;

        default:
            LCD_UNUSED(i);
            LCD_UNUSED(j);
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);
            LCD_UNUSED(x_start);
            LCD_UNUSED(y_start);
            LCD_UNUSED(x_end);
            LCD_UNUSED(y_end);
            LCD_UNUSED(dst_width);
            LCD_UNUSED(dst_height);
            break;
    }

}


void rotate_16bpp(uint16_t *src, uint16_t *dst, rotation_data_t *data)
{
    uint32_t i;
    uint32_t j;

    uint32_t x_start = data->x_start;
    uint32_t y_start = data->y_start;
    uint32_t x_end = data->x_end;
    uint32_t y_end = data->y_end;

    uint32_t dst_width = data->width;
    uint32_t dst_height = data->height;

    uint32_t src_bytes_per_line = x_end - x_start + 1;
    uint32_t offset = y_start * src_bytes_per_line + x_start;

    switch (data->rotation) {
        case ROTATION_90:
            dst_height--;

            for (uint32_t y = y_start; y < y_end; y++) {
                i = y * src_bytes_per_line - offset;

                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_16bpp(src + (i + x),
                            dst + (((dst_height - x) * dst_width) + y));
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case ROTATION_180:
            LCD_UNUSED(j);
            LCD_UNUSED(src_bytes_per_line);

            dst_height--;
            offset = dst_width - 1 - x_start;

            for (uint32_t y = y_start; y < y_end; y++) {
                i = ((dst_height - y) * dst_width) + offset;

                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_16bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case ROTATION_270:
            for (uint32_t y = y_start; y < y_end; y++) {
                i = y * src_bytes_per_line - offset;
                j = dst_width - 1 - y;

                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_16bpp(src + (i + x), dst + (x * dst_width + j));
                }
            }
            break;

        default:
            LCD_UNUSED(i);
            LCD_UNUSED(j);
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);
            LCD_UNUSED(x_start);
            LCD_UNUSED(y_start);
            LCD_UNUSED(x_end);
            LCD_UNUSED(y_end);
            LCD_UNUSED(dst_width);
            LCD_UNUSED(dst_height);
            break;
    }
}


void rotate_24bpp(uint8_t *src, uint8_t *dst, rotation_data_t *data)
{
    uint32_t i;
    uint32_t j;

    uint32_t x_start = data->x_start;
    uint32_t y_start = data->y_start;
    uint32_t x_end = data->x_end;
    uint32_t y_end = data->y_end;

    uint32_t dst_width = data->width;
    uint32_t dst_height = data->height;

    uint32_t src_bytes_per_line = (x_end - x_start + 1) * 3;
    uint32_t offset = y_start * src_bytes_per_line + x_start * 3;

    switch (data->rotation) {

        case ROTATION_90:
            dst_height--;

            for (uint32_t y = y_start; y < y_end; y++) {
                i = y * src_bytes_per_line - offset

                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_24bpp(src + (i + (x * 3)),
                            dst + ((((dst_height - x) * dst_width) + y) * 3));
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case ROTATION_180:
            LCD_UNUSED(j);
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);

            dst_height--;
            offset = dst_width - 1 - x_start;

            for (int y = y_start; y < y_end; y++) {
                i = (((dst_height - y) * dst_width) + offset) * 3;

                for (size_t x = x_start; x < x_end; x++) {
                    copy_24bpp(src, dst + i);
                    src += 3;
                    i -= 3;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case ROTATION_270:
            for (uint32_t y = y_start; y < y_end; y++) {
                i = y * src_bytes_per_line - offset;
                j = dst_width - 1 - y;

                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_24bpp(src + (i + (x * 3)),
                            dst + (((x * dst_width) + j) * 3));
                }
            }
            break;

        default:
            LCD_UNUSED(i);
            LCD_UNUSED(j);
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);
            LCD_UNUSED(x_start);
            LCD_UNUSED(y_start);
            LCD_UNUSED(x_end);
            LCD_UNUSED(y_end);
            LCD_UNUSED(dst_width);
            LCD_UNUSED(dst_height);
            break;
    }
}


void rotate_32bpp(uint32_t *src, uint32_t *dst, rotation_data_t *data)
{
    uint32_t i;

    uint32_t x_start = data->x_start;
    uint32_t y_start = data->y_start;
    uint32_t x_end = data->x_end;
    uint32_t y_end = data->y_end;

    uint32_t dst_width = data->width;
    uint32_t dst_height = data->height;

    uint32_t src_bytes_per_line = x_end - x_start + 1;
    uint32_t offset = y_start * src_bytes_per_line + x_start;

    switch (data->rotation) {
        case ROTATION_90:
            dst_height--;

            for (uint32_t y = y_start; y < y_end; y++) {
                i = y * src_bytes_per_line - offset

                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_32bpp(src + (i + x),
                            dst + (((dst_height - x) * dst_width) + y));
                }
            }
            break;

        // MIRROR_X MIRROR_Y
        case ROTATION_180:
            LCD_UNUSED(src_bytes_per_line);
            dst_height--;
            offset = dst_width - 1 - x_start;


            for (uint32_t y = y_start; y < y_end; y++) {
                i = (dst_height - y) * dst_width + offset;

                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_32bpp(src, dst + i);
                    src++;
                    i--;
                }
            }
            break;

        // SWAP_XY   MIRROR_X
        case ROTATION_270:
            uint32_t dst_width_minus_one = dst_width - 1;

            for (uint32_t y = y_start; y < y_end; y++) {
                i = y * src_bytes_per_line - offset;
                dst_height = dst_width_minus_one - y;

                for (uint32_t x = x_start; x < x_end; x++) {
                    copy_32bpp(src + (i + x), dst + (x * dst_width + dst_height));
                }
            }
            break;

        default:
            LCD_UNUSED(i);
            LCD_UNUSED(src_bytes_per_line);
            LCD_UNUSED(offset);
            LCD_UNUSED(x_start);
            LCD_UNUSED(y_start);
            LCD_UNUSED(x_end);
            LCD_UNUSED(y_end);
            LCD_UNUSED(dst_width);
            LCD_UNUSED(dst_height);
            break;
    }
}