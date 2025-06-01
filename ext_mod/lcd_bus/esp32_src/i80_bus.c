// Copyright (c) 2024 - 2025 Kevin G. Schlosser


// local includes
#include "lcd_types.h"
#include "lcd_bus_task.h"
#include "modlcd_bus.h"
#include "i80_bus.h"
#include "bus_trans_done.h"

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
    #include "esp_task.h"

    #ifndef SOC_LCD_I80_BUS_WIDTH
        #define SOC_LCD_I80_BUS_WIDTH (16)
    #endif


    mp_lcd_err_t i80_del(mp_lcd_bus_obj_t *self_in);
    mp_lcd_err_t i80_init(mp_lcd_bus_obj_t *self_in, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,
                          uint8_t cmd_bits, uint8_t param_bits);

    static void i80_send_func(mp_lcd_bus_obj_t *self_in, int cmd, uint8_t *params, size_t params_len)
    {
        mp_lcd_i80_bus_obj_t *self = (mp_lcd_i80_bus_obj_t *)self_in;
        esp_lcd_panel_io_tx_param(self->panel_io_handle, cmd, params, params_len);
    }

    static void i80_flush_func(mp_lcd_bus_obj_t *self_in, rotation_data_t *r_data, rotation_data_t *original_r_data,
                               uint8_t *idle_fb, uint8_t last_update)
    {

        mp_lcd_i80_bus_obj_t *self = (mp_lcd_i80_bus_obj_t *)self_in;
        if (self->bufs.partial != NULL) {
            if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
                cb_isr(self->callback);
            }
            self->trans_done = 1;
        }

        if (last_update) {
            esp_err_t ret = esp_lcd_panel_io_tx_color(self->panel_io_handle, r_data->cmd, idle_fb, r_data->color_size);

            if (ret != 0) {
                mp_printf(&mp_plat_print, "esp_lcd_panel_io_tx_color error (%d)\n", ret);
            } else if (self->bufs.partial != NULL) {
                uint8_t *temp_buf = self->bufs.active;
                self->bufs.active = idle_fb;
                self->bufs.idle = temp_buf;
            }
        }
    }


    static bool i80_init_func(mp_lcd_bus_obj_t *self_in)
    {
        mp_lcd_i80_bus_obj_t *self = (mp_lcd_i80_bus_obj_t *) self_in;

        self->init.err = esp_lcd_new_i80_bus(&self->bus_config, &self->bus_handle);

        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_i80_bus)");
            return false;
        }

        self->init.err = esp_lcd_new_panel_io_i80(self->bus_handle, &self->panel_io_config, &self->panel_io_handle);
        if (self->init.err != 0) {
            self->init.err_msg = MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_i80)");
            return false;
        }

        return true;
    }


    static mp_obj_t mp_lcd_i80_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        enum { ARG_dc, ARG_wr, ARG_data_pins, ARG_cs, ARG_freq, ARG_dc_idle_high, ARG_dc_cmd_high, ARG_dc_dummy_high,
               ARG_dc_data_high, ARG_cs_active_high, ARG_reverse_color_bits, ARG_pclk_active_low, ARG_pclk_idle_low };

        const mp_arg_t make_new_args[] = {
            { MP_QSTR_dc,                 MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_wr,                 MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_data_pins,          MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
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
        mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(make_new_args), make_new_args, args);
    
        // create new object
        mp_lcd_i80_bus_obj_t *self = m_new_obj(mp_lcd_i80_bus_obj_t);
        self->base.type = &mp_lcd_i80_bus_type;
    
        self->callback = mp_const_none;
        self->panel_io_handle = NULL;

        self->bus_config.dc_gpio_num = (int)args[ARG_dc].u_int;
        self->bus_config.wr_gpio_num = (int)args[ARG_wr].u_int;
        self->bus_config.clk_src = LCD_CLK_SRC_PLL160M;

        mp_obj_tuple_t *tuple = MP_OBJ_TO_PTR(args[ARG_data_pins].u_obj);
        size_t i = 0;

        if (tuple->len != 8 && tuple->len != 16) {
            mp_raise_msg(&mp_type_ValueError, MP_ERROR_TEXT("8 or 16 data pins are required"));
        }

        for (; i < tuple->len; i++) {
            self->bus_config.data_gpio_nums[i] = (int)mp_obj_get_int(tuple->items[i]);
        }

        for (i++; i < SOC_LCD_I80_BUS_WIDTH; i++) {
            self->bus_config.data_gpio_nums[i] = -1;
        }

        self->bus_config.bus_width = tuple->len;
        self->num_lanes = (uint8_t)tuple->len;

        self->panel_io_config.cs_gpio_num = (int)args[ARG_cs].u_int;
        self->panel_io_config.pclk_hz = (uint32_t)args[ARG_freq].u_int;
        self->panel_io_config.trans_queue_depth = 5;
        self->panel_io_config.on_color_trans_done = &bus_trans_done_cb;
        self->panel_io_config.user_ctx = self;
        self->panel_io_config.dc_levels.dc_idle_level = (unsigned int)args[ARG_dc_idle_high].u_bool;
        self->panel_io_config.dc_levels.dc_cmd_level = (unsigned int)args[ARG_dc_cmd_high].u_bool;
        self->panel_io_config.dc_levels.dc_dummy_level = (unsigned int)args[ARG_dc_dummy_high].u_bool;
        self->panel_io_config.dc_levels.dc_data_level = (unsigned int)args[ARG_dc_data_high].u_bool;
        self->panel_io_config.flags.cs_active_high = (unsigned int)args[ARG_cs_active_high].u_bool;
        self->panel_io_config.flags.reverse_color_bits = (unsigned int)args[ARG_reverse_color_bits].u_bool;
        self->panel_io_config.flags.pclk_active_neg = (unsigned int)args[ARG_pclk_active_low].u_bool;
        self->panel_io_config.flags.pclk_idle_low = (unsigned int)args[ARG_pclk_idle_low].u_bool;

        LCD_DEBUG_PRINT("dc_gpio_num=%d\n", self->bus_config.dc_gpio_num)
        LCD_DEBUG_PRINT("wr_gpio_num=%d\n", self->bus_config.wr_gpio_num)
        LCD_DEBUG_PRINT("clk_src=%d\n", self->bus_config.clk_src)
        LCD_DEBUG_PRINT("data_gpio_nums[0]=%d\n", self->bus_config.data_gpio_nums[0])
        LCD_DEBUG_PRINT("data_gpio_nums[1]=%d\n", self->bus_config.data_gpio_nums[1])
        LCD_DEBUG_PRINT("data_gpio_nums[2]=%d\n", self->bus_config.data_gpio_nums[2])
        LCD_DEBUG_PRINT("data_gpio_nums[3]=%d\n", self->bus_config.data_gpio_nums[3])
        LCD_DEBUG_PRINT("data_gpio_nums[4]=%d\n", self->bus_config.data_gpio_nums[4])
        LCD_DEBUG_PRINT("data_gpio_nums[5]=%d\n", self->bus_config.data_gpio_nums[5])
        LCD_DEBUG_PRINT("data_gpio_nums[6]=%d\n", self->bus_config.data_gpio_nums[6])
        LCD_DEBUG_PRINT("data_gpio_nums[7]=%d\n", self->bus_config.data_gpio_nums[7])
        LCD_DEBUG_PRINT("data_gpio_nums[8]=%d\n", self->bus_config.data_gpio_nums[8])
        LCD_DEBUG_PRINT("data_gpio_nums[9]=%d\n", self->bus_config.data_gpio_nums[9])
        LCD_DEBUG_PRINT("data_gpio_nums[10]=%d\n", self->bus_config.data_gpio_nums[10])
        LCD_DEBUG_PRINT("data_gpio_nums[11]=%d\n", self->bus_config.data_gpio_nums[11])
        LCD_DEBUG_PRINT("data_gpio_nums[12]=%d\n", self->bus_config.data_gpio_nums[12])
        LCD_DEBUG_PRINT("data_gpio_nums[13]=%d\n", self->bus_config.data_gpio_nums[13])
        LCD_DEBUG_PRINT("data_gpio_nums[14]=%d\n", self->bus_config.data_gpio_nums[14])
        LCD_DEBUG_PRINT("data_gpio_nums[15]=%d\n", self->bus_config.data_gpio_nums[15])
        LCD_DEBUG_PRINT("bus_width=%d\n", self->bus_config.bus_width)
        LCD_DEBUG_PRINT("cs_gpio_num=%d\n", self->panel_io_config.cs_gpio_num)
        LCD_DEBUG_PRINT("pclk_hz=%lu\n", self->panel_io_config.pclk_hz)
        LCD_DEBUG_PRINT("trans_queue_depth=%d\n", self->panel_io_config.trans_queue_depth)
        LCD_DEBUG_PRINT("dc_idle_level=%d\n", self->panel_io_config.dc_levels.dc_idle_level)
        LCD_DEBUG_PRINT("dc_cmd_level=%d\n", self->panel_io_config.dc_levels.dc_cmd_level)
        LCD_DEBUG_PRINT("dc_dummy_level=%d\n", self->panel_io_config.dc_levels.dc_dummy_level)
        LCD_DEBUG_PRINT("dc_data_level=%d\n", self->panel_io_config.dc_levels.dc_data_level)
        LCD_DEBUG_PRINT("cs_active_high=%d\n", self->panel_io_config.flags.cs_active_high)
        LCD_DEBUG_PRINT("reverse_color_bits=%d\n", self->panel_io_config.flags.reverse_color_bits)
        LCD_DEBUG_PRINT("pclk_active_neg=%d\n", self->panel_io_config.flags.pclk_active_neg)
        LCD_DEBUG_PRINT("pclk_idle_low=%d\n", self->panel_io_config.flags.pclk_idle_low)

        self->internal_cb_funcs.init = &i80_init;
        self->internal_cb_funcs.deinit = &i80_del;

        self->tx_data.flush_func = &i80_flush_func;
        self->init.init_func = &i80_init_func;

        self->tx_cmds.send_func = &i80_send_func;

        return MP_OBJ_FROM_PTR(self);
    }


    mp_lcd_err_t i80_init(mp_lcd_bus_obj_t *self_in, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size,
                          uint8_t cmd_bits, uint8_t param_bits)
    {
        LCD_DEBUG_PRINT("i80_init(self, width=%i, height=%i, bpp=%i, buffer_size=%lu, cmd_bits=%i, param_bits=%i)\n",
                        width, height, bpp, buffer_size, cmd_bits, param_bits)

        mp_lcd_i80_bus_obj_t *self = (mp_lcd_i80_bus_obj_t *)self_in;

        if (self->panel_io_handle != NULL) {
            return LCD_FAIL;
        }

        LCD_DEBUG_PRINT("rgb565_byte_swap=%i\n",  (uint8_t)self->r_data.swap)

        if (self->r_data.swap) {
            self->panel_io_config.flags.swap_color_bytes = 1;
            self->r_data.swap = false;
        }

        self->panel_io_config.lcd_cmd_bits = (int)cmd_bits;
        self->panel_io_config.lcd_param_bits = (int)param_bits;
        self->bus_config.max_transfer_bytes = (size_t)buffer_size;

        LCD_DEBUG_PRINT("lcd_cmd_bits=%d\n", self->panel_io_config.lcd_cmd_bits)
        LCD_DEBUG_PRINT("lcd_param_bits=%d\n", self->panel_io_config.lcd_param_bits)
        LCD_DEBUG_PRINT("max_transfer_bytes=%d\n", self->bus_config.max_transfer_bytes)

        xTaskCreatePinnedToCore(
                lcd_bus_task, "i80_task", LCD_DEFAULT_STACK_SIZE / sizeof(StackType_t),
                self, ESP_TASK_PRIO_MAX - 1, (TaskHandle_t *)&self->task.handle, 0);

        lcd_bus_lock_acquire(self->init.lock);
        lcd_bus_lock_release(self->init.lock);
        lcd_bus_lock_delete(self->init.lock);

        return self->init.err;
    }


    mp_lcd_err_t i80_del(mp_lcd_bus_obj_t *self_in)
    {
        LCD_DEBUG_PRINT("i80_del(self)\n")

        mp_lcd_i80_bus_obj_t *self = (mp_lcd_i80_bus_obj_t *)self_in;

        if (self->panel_io_handle != NULL) {
            mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle);
            if (ret != 0) {
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
                return ret;
            }

            ret = esp_lcd_del_i80_bus(self->bus_handle);
            if (ret != 0) {
                mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_del_i80_bus)"), ret);
                return ret;
            }

            self->panel_io_handle = NULL;

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
