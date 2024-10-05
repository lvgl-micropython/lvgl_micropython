// local includes
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "i80_bus.h"

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
    mp_lcd_err_t i80_get_lane_count(mp_obj_t obj, uint8_t *lane_count);

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

        self->bus_config.dc_gpio_num = (int)args[ARG_dc].u_int;
        self->bus_config.wr_gpio_num = (int)args[ARG_wr].u_int;
        self->bus_config.clk_src = LCD_CLK_SRC_PLL160M;
        self->bus_config.data_gpio_nums[0] = args[ARG_data0].u_int;
        self->bus_config.data_gpio_nums[1] = args[ARG_data1].u_int;
        self->bus_config.data_gpio_nums[2] = args[ARG_data2].u_int;
        self->bus_config.data_gpio_nums[3] = args[ARG_data3].u_int;
        self->bus_config.data_gpio_nums[4] = args[ARG_data4].u_int;
        self->bus_config.data_gpio_nums[5] = args[ARG_data5].u_int;
        self->bus_config.data_gpio_nums[6] = args[ARG_data6].u_int;
        self->bus_config.data_gpio_nums[7] = args[ARG_data7].u_int;
        self->bus_config.data_gpio_nums[8] = args[ARG_data8].u_int;
        self->bus_config.data_gpio_nums[9] = args[ARG_data9].u_int;
        self->bus_config.data_gpio_nums[10] = args[ARG_data10].u_int;
        self->bus_config.data_gpio_nums[11] = args[ARG_data11].u_int;
        self->bus_config.data_gpio_nums[12] = args[ARG_data12].u_int;
        self->bus_config.data_gpio_nums[13] = args[ARG_data13].u_int;
        self->bus_config.data_gpio_nums[14] = args[ARG_data14].u_int;
        self->bus_config.data_gpio_nums[15] = args[ARG_data15].u_int;

        uint8_t i = 0;
        for (; i < SOC_LCD_I80_BUS_WIDTH; i++) {
            if (self->bus_config.data_gpio_nums[i] == -1) {
                break;
            }
        }

        self->bus_config.bus_width = (size_t) i;

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

    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("dc_gpio_num=%d\n", self->bus_config.dc_gpio_num);
        printf("wr_gpio_num=%d\n", self->bus_config.wr_gpio_num);
        printf("clk_src=%d\n", self->bus_config.clk_src);
        printf("data_gpio_nums[0]=%d\n", self->bus_config.data_gpio_nums[0]);
        printf("data_gpio_nums[1]=%d\n", self->bus_config.data_gpio_nums[1]);
        printf("data_gpio_nums[2]=%d\n", self->bus_config.data_gpio_nums[2]);
        printf("data_gpio_nums[3]=%d\n", self->bus_config.data_gpio_nums[3]);
        printf("data_gpio_nums[4]=%d\n", self->bus_config.data_gpio_nums[4]);
        printf("data_gpio_nums[5]=%d\n", self->bus_config.data_gpio_nums[5]);
        printf("data_gpio_nums[6]=%d\n", self->bus_config.data_gpio_nums[6]);
        printf("data_gpio_nums[7]=%d\n", self->bus_config.data_gpio_nums[7]);
        printf("data_gpio_nums[8]=%d\n", self->bus_config.data_gpio_nums[8]);
        printf("data_gpio_nums[9]=%d\n", self->bus_config.data_gpio_nums[9]);
        printf("data_gpio_nums[10]=%d\n", self->bus_config.data_gpio_nums[10]);
        printf("data_gpio_nums[11]=%d\n", self->bus_config.data_gpio_nums[11]);
        printf("data_gpio_nums[12]=%d\n", self->bus_config.data_gpio_nums[12]);
        printf("data_gpio_nums[13]=%d\n", self->bus_config.data_gpio_nums[13]);
        printf("data_gpio_nums[14]=%d\n", self->bus_config.data_gpio_nums[14]);
        printf("data_gpio_nums[15]=%d\n", self->bus_config.data_gpio_nums[15]);
        printf("bus_width=%d\n", self->bus_config.bus_width);
        printf("cs_gpio_num=%d\n", self->panel_io_config.cs_gpio_num);
        printf("pclk_hz=%lu\n", self->panel_io_config.pclk_hz);
        printf("trans_queue_depth=%d\n", self->panel_io_config.trans_queue_depth);
        printf("dc_idle_level=%d\n", self->panel_io_config.dc_levels.dc_idle_level);
        printf("dc_cmd_level=%d\n", self->panel_io_config.dc_levels.dc_cmd_level);
        printf("dc_dummy_level=%d\n", self->panel_io_config.dc_levels.dc_dummy_level);
        printf("dc_data_level=%d\n", self->panel_io_config.dc_levels.dc_data_level);
        printf("cs_active_high=%d\n", self->panel_io_config.flags.cs_active_high);
        printf("reverse_color_bits=%d\n", self->panel_io_config.flags.reverse_color_bits);
        printf("pclk_active_neg=%d\n", self->panel_io_config.flags.pclk_active_neg);
        printf("pclk_idle_low=%d\n", self->panel_io_config.flags.pclk_idle_low);
    #endif

        self->panel_io_handle.init = &i80_init;
        self->panel_io_handle.del = &i80_del;
        self->panel_io_handle.get_lane_count = &i80_get_lane_count;

        return MP_OBJ_FROM_PTR(self);
    }
    

    mp_lcd_err_t i80_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits)
    {
    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("i80_init(self, width=%i, height=%i, bpp=%i, buffer_size=%lu, rgb565_byte_swap=%i, cmd_bits=%i, param_bits=%i)\n", width, height, bpp, buffer_size, (uint8_t)rgb565_byte_swap, cmd_bits, param_bits);
    #endif

        mp_lcd_i80_bus_obj_t *self = (mp_lcd_i80_bus_obj_t *)obj;
        self->rgb565_byte_swap = false;

        if (rgb565_byte_swap && bpp == 16) {
            self->panel_io_config.flags.swap_color_bytes = 1;
        }

        self->panel_io_config.lcd_cmd_bits = (int)cmd_bits;
        self->panel_io_config.lcd_param_bits = (int)param_bits;
        self->bus_config.max_transfer_bytes = (size_t)buffer_size;

    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("lcd_cmd_bits=%d\n", self->panel_io_config.lcd_cmd_bits);
        printf("lcd_param_bits=%d\n", self->panel_io_config.lcd_param_bits);
        printf("max_transfer_bytes=%d\n", self->bus_config.max_transfer_bytes);
    #endif
        esp_err_t ret = esp_lcd_new_i80_bus(&self->bus_config, &self->bus_handle);

        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_i80_bus)"), ret);
        }

        ret = esp_lcd_new_panel_io_i80(self->bus_handle, &self->panel_io_config, &self->panel_io_handle.panel_io);

        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_new_panel_io_i80)"), ret);
        }

        return ret;
    }


    mp_lcd_err_t i80_del(mp_obj_t obj)
    {
    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("i80_del(self)\n");
    #endif

        mp_lcd_i80_bus_obj_t *self = (mp_lcd_i80_bus_obj_t *)obj;

        mp_lcd_err_t ret = esp_lcd_panel_io_del(self->panel_io_handle.panel_io);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_panel_io_del)"), ret);
        }

        ret = esp_lcd_del_i80_bus(self->bus_handle);
        if (ret != 0) {
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(esp_lcd_del_i80_bus)"), ret);
        }

        return ret;
    }

    mp_lcd_err_t i80_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
    {
        mp_lcd_i80_bus_obj_t *self = (mp_lcd_i80_bus_obj_t *)obj;
        *lane_count = (uint8_t)self->bus_config.bus_width;

    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("i80_get_lane_count(self)-> %d\n", (uint8_t)self->bus_config.bus_width);
    #endif

        return LCD_OK;
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
