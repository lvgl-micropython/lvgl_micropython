// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"

#include "soc/soc_caps.h"

#if SOC_LCD_RGB_SUPPORTED

    #include "hal/lcd_hal.h"
    #include "esp_pm.h"
    #include "esp_intr_alloc.h"
    #include "esp_heap_caps.h"

    #include "esp_lcd_panel_io.h"
    #include "esp_lcd_panel_ops.h"
    #include "esp_lcd_panel_interface.h"
    #include "esp_lcd_panel_rgb.h"

    #include "common/modlcd_bus.h"
    #include "common/lcd_common_types.h"
    #include "lcd_types.h"
    #include "rgb_bus.h"

    #include <string.h>


    mp_lcd_err_t rgb_del(mp_obj_t obj);
    mp_lcd_err_t rgb_init(mp_obj_t obj, uint8_t cmd_bits, uint8_t param_bits);


    typedef struct {
        esp_lcd_panel_t base;  // Base class of generic lcd panel
        int panel_id;          // LCD panel ID
        lcd_hal_context_t hal; // Hal layer object
        size_t data_width;     // Number of data lines
        size_t fb_bits_per_pixel; // Frame buffer color depth, in bpp
        size_t num_fbs;           // Number of frame buffers
        size_t output_bits_per_pixel; // Color depth seen from the output data line. Default to fb_bits_per_pixel, but can be changed by YUV-RGB conversion
        size_t sram_trans_align;  // Alignment for framebuffer that allocated in SRAM
        size_t psram_trans_align; // Alignment for framebuffer that allocated in PSRAM
        int disp_gpio_num;     // Display control GPIO, which is used to perform action like "disp_off"
        intr_handle_t intr;    // LCD peripheral interrupt handle
        esp_pm_lock_handle_t pm_lock; // Power management lock
        size_t num_dma_nodes;  // Number of DMA descriptors that used to carry the frame buffer
        uint8_t *fbs[3]; // Frame buffers
        uint8_t cur_fb_index;  // Current frame buffer index
        uint8_t bb_fb_index;  // Current frame buffer index which used by bounce buffer
    } rgb_panel_t;


    static bool rgb_trans_done_cb(esp_lcd_panel_handle_t panel,
                                const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
    {
        LCD_UNUSED(edata);
        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)user_ctx;
        mp_lcd_sw_rotation_buffers_t *buffers = &self->sw_rot.buffers;

        rgb_panel_t *rgb_panel = __containerof(panel, rgb_panel_t, base);
        uint8_t *curr_buf = rgb_panel->fbs[rgb_panel->cur_fb_index];

        if (curr_buf != buffers->active && !mp_lcd_event_isset_from_isr(&self->sw_rot.handles.swap_bufs)) {
            uint8_t *idle_fb = buffers->idle;
            buffers->idle = buffers->active;
            buffers->active = idle_fb;
            mp_lcd_event_set_from_isr(&self->sw_rot.handles.swap_bufs);
        }

        return false;
    }


    static bool rgb_init_cb(void *self_in)
    {
        LCD_DEBUG_PRINT("rgb_init_cb\n")

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)self_in;
        mp_lcd_sw_rotation_init_t *init = &self->sw_rot.init;

        free(self->sw_rot.buffers.active);
        self->sw_rot.buffers.active = NULL;
        self->sw_rot.buffers.idle = NULL;

        esp_lcd_rgb_panel_event_callbacks_t callbacks = { .on_vsync = rgb_trans_done_cb };

        LCD_DEBUG_PRINT("esp_lcd_new_rgb_panel\n")
        init->err = esp_lcd_new_rgb_panel(self->panel_io_config, &self->panel_handle);
        if (init->err != 0) {
            init->err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_rgb_panel)");
            return false;
        }

        LCD_DEBUG_PRINT("esp_lcd_rgb_panel_register_event_callbacks\n")
        init->err = esp_lcd_rgb_panel_register_event_callbacks(self->panel_handle, &callbacks, self);
        if (init->err != 0) {
            init->err_msg = MP_ERROR_TEXT("%d(esp_lcd_rgb_panel_register_event_callbacks)");
            return false;
        }

        LCD_DEBUG_PRINT("esp_lcd_panel_reset\n")
        init->err = esp_lcd_panel_reset(self->panel_handle);
        if (self->sw_rot.init.err != 0) {
            init->err_msg = MP_ERROR_TEXT("%d(esp_lcd_panel_reset)");
            return false;
        }

        LCD_DEBUG_PRINT("esp_lcd_panel_init\n")
        init->err = esp_lcd_panel_init(self->panel_handle);
        if (init->err != 0) {
            init->err_msg = MP_ERROR_TEXT("%d(esp_lcd_panel_init)");
            return false;
        }

        rgb_panel_t *rgb_panel = __containerof((esp_lcd_panel_t *)self->panel_handle, rgb_panel_t, base);

        self->sw_rot.buffers.active = rgb_panel->fbs[0];
        self->sw_rot.buffers.idle = rgb_panel->fbs[1];

        free(self->panel_io_config);

        return true;
    }


    static void rgb_flush_cb(void *self_in, int cmd, uint8_t last_update, uint8_t *idle_fb)
    {
        LCD_UNUSED(cmd);

        if (last_update) {
            mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)self_in;
            mp_lcd_sw_rotation_data_t *data = &self->sw_rot.data;

            mp_lcd_err_t ret = esp_lcd_panel_draw_bitmap(
                self->panel_handle,
                0,
                0,
                data->dst_width - 1,
                data->dst_height - 1,
                idle_fb
            );

            if (ret != LCD_OK) {
                mp_printf(&mp_plat_print, "esp_lcd_panel_draw_bitmap error (%d)\n", ret);
            } else {
                mp_lcd_sw_rotation_handles_t *handles = &self->sw_rot.handles;
                mp_lcd_sw_rotation_buffers_t *buffers = &self->sw_rot.buffers;

                mp_lcd_event_clear(&handles->swap_bufs);
                mp_lcd_event_wait(&handles->swap_bufs);

                memcpy(buffers->idle, buffers->active,
                       data->dst_width * data->dst_height * data->bytes_per_pixel);
            }
        }
    }


    static mp_obj_t rgb_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        enum {
            ARG_hsync,
            ARG_vsync,
            ARG_de,
            ARG_pclk,
            ARG_data0,
            ARG_data1,
            ARG_data2,
            ARG_data3,
            ARG_data4,
            ARG_data5,
            ARG_data6,
            ARG_data7,
            ARG_data8,
            ARG_data9,
            ARG_data10,
            ARG_data11,
            ARG_data12,
            ARG_data13,
            ARG_data14,
            ARG_data15,
            ARG_freq,
            ARG_hsync_front_porch,
            ARG_hsync_back_porch,
            ARG_hsync_pulse_width,
            ARG_hsync_idle_low,
            ARG_vsync_front_porch,
            ARG_vsync_back_porch,
            ARG_vsync_pulse_width,
            ARG_vsync_idle_low,
            ARG_de_idle_high,
            ARG_pclk_idle_high,
            ARG_pclk_active_low,
            ARG_refresh_on_demand
        };

        const mp_arg_t allowed_args[] = {
            { MP_QSTR_hsync,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_vsync,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_de,                 MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_pclk,               MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data0,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data1,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data2,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data3,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data4,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data5,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data6,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data7,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED     },
            { MP_QSTR_data8,              MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data9,              MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data10,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data11,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data12,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data13,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data14,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_data15,             MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = -1      } },
            { MP_QSTR_freq,               MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 8000000 } },
            { MP_QSTR_hsync_front_porch,  MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0       } },
            { MP_QSTR_hsync_back_porch,   MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0       } },
            { MP_QSTR_hsync_pulse_width,  MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 1       } },
            { MP_QSTR_hsync_idle_low,     MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } },
            { MP_QSTR_vsync_front_porch,  MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0       } },
            { MP_QSTR_vsync_back_porch,   MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 0       } },
            { MP_QSTR_vsync_pulse_width,  MP_ARG_INT  | MP_ARG_KW_ONLY, { .u_int = 1       } },
            { MP_QSTR_vsync_idle_low,     MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } },
            { MP_QSTR_de_idle_high,       MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } },
            { MP_QSTR_pclk_idle_high,     MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } },
            { MP_QSTR_pclk_active_low,    MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } },
            { MP_QSTR_refresh_on_demand,  MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false  } }
        };

        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

        // create new object
        mp_lcd_rgb_bus_obj_t *self = m_new_obj(mp_lcd_rgb_bus_obj_t);
        self->base.type = &mp_lcd_rgb_bus_type;

        self->callback = mp_const_none;

        self->panel_io_config = malloc(sizeof(esp_lcd_rgb_panel_config_t));

        esp_lcd_rgb_panel_config_t *panel_io_config = self->panel_io_config;

        esp_lcd_rgb_timing_t timings = {
            .pclk_hz = (uint32_t)args[ARG_freq].u_int,
            .hsync_pulse_width = (uint32_t)args[ARG_hsync_pulse_width].u_int,
            .hsync_back_porch = (uint32_t)args[ARG_hsync_back_porch].u_int,
            .hsync_front_porch = (uint32_t)args[ARG_hsync_front_porch].u_int,
            .vsync_pulse_width = (uint32_t)args[ARG_vsync_pulse_width].u_int,
            .vsync_back_porch = (uint32_t)args[ARG_vsync_back_porch].u_int,
            .vsync_front_porch = (uint32_t)args[ARG_vsync_front_porch].u_int,
            .flags.hsync_idle_low = (uint32_t)args[ARG_hsync_idle_low].u_bool,
            .flags.vsync_idle_low = (uint32_t)args[ARG_vsync_idle_low].u_bool,
            .flags.de_idle_high = (uint32_t)args[ARG_de_idle_high].u_bool,
            .flags.pclk_active_neg = (uint32_t)args[ARG_pclk_active_low].u_bool,
            .flags.pclk_idle_high = (uint32_t)args[ARG_pclk_idle_high].u_bool
        };

        panel_io_config->clk_src = LCD_CLK_SRC_PLL160M;
        panel_io_config->timings = timings;
        panel_io_config->hsync_gpio_num = (int)args[ARG_hsync].u_int;
        panel_io_config->vsync_gpio_num = (int)args[ARG_vsync].u_int;
        panel_io_config->de_gpio_num = (int)args[ARG_de].u_int;
        panel_io_config->pclk_gpio_num = (int)args[ARG_pclk].u_int;
        panel_io_config->data_gpio_nums[0] = (int)args[ARG_data0].u_int;
        panel_io_config->data_gpio_nums[1] = (int)args[ARG_data1].u_int;
        panel_io_config->data_gpio_nums[2] = (int)args[ARG_data2].u_int;
        panel_io_config->data_gpio_nums[3] = (int)args[ARG_data3].u_int;
        panel_io_config->data_gpio_nums[4] = (int)args[ARG_data4].u_int;
        panel_io_config->data_gpio_nums[5] = (int)args[ARG_data5].u_int;
        panel_io_config->data_gpio_nums[6] = (int)args[ARG_data6].u_int;
        panel_io_config->data_gpio_nums[7] = (int)args[ARG_data7].u_int;
        panel_io_config->data_gpio_nums[8] = (int)args[ARG_data8].u_int;
        panel_io_config->data_gpio_nums[9] = (int)args[ARG_data9].u_int;
        panel_io_config->data_gpio_nums[10] = (int)args[ARG_data10].u_int;
        panel_io_config->data_gpio_nums[11] = (int)args[ARG_data11].u_int;
        panel_io_config->data_gpio_nums[12] = (int)args[ARG_data12].u_int;
        panel_io_config->data_gpio_nums[13] = (int)args[ARG_data13].u_int;
        panel_io_config->data_gpio_nums[14] = (int)args[ARG_data14].u_int;
        panel_io_config->data_gpio_nums[15] = (int)args[ARG_data15].u_int;
        panel_io_config->disp_gpio_num = -1;   // -1 means no GPIO is assigned to this function
        panel_io_config->sram_trans_align = 8;
        panel_io_config->psram_trans_align = 64;
        panel_io_config->flags.refresh_on_demand = (uint32_t)args[ARG_refresh_on_demand].u_bool;
        panel_io_config->flags.fb_in_psram = 0;
        panel_io_config->flags.double_fb = 0;

        int i = 0;
        for (; i < 16; i++) {
            if (panel_io_config->data_gpio_nums[i] == -1) {
                break;
            }
        }

        panel_io_config->data_width = (size_t) i;
        self->lanes = (uint8_t)i;

        LCD_DEBUG_PRINT("pclk_hz=%lu\n", timings.pclk_hz)
        LCD_DEBUG_PRINT("hsync_pulse_width=%lu\n", timings.hsync_pulse_width)
        LCD_DEBUG_PRINT("hsync_back_porch=%lu\n", timings.hsync_back_porch)
        LCD_DEBUG_PRINT("hsync_front_porch=%lu\n", timings.hsync_front_porch)
        LCD_DEBUG_PRINT("vsync_pulse_width=%lu\n", timings.vsync_pulse_width)
        LCD_DEBUG_PRINT("vsync_back_porch=%lu\n", timings.vsync_back_porch)
        LCD_DEBUG_PRINT("vsync_front_porch=%lu\n", timings.vsync_front_porch)
        LCD_DEBUG_PRINT("hsync_idle_low=%d\n", timings.flags.hsync_idle_low)
        LCD_DEBUG_PRINT("vsync_idle_low=%d\n", timings.flags.vsync_idle_low)
        LCD_DEBUG_PRINT("de_idle_high=%d\n", timings.flags.de_idle_high)
        LCD_DEBUG_PRINT("pclk_active_neg=%d\n", timings.flags.pclk_active_neg)
        LCD_DEBUG_PRINT("pclk_idle_high=%d\n", timings.flags.pclk_idle_high)
        LCD_DEBUG_PRINT("clk_src=%d\n", panel_io_config->clk_src)
        LCD_DEBUG_PRINT("hsync_gpio_num=%d\n", panel_io_config->hsync_gpio_num)
        LCD_DEBUG_PRINT("vsync_gpio_num=%d\n", panel_io_config->vsync_gpio_num)
        LCD_DEBUG_PRINT("de_gpio_num=%d\n", panel_io_config->de_gpio_num)
        LCD_DEBUG_PRINT("pclk_gpio_num=%d\n", panel_io_config->pclk_gpio_num)
        LCD_DEBUG_PRINT("data_gpio_nums[0]=%d\n", panel_io_config->data_gpio_nums[0])
        LCD_DEBUG_PRINT("data_gpio_nums[1]=%d\n", panel_io_config->data_gpio_nums[1])
        LCD_DEBUG_PRINT("data_gpio_nums[2]=%d\n", panel_io_config->data_gpio_nums[2])
        LCD_DEBUG_PRINT("data_gpio_nums[3]=%d\n", panel_io_config->data_gpio_nums[3])
        LCD_DEBUG_PRINT("data_gpio_nums[4]=%d\n", panel_io_config->data_gpio_nums[4])
        LCD_DEBUG_PRINT("data_gpio_nums[5]=%d\n", panel_io_config->data_gpio_nums[5])
        LCD_DEBUG_PRINT("data_gpio_nums[6]=%d\n", panel_io_config->data_gpio_nums[6])
        LCD_DEBUG_PRINT("data_gpio_nums[7]=%d\n", panel_io_config->data_gpio_nums[7])
        LCD_DEBUG_PRINT("data_gpio_nums[8]=%d\n", panel_io_config->data_gpio_nums[8])
        LCD_DEBUG_PRINT("data_gpio_nums[9]=%d\n", panel_io_config->data_gpio_nums[9])
        LCD_DEBUG_PRINT("data_gpio_nums[10]=%d\n", panel_io_config->data_gpio_nums[10])
        LCD_DEBUG_PRINT("data_gpio_nums[11]=%d\n", panel_io_config->data_gpio_nums[11])
        LCD_DEBUG_PRINT("data_gpio_nums[12]=%d\n", panel_io_config->data_gpio_nums[12])
        LCD_DEBUG_PRINT("data_gpio_nums[13]=%d\n", panel_io_config->data_gpio_nums[13])
        LCD_DEBUG_PRINT("data_gpio_nums[14]=%d\n", panel_io_config->data_gpio_nums[14])
        LCD_DEBUG_PRINT("data_gpio_nums[15]=%d\n", panel_io_config->data_gpio_nums[15])
        LCD_DEBUG_PRINT("sram_trans_align=%d\n", panel_io_config->sram_trans_align)
        LCD_DEBUG_PRINT("psram_trans_align=%d\n", panel_io_config->psram_trans_align)
        LCD_DEBUG_PRINT("refresh_on_demand=%d\n", panel_io_config->flags.refresh_on_demand)
        LCD_DEBUG_PRINT("fb_in_psram=%d\n", panel_io_config->flags.fb_in_psram)
        LCD_DEBUG_PRINT("double_fb=%d\n", panel_io_config->flags.double_fb)
        LCD_DEBUG_PRINT("data_width=%d\n", panel_io_config->data_width)

        self->panel_io_handle.del = &rgb_del;
        self->panel_io_handle.init = &rgb_init;

        return MP_OBJ_FROM_PTR(self);
    }

    mp_lcd_err_t rgb_del(mp_obj_t obj)
    {
        LCD_DEBUG_PRINT("rgb_del(self)\n")

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)obj;

        if (self->panel_handle != NULL) {
            mp_lcd_err_t ret = esp_lcd_panel_del(self->panel_handle);

            if (ret != 0) {
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_del)"), ret);
            }
            self->panel_handle = NULL;

            return ret;
        } else {
            return LCD_FAIL;
        }
    }


    mp_lcd_err_t rgb_init(mp_obj_t obj, uint8_t cmd_bits, uint8_t param_bits)
    {
        LCD_UNUSED(cmd_bits);
        LCD_UNUSED(param_bits);

        mp_lcd_rgb_bus_obj_t *self = (mp_lcd_rgb_bus_obj_t *)MP_OBJ_TO_PTR(obj);
        mp_lcd_sw_rotation_data_t *data = &self->sw_rot.data;

        self->sw_rotate = 1;

        if (data->bytes_per_pixel != 2) data->rgb565_swap = 0;

        if (data->rgb565_swap && self->panel_io_config->data_width == 16) {
            /*
            We change the pins aound when the bus width is 16 and wanting to
            swap bytes. This does the same thing as moving the bytes around in
            the buffer but without having to iterate over the entire buffer to
            swap the bytes around. Swapping the bytes around on larger displays
            has a serious performance impact on the speed. Moving the pins
            eliminates the need to do that.
            */
            int temp_pin;

            for (uint8_t i = 0; i < 8; i++) {
                temp_pin = self->panel_io_config->data_gpio_nums[i];
                self->panel_io_config->data_gpio_nums[i] = self->panel_io_config->data_gpio_nums[i + 8];
                self->panel_io_config->data_gpio_nums[i + 8] = temp_pin;
            }

            data->rgb565_swap = 0;
        }

        self->panel_io_config->timings.h_res = data->dst_width;
        self->panel_io_config->timings.v_res = data->dst_height;
        self->panel_io_config->bits_per_pixel = data->bytes_per_pixel * 8;
        self->panel_io_config->flags.fb_in_psram = 1;
        self->panel_io_config->flags.double_fb = 1;

        self->sw_rot.init.cb = &rgb_init_cb;
        self->sw_rot.flush_cb = &rgb_flush_cb;

        uint8_t *tmp_buf = (uint8_t *)malloc(1);
        self->sw_rot.buffers.active = tmp_buf;
        self->sw_rot.buffers.idle = tmp_buf;

        LCD_DEBUG_PRINT("h_res=%lu\n", self->panel_io_config->timings.h_res)
        LCD_DEBUG_PRINT("v_res=%lu\n", self->panel_io_config->timings.v_res)
        LCD_DEBUG_PRINT("bits_per_pixel=%d\n", self->panel_io_config->bits_per_pixel)
        LCD_DEBUG_PRINT("rgb565_byte_swap=%d\n", data->rgb565_swap)

        return LCD_OK;
    }


    MP_DEFINE_CONST_OBJ_TYPE(
        mp_lcd_rgb_bus_type,
        MP_QSTR_RGBBus,
        MP_TYPE_FLAG_NONE,
        make_new, rgb_make_new,
        locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
    );

#else
    #include "../other_mcus/rgb_bus.c"
#endif /*SOC_LCD_RGB_SUPPORTED*/
