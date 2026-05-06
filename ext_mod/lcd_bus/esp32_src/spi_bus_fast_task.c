// Copyright (c) 2024 - 2025 Kevin G. Schlosser
// Copyright (c) 2024 - 2025 Viktor Vorobjov

#include "spi_bus_fast.h"
#include "lcd_types.h"
#include "spi_bus_fast_rotation.h"

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/stackctrl.h"

// esp-idf includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"
#include "esp_system.h"
#include "esp_cpu.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"

#include <string.h>

// Copy partial data to full frame buffer optimization
static void copy_partial_to_full_buffer(mp_lcd_spi_bus_fast_obj_t *self, int x_start, int y_start, int x_end, int y_end)
{
    if (self->partial_buf == NULL) {
        return;
    }

    uint8_t *idle_fb = self->idle_fb;
    uint8_t *partial_data = self->partial_buf;
    
    uint32_t width = x_end - x_start + 1;
    uint32_t height = y_end - y_start + 1;
    uint32_t bytes_per_pixel = self->bytes_per_pixel;
    
    
    
    // Use rotation-aware copy function
    uint32_t fb_width = self->width;
    uint32_t fb_height = self->height;
    
    spi_fast_copy_pixels(
        idle_fb, partial_data,
        x_start, y_start, x_end, y_end,
        fb_width, fb_height,
        bytes_per_pixel, 
        self->rotation,
        self->rgb565_byte_swap && (bytes_per_pixel == 2) ? 1 : 0
    );
    
}

// Send full frame buffer with chunking (like current implementation)
static void send_full_buffer_chunked(mp_lcd_spi_bus_fast_obj_t *self, int ramwr_cmd, int ramwrc_cmd)
{
    uint32_t total_size = self->width * self->height * self->bytes_per_pixel;
    uint32_t chunk_size = 1024 * 10; // 10KB chunks - TODO: make configurable for different screen sizes
    uint32_t remaining = total_size;
    uint32_t offset = 0;
    uint8_t *buffer = self->idle_fb;
    
    

    // Set window coordinates for FULL screen before sending full buffer
    uint16_t max_x = self->width - 1;
    uint8_t caset_params[4] = {
        0x00, 0x00,                    // x1 = 0 (start)
        (max_x >> 8) & 0xFF,          // x2 high byte  
        max_x & 0xFF                  // x2 low byte
    };
    
    
    // QSPI CASET command: 0x2A -> 0x02002A00 (like in working Python driver)
    // Format: [MODE][ADDR][DUMMY][DATA] where:
    // - 0x02 = command mode for QSPI
    // - 0x2A = CASET (Column Address Set) command
    // - 0x00 = dummy/padding bytes
    uint32_t caset_cmd = (0x02 << 24) | (0x2A << 8); // 0x02002A00
    esp_lcd_panel_io_tx_param(self->panel_io_handle.panel_io, caset_cmd, caset_params, 4);
    
    uint32_t pixel_count = total_size / 2;
    
    int chunk_count = 0;
    while (remaining > 0) {
        uint32_t current_chunk = (remaining > chunk_size) ? chunk_size : remaining;
        uint8_t *chunk_data = buffer + offset;
        int cmd = (offset == 0) ? ramwr_cmd : ramwrc_cmd;
        
        // LCD_DEBUG_PRINT("send_full_buffer_chunked: chunk %d - offset=%d, size=%d, cmd=0x%x, data=%p\n", 
        //                 chunk_count, offset, current_chunk, cmd, chunk_data)
        
        // Direct ESP-IDF API call to send data
        esp_err_t ret = esp_lcd_panel_io_tx_color(
            self->panel_io_handle.panel_io,
            cmd,
            chunk_data,
            current_chunk
        );
        
        if (ret != ESP_OK) {
        } else {
            // LCD_DEBUG_PRINT("send_full_buffer_chunked: chunk %d sent successfully\n", chunk_count)
        }
        
        offset += current_chunk;
        remaining -= current_chunk;
        chunk_count++;
    }
    
    // Cannot print from RTOS task to MicroPython I/O
}

void spi_bus_fast_copy_task(void *self_in) {
    mp_lcd_spi_bus_fast_obj_t *self = (mp_lcd_spi_bus_fast_obj_t *)self_in;
    
    
    // Perform all SPI initialization inside task (like RGB does with LCD)
    self->init_err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)self->spi_device.spi_bus->host, 
                                              &self->panel_io_config, 
                                              &self->panel_io_handle.panel_io);
    
    if (self->init_err != ESP_OK) {
        self->init_err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_spi)");
        spi_bus_fast_lock_release(&self->init_lock);
        return;
    }
    
    bool last_update;
    
    // Critical lock ordering to prevent race conditions:
    // 1. Acquire copy_lock first (task is now ready to process data)
    // 2. Then release init_lock (signals main thread that init is complete)
    // This ensures main thread cannot call tx_color before task is ready
    // IMPORTANT: acquire copy_lock BEFORE releasing init_lock (like in RGB)
    spi_bus_fast_lock_acquire(&self->copy_lock, -1);
    
    self->init_err = LCD_OK;
    spi_bus_fast_lock_release(&self->init_lock);


    bool exit = spi_bus_fast_event_isset(&self->copy_task_exit);
    
    while (!exit) {
        // Wait for signal from spi_fast_tx_color
        spi_bus_fast_lock_acquire(&self->copy_lock, -1);
        
        
        if (self->partial_buf == NULL) break;
        
        // Read ALL variables under lock protection atomically
        // This prevents race conditions where coordinates might change 
        // between reads if main thread calls tx_color during this moment
        last_update = self->last_update;
        
        // CRITICAL: read coordinates at the SAME moment as last_update
        // All these values must be read atomically as a group
        int x_start = self->x_start;
        int y_start = self->y_start;
        int x_end = self->x_end;
        int y_end = self->y_end;
        
        
        // Diagnostics removed - use ets_printf from task
        
        // Copy partial data to full buffer (analog of copy_pixels) with ATOMIC coordinates
        copy_partial_to_full_buffer(self, x_start, y_start, x_end, y_end);
        
        // Release tx_color_lock AFTER reading last_update and copying (like RGB)
        spi_bus_fast_lock_release(&self->tx_color_lock);
        
        // Callback like in RGB
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
                mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
            }
            gc_unlock();
            mp_sched_unlock();
            mp_thread_set_state(old_state);
        }
        
        // Send full buffer only on last update (like RGB driver)
        if (last_update) {
            send_full_buffer_chunked(self, 0x32002C00, 0x32003C00);
            
            // FIRST FRAME STRATEGY: initialize both buffers with same content
            if (!self->first_frame_received) {
                // First full frame - copy it to active_fb, then to idle_fb
                uint32_t total_size = self->width * self->height * self->bytes_per_pixel;
                memcpy(self->active_fb, self->idle_fb, total_size);  // active_fb = idle_fb (accumulated frame)
                memcpy(self->idle_fb, self->active_fb, total_size);  // idle_fb = active_fb (same frame)
                self->first_frame_received = 1;
                // ets_printf("FIRST_FRAME: initialized both buffers with full frame\n");
            } else {
                // Normal simple swap like RGB driver
                uint8_t *temp = self->active_fb;
                self->active_fb = self->idle_fb;
                self->idle_fb = temp;
                // ets_printf("SIMPLE_SWAP: active <-> idle\n");
            }
        }
        
        self->partial_buf = NULL;
        exit = spi_bus_fast_event_isset(&self->copy_task_exit);
    }
}