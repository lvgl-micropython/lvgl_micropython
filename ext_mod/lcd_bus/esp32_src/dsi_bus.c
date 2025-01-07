// Copyright (c) 2024 - 2025 Kevin G. Schlosser

// local includes
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "dsi_bus.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>

// esp-idf includes
#include "soc/soc_caps.h"

#if SOC_MIPI_DSI_SUPPORTED
    // esp-idf includes
    #include "esp_lcd_panel_ops.h"
    #include "esp_lcd_panel_interface.h"
    #include "esp_lcd_panel_io.h"
    #include "esp_heap_caps.h"
    #include "hal/lcd_types.h"
    #include "esp_lcd_mipi_dsi.h"

    
    typedef struct {
        esp_lcd_panel_t base;         // Base class of generic lcd panel
        esp_lcd_dsi_bus_handle_t bus; // DSI bus handle
        uint8_t virtual_channel;      // Virtual channel ID, index from 0
        uint8_t cur_fb_index;         // Current frame buffer index
        uint8_t num_fbs;              // Number of frame buffers
        uint8_t *fbs[DPI_PANEL_MAX_FB_NUM]; // Frame buffers
    } dpi_panel_t;


    mp_lcd_err_t dsi_del(mp_obj_t obj);
    mp_lcd_err_t dsi_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
    mp_lcd_err_t dsi_get_lane_count(mp_obj_t obj, uint8_t *lane_count);
    mp_lcd_err_t dsi_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, , uint8_t rotation, bool last_update);
    mp_obj_t dsi_allocate_framebuffer(mp_obj_t obj, uint32_t size, uint32_t caps);
    mp_obj_t dsi_free_framebuffer(mp_obj_t obj, mp_obj_t buf);


    static bool dsi_bus_trans_done_cb(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
    {
        LCD_UNUSED(edata);

        dpi_panel_t *dpi_panel = __containerof(panel, dpi_panel_t, base);
        mp_lcd_dsi_bus_obj_t *self = (mp_lcd_dsi_bus_obj_t *)user_ctx;

        if (!self->trans_done && dpi_panel->fbs[dpi_panel->cur_fb_index] == self->transmitting_buf) {
           if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
               cb_isr(self->callback);
           }
           self->trans_done = true;
        }

        return false;
    }


    static mp_obj_t mp_lcd_dsi_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        enum {
            ARG_bus_id,
            ARG_data_lanes,
            ARG_freq,
            ARG_virtual_channel,
            ARG_hsync_front_porch,
            ARG_hsync_back_porch,
            ARG_hsync_pulse_width,
            ARG_vsync_front_porch,
            ARG_vsync_back_porch,
            ARG_vsync_pulse_width
        };

        const mp_arg_t make_new_args[] = {
            { MP_QSTR_bus_id,             MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_data_lanes,         MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_freq,               MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_virtual_channel,    MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_hsync_front_porch,  MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0       } },
            { MP_QSTR_hsync_back_porch,   MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0       } },
            { MP_QSTR_hsync_pulse_width,  MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 1       } },
            { MP_QSTR_vsync_front_porch,  MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0       } },
            { MP_QSTR_vsync_back_porch,   MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0       } },
            { MP_QSTR_vsync_pulse_width,  MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 1       } }
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

        // create new object
        mp_lcd_dsi_bus_obj_t *self = m_new_obj(mp_lcd_dsi_bus_obj_t);
        self->base.type = &mp_lcd_dsi_bus_type;
    
        self->callback = mp_const_none;

        self->bus_config.bus_id = (int)args[ARG_bus_id].u_int;
        self->bus_config.num_data_lanes = (uint8_t)args[ARG_data_lanes].u_int;
        self->bus_config.lane_bit_rate_mbps = (uint32_t)args[ARG_freq].u_int;
        self->bus_config.phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT;

        self->panel_io_config.virtual_channel = (uint8_t)args[ARG_virtual_channel].u_int;

        self->panel_config.virtual_channel = (uint8_t)args[ARG_virtual_channel].u_int;
        self->panel_config.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;

        self->panel_config.dpi_clock_freq_mhz = (uint32_t)args[ARG_freq].u_int;

        self->panel_config.video_timing.hsync_back_porch = (uint32_t)args[ARG_hsync_back_porch].u_int;
        self->panel_config.video_timing.hsync_pulse_width = (uint32_t)args[ARG_hsync_pulse_width].u_int;
        self->panel_config.video_timing.hsync_front_porch = (uint32_t)args[ARG_hsync_front_porch].u_int;
        self->panel_config.video_timing.vsync_back_porch = (uint32_t)args[ARG_vsync_back_porch].u_int;
        self->panel_config.video_timing.vsync_pulse_width = (uint32_t)args[ARG_vsync_pulse_width].u_int;
        self->panel_config.video_timing.vsync_front_porch = (uint32_t)args[ARG_vsync_front_porch].u_int;

        self->panel_config.num_fbs = 0;

        self->bus_config.pclk_hz = (uint32_t)args[ARG_freq].u_int;

        LCD_DEBUG_PRINT("bus_id=%d\n", self->bus_config.bus_id)
        LCD_DEBUG_PRINT("num_data_lanes=%d\n", self->bus_config.num_data_lanes)
        LCD_DEBUG_PRINT("lane_bit_rate_mbps=%d\n",self->bus_config.lane_bit_rate_mbps)
        LCD_DEBUG_PRINT("phy_clk_src=%d\n", self->bus_config.phy_clk_src)
        LCD_DEBUG_PRINT("virtual_channel=%d\n", self->panel_io_config.virtual_channel)
        LCD_DEBUG_PRINT("dpi_clk_src=%d\n", self->panel_config.dpi_clk_src)
        LCD_DEBUG_PRINT("dpi_clock_freq_mhz=%d\n", self->panel_config.dpi_clock_freq_mhz)
        LCD_DEBUG_PRINT("hsync_front_porch=%d\n", self->panel_config.video_timing.hsync_front_porch)
        LCD_DEBUG_PRINT("hsync_back_porch=%d\n", self->panel_config.video_timing.hsync_back_porch)
        LCD_DEBUG_PRINT("hsync_pulse_width=%d\n", self->panel_config.video_timing.hsync_pulse_width)
        LCD_DEBUG_PRINT("vsync_front_porch=%d\n", self->panel_config.video_timing.vsync_front_porch)
        LCD_DEBUG_PRINT("vsync_back_porch=%d\n", self->panel_config.video_timing.vsync_back_porch)
        LCD_DEBUG_PRINT("vsync_pulse_width=%d\n", self->panel_config.video_timing.vsync_pulse_width)
        LCD_DEBUG_PRINT("pclk_hz[10]=%d\n", self->bus_config.pclk_hz)

        self->panel_io_handle.get_lane_count = &dsi_get_lane_count;
        self->panel_io_handle.del = &dsi_del;
        self->panel_io_handle.tx_color = &dsi_tx_color;
        self->panel_io_handle.allocate_framebuffer = &dsi_allocate_framebuffer;
        self->panel_io_handle.free_framebuffer = &dsi_free_framebuffer;
        self->panel_io_handle.init = &dsi_init;

        return MP_OBJ_FROM_PTR(self);
    }
    

    mp_lcd_err_t dsi_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits)
    {
        LCD_DEBUG_PRINT("dsi_init(self, width=%i, height=%i, bpp=%i, buffer_size=%lu, rgb565_byte_swap=%i, cmd_bits=%i, param_bits=%i)\n", width, height, bpp, buffer_size, (uint8_t)rgb565_byte_swap, cmd_bits, param_bits)

        mp_lcd_dsi_bus_obj_t *self = (mp_lcd_dsi_bus_obj_t *)obj;

        switch(bpp) {
            case 16:
                self->panel_config.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
                self->rgb565_byte_swap = rgb565_byte_swap;
                break;
            case 18:
                self->panel_config.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB666;
                self->rgb565_byte_swap = false;
                break;
            case 24:
                self->panel_config.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB888;
                self->rgb565_byte_swap = false;
                break;
            default:
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("unsopported bits per pixel.(%d)"), bpp);
                return LCD_ERR_INVALID_ARG;
        }

        self->panel_config.video_timing.h_size = (uint32_t)width;
        self->panel_config.video_timing.v_size = (uint32_t)height;

        self->panel_io_config.lcd_cmd_bits = (int)cmd_bits;
        self->panel_io_config.lcd_param_bits = (int)param_bits;

        LCD_DEBUG_PRINT("lcd_cmd_bits=%d\n", self->panel_io_config.lcd_cmd_bits)
        LCD_DEBUG_PRINT("lcd_param_bits=%d\n", self->panel_io_config.lcd_param_bits)
        LCD_DEBUG_PRINT("h_size=%d\n", self->panel_config.video_timing.h_size)
        LCD_DEBUG_PRINT("v_size=%d\n", self->panel_config.video_timing.v_size)
        LCD_DEBUG_PRINT("pixel_format=%d\n", self->panel_config.pixel_format)

        esp_err_t ret = esp_lcd_new_dsi_bus(&self->bus_config,  &self->bus_handle);

        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_dsi_bus)"), ret);
            return ret;
        }

        ret = esp_lcd_new_panel_io_dsi(self->bus_handle, &self->panel_io_config, &self->panel_io_handle.panel_io);

        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_dsi)"), ret);
            return ret;
        }

        ret = esp_lcd_new_panel_dpi(self->bus_handle, &self->panel_config, &self->panel_handle);

        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_panel_dpi)"), ret);
            return ret;
        }

        ret = esp_lcd_panel_init(self->panel_handle);

        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_init)"), ret);
            return ret;
        }

        esp_lcd_dpi_panel_event_callbacks_t callbacks = {
            .on_refresh_done = &dsi_bus_trans_done_cb
        };

        ret = esp_lcd_dpi_panel_register_event_callbacks(self->panel_handle, &callbacks, self);

        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_dpi_panel_register_event_callbacks)"), ret);
            return ret;
        }

        dpi_panel_t *dpi_panel = __containerof((esp_lcd_panel_t *)self->panel_handle, dpi_panel_t, base);

        void *buf1 = self->view1->items;
        self->view1->items = (void *)dpi_panel->fbs[0];
        self->view1->len = buffer_size;
        heap_caps_free(buf1);

        if (self->panel_config.num_fbs == 2) {
            void *buf2 = self->view2->items;
            self->view2->items = (void *)dpi_panel->fbs[1];
            self->view2->len = buffer_size;
            heap_caps_free(buf2);
        }

        return ret;
    }


    mp_lcd_err_t dsi_del(mp_obj_t obj)
    {
        LCD_DEBUG_PRINT("dsi_del(self)\n")

        mp_lcd_dsi_bus_obj_t *self = (mp_lcd_dsi_bus_obj_t *)obj;

        mp_lcd_err_t ret = esp_lcd_panel_del(self->panel_handle);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_del)"), ret);
            return ret;
        }


        mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle.panel_io);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
            return ret;
        }

        ret = esp_lcd_del_dsi_bus(self->bus_handle);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_del_dsi_bus)"), ret);
            return ret;
        }

        return ret;
    }

    mp_lcd_err_t dsi_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
    {
        mp_lcd_dsi_bus_obj_t *self = (mp_lcd_dsi_bus_obj_t *)obj;
        *lane_count = (uint8_t)self->bus_config.num_data_lanes;

        LCD_DEBUG_PRINT("dsi_get_lane_count(self)-> %d\n", (uint8_t)self->bus_config.num_data_lanes)

        return LCD_OK;
    }


    mp_obj_t dsi_free_framebuffer(mp_obj_t obj, mp_obj_t buf)
    {
        mp_lcd_dsi_bus_obj_t *self = (mp_lcd_dsi_bus_obj_t *)obj;

        if (self->panel_handle != NULL) {
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("Unable to free buffer"));
            return mp_const_none;
        }

        mp_obj_array_t *array_buf = (mp_obj_array_t *)MP_OBJ_TO_PTR(buf);
        void *item_buf = array_buf->items;

        if (array_buf == self->view1) {
            heap_caps_free(item_buf);
            self->view1 = NULL;
            LCD_DEBUG_PRINT("dsi_free_framebuffer(self, buf=1)\n")
        } else if (array_buf == self->view2) {
            heap_caps_free(item_buf);
            self->view2 = NULL;
            LCD_DEBUG_PRINT("dsi_free_framebuffer(self, buf=2)\n")
        } else {
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("No matching buffer found"));
        }
        return mp_const_none;
    }


    mp_obj_t dsi_allocate_framebuffer(mp_obj_t obj, uint32_t size, uint32_t caps)
    {
        LCD_DEBUG_PRINT("dsi_allocate_framebuffer(self, size=%lu, caps=%lu)\n", size, caps)

        mp_lcd_dsi_bus_obj_t *self = (mp_lcd_dsi_bus_obj_t *)obj;

        if ((caps | MALLOC_CAP_DMA) == caps) {
         #if SOC_DMA2D_SUPPORTED
            self->panel_config.flags.use_dma2d = true;
         #else
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("DMA is not supported"));
            return mp_const_none;
         #endif
        }

        void *buf = heap_caps_calloc(1, 1, MALLOC_CAP_INTERNAL);

        mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, 1, buf));
        view->typecode |= 0x80; // used to indicate writable buffer

        uint32_t available =  (uint32_t)heap_caps_get_largest_free_block(caps);
        if (available < size) {
            heap_caps_free(buf);
            mp_raise_msg_varg(
                &mp_type_MemoryError,
                MP_ERROR_TEXT("Not enough memory available (%d)"),
                size
            );
            return mp_const_none;
        }

        if (self->view1 == NULL) {
            self->buffer_size = size;
            self->view1 = view;
            self->panel_config.num_fbs = 1;
        } else if (self->buffer_size != size) {
            heap_caps_free(buf);
            mp_raise_msg_varg(
                &mp_type_MemoryError,
                MP_ERROR_TEXT("Frame buffer sizes do not match (%d)"),
                size
            );
            return mp_const_none;
        } else if (self->view2 == NULL) {
            self->view2 = view;
            self->panel_config.num_fbs = 2;
        } else {
            heap_caps_free(buf);
            mp_raise_msg(&mp_type_MemoryError,
                    MP_ERROR_TEXT("There is a maximum of 2 frame buffers allowed"));
            return mp_const_none;
        }

        return MP_OBJ_FROM_PTR(view);
    }
    

    mp_lcd_err_t dsi_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update)
    {
        LCD_DEBUG_PRINT("dsi_tx_color(self, lcd_cmd=%d, color, color_size=%d, x_start=%d, y_start=%d, x_end=%d, y_end=%d)\n", lcd_cmd, color_size, x_start, y_start, x_end, y_end)

        LCD_UNUSED(rotation);
        LCD_UNUSED(last_update);

        mp_lcd_dsi_bus_obj_t *self = (mp_lcd_dsi_bus_obj_t *)obj;

        self->trans_done = false;
        self->transmitting_buf = color;

        esp_err_t ret = esp_lcd_panel_draw_bitmap(
            self->panel_handle,
            x_start,
            y_start,
            x_end,
            y_end,
            color
        );

        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_draw_bitmap)"), ret);
            return LCD_OK;
        }

        if (self->callback == mp_const_none || self->panel_config.num_fbs != 2) {
            while (!self->trans_done) {}
            self->trans_done = false;
        }

        return LCD_OK;
    }


    MP_DEFINE_CONST_OBJ_TYPE(
        mp_lcd_dsi_bus_type,
        MP_QSTR_DSIBus,
        MP_TYPE_FLAG_NONE,
        make_new, mp_lcd_dsi_bus_make_new,
        locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
    );

#endif /*SOC_MIPI_DSI_SUPPORTED*/
