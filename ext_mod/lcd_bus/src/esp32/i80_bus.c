// Copyright (c) 2024 - 2025 Kevin G. Schlosser


// local includes
#include "common/lcd_types.h"
#include "common/modlcd_bus.h"
#include "esp32/i80_bus.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>

// esp-idf includes
#include "soc/soc_caps.h"

#if SOC_LCD_I80_SUPPORTED
    // esp-idf includes
    #include "esp_lcd_panel_io.h"
    #include "esp_heap_caps.h"
    #include "hal/lcd_types.h"


    mp_lcd_err_t i80_del(mp_obj_t obj);
    mp_lcd_err_t i80_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);

    
    static bool i80_trans_done_cb(esp_lcd_panel_handle_t panel,
                            const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
    {
        mp_lcd_i80_bus_obj_t *self = (mp_lcd_i80_bus_obj_t *)user_ctx;
    
        if (self->trans_done == 0) {
            if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
                mp_lcd_flush_ready_cb(self->callback);
            }
            self->trans_done = 1;
        }
    
        return false;
    }


    static void i80_tx_param_cb(void* self_in, int cmd, uint8_t *params, size_t params_len)
    {
        esp_lcd_panel_io_tx_param(self->panel_io_handle.panel_io, cmd, params, params_len);
    }
    
    
    static bool i80_init_cb(void *self_in)
    {
        mp_lcd_i80_bus_obj_t *self = (mp_lcd_i80_bus_obj_t *)self_in;

        mp_lcd_sw_rotation_init_t *init = &self->sw_rot.init;

        init->err = esp_lcd_new_i80_bus(self->bus_config, &self->bus_handle);

        if (init->err != LCD_OK) {
            init->err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_i80_bus)");
            return false;
        }

        init->err = esp_lcd_new_panel_io_i80(self->bus_handle, self->panel_io_config, &self->panel_io_handle.panel_io);

        if (init->err !=LCD_OK) {
            init->err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_i80)");
            return false;
        }

        free(self->panel_io_config);
        free(self->bus_config);
        self->panel_io_config = NULL;
        self->bus_config = NULL;

        return true;
    }
    
    
    static void i80_flush_cb(void *self_in, uint8_t last_update, int cmd, uint8_t *idle_fb)
    {
        LCD_UNUSED(last_update);
        mp_lcd_i80_bus_obj_t *self = (mp_lcd_i80_bus_obj_t *)self_in;
        mp_lcd_sw_rotation_buffers_t *buffers = &self->sw_rot.buffers;

    
        if (idle_fb == buffers->idle) {
            buffers->idle = buffers->active;
            buffers->active = idle_fb;
        }
    
        mp_lcd_err_t ret = esp_lcd_panel_io_tx_color(self->panel_io_handle.panel_io, cmd, idle_fb, self->fb1->len);
    
        if (ret != LCD_OK) {
            mp_printf(&mp_plat_print, "esp_lcd_panel_draw_bitmap error (%d)\n", ret);
        }
    }


    static mp_obj_t mp_lcd_i80_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
    
        enum {
            ARG_dc,
            ARG_wr,
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
            ARG_cs,
            ARG_freq,
            ARG_dc_idle_high,
            ARG_dc_cmd_high,
            ARG_dc_dummy_high,
            ARG_dc_data_high,
            ARG_cs_active_high,
            ARG_reverse_color_bits,
            ARG_pclk_active_low,
            ARG_pclk_idle_low,
        };

        const mp_arg_t make_new_args[] = {
            { MP_QSTR_dc,                 MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_wr,                 MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_data0,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_data1,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_data2,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_data3,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_data4,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_data5,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_data6,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_data7,              MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_data8,              MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = -1       } },
            { MP_QSTR_data9,              MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = -1       } },
            { MP_QSTR_data10,             MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = -1       } },
            { MP_QSTR_data11,             MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = -1       } },
            { MP_QSTR_data12,             MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = -1       } },
            { MP_QSTR_data13,             MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = -1       } },
            { MP_QSTR_data14,             MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = -1       } },
            { MP_QSTR_data15,             MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = -1       } },
            { MP_QSTR_cs,                 MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = -1       } },
            { MP_QSTR_freq,               MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = 10000000 } },
            { MP_QSTR_dc_idle_high,       MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false  } },
            { MP_QSTR_dc_cmd_high,        MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false  } },
            { MP_QSTR_dc_dummy_high,      MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false  } },
            { MP_QSTR_dc_data_high,       MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = true   } },
            { MP_QSTR_cs_active_high,     MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false   } },
            { MP_QSTR_reverse_color_bits, MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false   } },
            { MP_QSTR_pclk_active_low,    MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false   } },
            { MP_QSTR_pclk_idle_low,      MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false   } }
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
        mp_lcd_i80_bus_obj_t *self = m_new_obj(mp_lcd_i80_bus_obj_t);
        self->base.type = &mp_lcd_i80_bus_type;
    
        self->callback = mp_const_none;

        self->panel_io_config = malloc(sizeof(esp_lcd_panel_io_i80_config_t));
        esp_lcd_panel_io_i80_config_t *panel_io_config = self->panel_io_config;
        
        self->bus_config = malloc(sizeof(esp_lcd_i80_bus_config_t));
        esp_lcd_i80_bus_config_t *bus_config = self->bus_config

        bus_config->dc_gpio_num = (int)args[ARG_dc].u_int;
        bus_config->wr_gpio_num = (int)args[ARG_wr].u_int;
        bus_config->clk_src = LCD_CLK_SRC_PLL160M;
        bus_config->data_gpio_nums[0] = args[ARG_data0].u_int;
        bus_config->data_gpio_nums[1] = args[ARG_data1].u_int;
        bus_config->data_gpio_nums[2] = args[ARG_data2].u_int;
        bus_config->data_gpio_nums[3] = args[ARG_data3].u_int;
        bus_config->data_gpio_nums[4] = args[ARG_data4].u_int;
        bus_config->data_gpio_nums[5] = args[ARG_data5].u_int;
        bus_config->data_gpio_nums[6] = args[ARG_data6].u_int;
        bus_config->data_gpio_nums[7] = args[ARG_data7].u_int;
        bus_config->data_gpio_nums[8] = args[ARG_data8].u_int;
        bus_config->data_gpio_nums[9] = args[ARG_data9].u_int;
        bus_config->data_gpio_nums[10] = args[ARG_data10].u_int;
        bus_config->data_gpio_nums[11] = args[ARG_data11].u_int;
        bus_config->data_gpio_nums[12] = args[ARG_data12].u_int;
        bus_config->data_gpio_nums[13] = args[ARG_data13].u_int;
        bus_config->data_gpio_nums[14] = args[ARG_data14].u_int;
        bus_config->data_gpio_nums[15] = args[ARG_data15].u_int;

        uint8_t i = 0;
        for (; i < SOC_LCD_I80_BUS_WIDTH; i++) {
            if (bus_config->data_gpio_nums[i] == -1) {
                break;
            }
        }

        bus_config->bus_width = (size_t) i;
        self->lanes = (uint8_t)i;

        panel_io_config->cs_gpio_num = (int)args[ARG_cs].u_int;
        panel_io_config->pclk_hz = (uint32_t)args[ARG_freq].u_int;
        panel_io_config->trans_queue_depth = 5;
        panel_io_config->on_color_trans_done = &bus_trans_done_cb;
        panel_io_config->user_ctx = self;
        panel_io_config->dc_levels.dc_idle_level = (unsigned int)args[ARG_dc_idle_high].u_bool;
        panel_io_config->dc_levels.dc_cmd_level = (unsigned int)args[ARG_dc_cmd_high].u_bool;
        panel_io_config->dc_levels.dc_dummy_level = (unsigned int)args[ARG_dc_dummy_high].u_bool;
        panel_io_config->dc_levels.dc_data_level = (unsigned int)args[ARG_dc_data_high].u_bool;
        panel_io_config->flags.cs_active_high = (unsigned int)args[ARG_cs_active_high].u_bool;
        panel_io_config->flags.reverse_color_bits = (unsigned int)args[ARG_reverse_color_bits].u_bool;
        panel_io_config->flags.pclk_active_neg = (unsigned int)args[ARG_pclk_active_low].u_bool;
        panel_io_config->flags.pclk_idle_low = (unsigned int)args[ARG_pclk_idle_low].u_bool;

        LCD_DEBUG_PRINT("dc_gpio_num=%d\n", bus_config->dc_gpio_num)
        LCD_DEBUG_PRINT("wr_gpio_num=%d\n", bus_config->wr_gpio_num)
        LCD_DEBUG_PRINT("clk_src=%d\n", bus_config->clk_src)
        LCD_DEBUG_PRINT("data_gpio_nums[0]=%d\n", bus_config->data_gpio_nums[0])
        LCD_DEBUG_PRINT("data_gpio_nums[1]=%d\n", bus_config->data_gpio_nums[1])
        LCD_DEBUG_PRINT("data_gpio_nums[2]=%d\n", bus_config->data_gpio_nums[2])
        LCD_DEBUG_PRINT("data_gpio_nums[3]=%d\n", bus_config->data_gpio_nums[3])
        LCD_DEBUG_PRINT("data_gpio_nums[4]=%d\n", bus_config->data_gpio_nums[4])
        LCD_DEBUG_PRINT("data_gpio_nums[5]=%d\n", bus_config->data_gpio_nums[5])
        LCD_DEBUG_PRINT("data_gpio_nums[6]=%d\n", bus_config->data_gpio_nums[6])
        LCD_DEBUG_PRINT("data_gpio_nums[7]=%d\n", bus_config->data_gpio_nums[7])
        LCD_DEBUG_PRINT("data_gpio_nums[8]=%d\n", bus_config->data_gpio_nums[8])
        LCD_DEBUG_PRINT("data_gpio_nums[9]=%d\n", bus_config->data_gpio_nums[9])
        LCD_DEBUG_PRINT("data_gpio_nums[10]=%d\n", bus_config->data_gpio_nums[10])
        LCD_DEBUG_PRINT("data_gpio_nums[11]=%d\n", bus_config->data_gpio_nums[11])
        LCD_DEBUG_PRINT("data_gpio_nums[12]=%d\n", bus_config->data_gpio_nums[12])
        LCD_DEBUG_PRINT("data_gpio_nums[13]=%d\n", bus_config->data_gpio_nums[13])
        LCD_DEBUG_PRINT("data_gpio_nums[14]=%d\n", bus_config->data_gpio_nums[14])
        LCD_DEBUG_PRINT("data_gpio_nums[15]=%d\n", bus_config->data_gpio_nums[15])
        LCD_DEBUG_PRINT("bus_width=%d\n", bus_config->bus_width)
        LCD_DEBUG_PRINT("cs_gpio_num=%d\n", panel_io_config->cs_gpio_num)
        LCD_DEBUG_PRINT("pclk_hz=%lu\n", panel_io_config->pclk_hz)
        LCD_DEBUG_PRINT("trans_queue_depth=%d\n", panel_io_config->trans_queue_depth)
        LCD_DEBUG_PRINT("dc_idle_level=%d\n", panel_io_config->dc_levels.dc_idle_level)
        LCD_DEBUG_PRINT("dc_cmd_level=%d\n", panel_io_config->dc_levels.dc_cmd_level)
        LCD_DEBUG_PRINT("dc_dummy_level=%d\n", panel_io_config->dc_levels.dc_dummy_level)
        LCD_DEBUG_PRINT("dc_data_level=%d\n", panel_io_config->dc_levels.dc_data_level)
        LCD_DEBUG_PRINT("cs_active_high=%d\n", panel_io_config->flags.cs_active_high)
        LCD_DEBUG_PRINT("reverse_color_bits=%d\n", panel_io_config->flags.reverse_color_bits)
        LCD_DEBUG_PRINT("pclk_active_neg=%d\n", panel_io_config->flags.pclk_active_neg)
        LCD_DEBUG_PRINT("pclk_idle_low=%d\n", panel_io_config->flags.pclk_idle_low)

        self->panel_io_handle.init = &i80_init;
        self->panel_io_handle.del = &i80_del;

        return MP_OBJ_FROM_PTR(self);
    }
    

    mp_lcd_err_t i80_init(mp_obj_t obj, uint8_t cmd_bits, uint8_t param_bits)
    {
        LCD_DEBUG_PRINT("i80_init(self, cmd_bits=%i, param_bits=%i)\n", cmd_bits, param_bits)

        mp_lcd_i80_bus_obj_t *self = (mp_lcd_i80_bus_obj_t *)obj;

        if (self->panel_io_handle.panel_io != NULL) {
            return LCD_FAIL;
        }

        if (self->sw_rot.data.bytes_per_pixel != 2) {
            // self->panel_io_config.flags.swap_color_bytes = 1;
            self->sw_rot.data.rgb565_swap = 0;
        }

        self->sw_rot.data.dst_width = 0;
        self->sw_rot.data.dst_height = 0;
        self->sw_rot.init.cb = &i80_init_cb;
        self->sw_rot.flush_cb = &i80_flush_cb;
        self->sw_rot.tx_params.cb = &i80_tx_param_cb;

        self->panel_io_config->lcd_cmd_bits = (int)cmd_bits;
        self->panel_io_config->lcd_param_bits = (int)param_bits;
        self->bus_config->max_transfer_bytes = self->fb1->len;

        LCD_DEBUG_PRINT("lcd_cmd_bits=%d\n", self->panel_io_config->lcd_cmd_bits)
        LCD_DEBUG_PRINT("lcd_param_bits=%d\n", self->panel_io_config->lcd_param_bits)
        LCD_DEBUG_PRINT("max_transfer_bytes=%d\n", self->bus_config->max_transfer_bytes)

        return LCD_OK;
    }


    mp_lcd_err_t i80_del(mp_obj_t obj)
    {
        LCD_DEBUG_PRINT("i80_del(self)\n")

        mp_lcd_i80_bus_obj_t *self = (mp_lcd_i80_bus_obj_t *)obj;

        if (self->panel_io_handle.panel_io != NULL) {
            mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle.panel_io);
            if (ret != LCD_OK) {
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
                return ret;
            }

            ret = esp_lcd_del_i80_bus(self->bus_handle);
            if (ret != LCD_OK) {
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_del_i80_bus)"), ret);
                return ret;
            }

            self->panel_io_handle.panel_io = NULL;

            return ret;
        } else {
            return LCD_FAIL;
        }
    }

    MP_DEFINE_CONST_OBJ_TYPE(
        mp_lcd_i80_bus_type,
        MP_QSTR_I80Bus,
        MP_TYPE_FLAG_NONE,
        make_new, mp_lcd_i80_bus_make_new,
        locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
    );

#else
    #include "../common_src/i80_bus.c"

#endif /*SOC_LCD_I80_SUPPORTED*/
