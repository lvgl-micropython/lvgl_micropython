// Copyright (c) 2024 - 2025 Kevin G. Schlosser

// local includes
#include "lcd_types.h"
#include "lcd_bus_task.h"
#include "modlcd_bus.h"
#include "dsi_bus.h"
#include "bus_trans_done.h"

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
    #include "hal/lcd_types.h"
    #include "esp_lcd_mipi_dsi.h"
    #include "esp_task.h"


    typedef struct {
        esp_lcd_panel_t base;         // Base class of generic lcd panel
        esp_lcd_dsi_bus_handle_t bus; // DSI bus handle
        uint8_t virtual_channel;      // Virtual channel ID, index from 0
        uint8_t cur_fb_index;         // Current frame buffer index
        uint8_t num_fbs;              // Number of frame buffers
        uint8_t *fbs[3]; // Frame buffers
    } dpi_panel_t;


    mp_lcd_err_t dsi_del(mp_lcd_bus_obj_t *self_in);
    mp_lcd_err_t dsi_init(mp_lcd_bus_obj_t *self_in, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,
                          uint8_t cmd_bits, uint8_t param_bits);


    static void dsi_send_func(mp_lcd_bus_obj_t *self_in, int cmd, uint8_t *params, size_t params_len)
    {
        mp_lcd_dsi_bus_obj_t *self = (mp_lcd_dsi_bus_obj_t *)self_in;
        esp_lcd_panel_io_tx_param(self->panel_io_handle, cmd, params, params_len);
    }


    static bool dsi_bus_trans_done_cb(esp_lcd_panel_handle_t panel, esp_lcd_dpi_panel_event_data_t *edata, void *user_ctx)
    {
        LCD_UNUSED(edata);
        mp_lcd_dsi_bus_obj_t *self = (mp_lcd_dsi_bus_obj_t *)user_ctx;
        dpi_panel_t *dpi_panel = __containerof(panel, dpi_panel_t, base);

        uint8_t *curr_buf = dpi_panel->fbs[dpi_panel->cur_fb_index];

        if (curr_buf != self->bufs.active && !lcd_bus_event_isset_from_isr(&self->tx_data.swap_bufs)) {
            uint8_t *idle_fb = self->bufs.idle;
            self->bufs.idle = self->bufs.active;
            self->bufs.active = idle_fb;
            lcd_bus_event_set_from_isr(self->tx_data.swap_bufs);
        }

        return false;
    }

    static void dsi_flush_func(mp_lcd_bus_obj_t *self_in, rotation_data_t *r_data, rotation_data_t *original_r_data,
                               uint8_t *idle_fb, uint8_t last_update)
    {

        mp_lcd_dsi_bus_obj_t *self = (mp_lcd_dsi_bus_obj_t *)self_in;

        if (self->callback != mp_const_none) {
            cb_isr(self->callback);
        }

        if (last_update) {
            mp_lcd_err_t ret = esp_lcd_panel_draw_bitmap(self->panel_handle, 0, 0,
                                original_r_data->dst_width, original_r_data->dst_height,
                                idle_fb);

            if (ret != 0) {
                mp_printf(&mp_plat_print, "esp_lcd_panel_draw_bitmap error (%d)\n", ret);
            } else {
                lcd_bus_event_clear(self->tx_data.swap_bufs);
                lcd_bus_event_wait(self->tx_data.swap_bufs);

                size_t size = (size_t)(original_r_data->dst_width * original_r_data->dst_height * original_r_data->bytes_per_pixel);
                memcpy(self->bufs.idle, self->bufs.active, size);
            }
        }
    }


    static bool dsi_init_func(mp_lcd_bus_obj_t *self_in)
    {
        mp_lcd_dsi_bus_obj_t *self = (mp_lcd_dsi_bus_obj_t *) self_in;

        self->init.err = esp_lcd_new_dsi_bus(&self->bus_config,  &self->bus_handle);

        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_dsi_bus)");
            return false;
        }

        self->init.err = esp_lcd_new_panel_io_dbi(self->bus_handle, &self->panel_io_config, &self->panel_io_handle);

        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_dbi)");
            return false;
        }

        self->init.err = esp_lcd_new_panel_dpi(self->bus_handle, &self->panel_config, &self->panel_handle);

        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_panel_dpi)");
            return false;
        }

        self->init.err = esp_lcd_panel_init(self->panel_handle);

        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_panel_init)");
            return false;
        }

        esp_lcd_dpi_panel_event_callbacks_t callbacks = {
            .on_refresh_done = &dsi_bus_trans_done_cb
        };

        self->init.err = esp_lcd_dpi_panel_register_event_callbacks(self->panel_handle, &callbacks, self);

        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_dpi_panel_register_event_callbacks)");
            return false;
        }

        dpi_panel_t *dpi_panel = __containerof((esp_lcd_panel_t *)self->panel_handle, dpi_panel_t, base);

        self->bufs.active = dpi_panel->fbs[0];
        self->bufs.idle = dpi_panel->fbs[1];

        return true;

    }


    static mp_obj_t mp_lcd_dsi_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        enum { ARG_bus_id, ARG_data_lanes, ARG_freq, ARG_lane_bit_rate, ARG_panel_bpp,
               ARG_hsync_front_porch, ARG_hsync_back_porch, ARG_hsync_pulse_width,
               ARG_vsync_front_porch, ARG_vsync_back_porch, ARG_vsync_pulse_width };

        const mp_arg_t make_new_args[] = {
            { MP_QSTR_bus_id,            MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
            { MP_QSTR_num_lanes,         MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
            { MP_QSTR_freq,              MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
            { MP_QSTR_lane_bit_rate,     MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
            { MP_QSTR_panel_bpp,         MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
            { MP_QSTR_hsync_front_porch, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
            { MP_QSTR_hsync_back_porch,  MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
            { MP_QSTR_hsync_pulse_width, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
            { MP_QSTR_vsync_front_porch, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
            { MP_QSTR_vsync_back_porch,  MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED },
            { MP_QSTR_vsync_pulse_width, MP_ARG_INT | MP_ARG_KW_ONLY | MP_ARG_REQUIRED }
        };
    
        mp_arg_val_t args[MP_ARRAY_SIZE(make_new_args)];
        mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(make_new_args), make_new_args, args);

        // create new object
        mp_lcd_dsi_bus_obj_t *self = m_new_obj(mp_lcd_dsi_bus_obj_t);
        self->base.type = &mp_lcd_dsi_bus_type;
    
        self->callback = mp_const_none;

        self->bus_config.bus_id = (int)args[ARG_bus_id].u_int;
        self->bus_config.num_data_lanes = (uint8_t)args[ARG_data_lanes].u_int;
        self->bus_config.lane_bit_rate_mbps = (uint32_t)args[ARG_lane_bit_rate].u_int;
        self->bus_config.phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT;

        switch((uint8_t)args[ARG_panel_bpp].u_int) {
            case 16:
                self->panel_config.out_color_format = LCD_COLOR_FMT_RGB565;
                break;
            case 18:
                self->panel_config.out_color_format = LCD_COLOR_FMT_RGB666;
                self->r_data.swap = 0;
                break;
            case 24:
                self->panel_config.out_color_format = LCD_COLOR_FMT_RGB888;
                self->r_data.swap = 0;
                break;
            default:
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("unsopported bits per pixel.(%d)"),
                                  (uint8_t)args[ARG_panel_bpp].u_int);
                return mp_const_none;
        }

        self->panel_config.virtual_channel = 0;
        self->panel_config.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
        self->panel_config.flags.use_dma2d = 0;
        self->panel_config.flags.disable_lp = 1;
        self->panel_config.dpi_clock_freq_mhz = (uint32_t)args[ARG_freq].u_int;
        self->panel_config.video_timing.hsync_back_porch = (uint32_t)args[ARG_hsync_back_porch].u_int;
        self->panel_config.video_timing.hsync_pulse_width = (uint32_t)args[ARG_hsync_pulse_width].u_int;
        self->panel_config.video_timing.hsync_front_porch = (uint32_t)args[ARG_hsync_front_porch].u_int;
        self->panel_config.video_timing.vsync_back_porch = (uint32_t)args[ARG_vsync_back_porch].u_int;
        self->panel_config.video_timing.vsync_pulse_width = (uint32_t)args[ARG_vsync_pulse_width].u_int;
        self->panel_config.video_timing.vsync_front_porch = (uint32_t)args[ARG_vsync_front_porch].u_int;
        self->panel_config.num_fbs = 2;

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

        self->num_lanes = self->bus_config.num_data_lanes;

        self->internal_cb_funcs.deinit = &dsi_del;
        self->internal_cb_funcs.init = &dsi_init;

        self->tx_data.flush_func = &dsi_flush_func;
        self->init.init_func = &dsi_init_func;

        self->tx_cmds.send_func = &dsi_send_func;

        return MP_OBJ_FROM_PTR(self);
    }
    

    mp_lcd_err_t dsi_init(mp_lcd_bus_obj_t *self_in, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,
                          uint8_t cmd_bits, uint8_t param_bits)
    {
        LCD_DEBUG_PRINT("dsi_init(self, width=%i, height=%i, bpp=%i, buffer_size=%lu, cmd_bits=%i, param_bits=%i)\n",
                         width, height, bpp, buffer_size, cmd_bits, param_bits)

        mp_lcd_dsi_bus_obj_t *self = (mp_lcd_dsi_bus_obj_t *)self_in;

        switch(bpp) {
            case 16:
                self->panel_config.in_color_format = LCD_COLOR_FMT_RGB565;
                self->panel_config.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB565;
                break;
            case 18:
                self->panel_config.in_color_format = LCD_COLOR_FMT_RGB666;
                self->panel_config.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB666;
                self->r_data.swap = 0;
                break;
            case 24:
                self->panel_config.in_color_format = LCD_COLOR_FMT_RGB888;
                self->panel_config.pixel_format = LCD_COLOR_PIXEL_FORMAT_RGB888;
                self->r_data.swap = 0;
                break;
            default:
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("unsopported bits per pixel.(%d)"), bpp);
                return LCD_ERR_INVALID_ARG;
        }

        self->panel_config.video_timing.h_size = (uint32_t)width;
        self->panel_config.video_timing.v_size = (uint32_t)height;

        self->panel_io_config.virtual_channel = 0;
        self->panel_io_config.lcd_cmd_bits = (int)cmd_bits;
        self->panel_io_config.lcd_param_bits = (int)param_bits;

        self->r_data.dst_width = (uint16_t)width;
        self->r_data.dst_height = (uint16_t)height;

        LCD_DEBUG_PRINT("virtual_channel=%d\n", self->panel_io_config.virtual_channel)
        LCD_DEBUG_PRINT("lcd_cmd_bits=%d\n", self->panel_io_config.lcd_cmd_bits)
        LCD_DEBUG_PRINT("lcd_param_bits=%d\n", self->panel_io_config.lcd_param_bits)
        LCD_DEBUG_PRINT("h_size=%d\n", self->panel_config.video_timing.h_size)
        LCD_DEBUG_PRINT("v_size=%d\n", self->panel_config.video_timing.v_size)
        LCD_DEBUG_PRINT("pixel_format=%d\n", self->panel_config.pixel_format)

        xTaskCreatePinnedToCore(
                lcd_bus_task, "dsi_task", LCD_DEFAULT_STACK_SIZE / sizeof(StackType_t),
                self, ESP_TASK_PRIO_MAX - 1, (TaskHandle_t *)&self->task.handle, 0);

        lcd_bus_lock_acquire(self->init.lock);
        lcd_bus_lock_release(self->init.lock);
        lcd_bus_lock_delete(self->init.lock);

        return self->init.err;
    }


    mp_lcd_err_t dsi_del(mp_lcd_bus_obj_t *self_in)
    {
        LCD_DEBUG_PRINT("dsi_del(self)\n")

        mp_lcd_dsi_bus_obj_t *self = (mp_lcd_dsi_bus_obj_t *)self_in;

        mp_lcd_err_t ret = esp_lcd_panel_del(self->panel_handle);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_del)"), ret);
            return ret;
        }

        ret = esp_lcd_panel_io_del(self->panel_io_handle);
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

    MP_DEFINE_CONST_OBJ_TYPE(
        mp_lcd_dsi_bus_type,
        MP_QSTR_DSIBus,
        MP_TYPE_FLAG_NONE,
        make_new, mp_lcd_dsi_bus_make_new,
        locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
    );

#else
    #include "../common_src/dsi_bus.c"

#endif /*SOC_MIPI_DSI_SUPPORTED*/
