// Copyright (c) 2024 - 2025 Kevin G. Schlosser

/* includes */
// local includes
#include "lcd_types.h"
#include "modlcd_bus.h"
#include "../common_include/i80_bus.h"

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objarray.h"

//#include "mpconfigport.h"
//#include "modmachine.h"

// stdlib includes
#include <string.h>
/* end includes */


#ifdef MP_PORT_UNIX
    static mp_obj_t mp_lcd_i80_bus_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        LCD_UNUSED(type);
        LCD_UNUSED(n_args);
        LCD_UNUSED(n_kw);
        LCD_UNUSED(all_args);

        mp_raise_msg(&mp_type_NotImplementedError, MP_ERROR_TEXT("I80 display bus is not supported"));
        return mp_const_none;
    }
#else
    /* macros */
    #define CS_LOW()  { if (self->panel_io_config.cs_gpio_num) mp_hal_pin_write(self->panel_io_config.cs_gpio_num, self->panel_io_config.flags.cs_active_high);  }
    #define CS_HIGH() { if (self->panel_io_config.cs_gpio_num) mp_hal_pin_write(self->panel_io_config.cs_gpio_num, !self->panel_io_config.flags.cs_active_high); }

    #define DC_CMD()  { mp_hal_pin_write(self->bus_config.dc_gpio_num, self->panel_io_config.dc_levels.dc_cmd_level);  }
    #define DC_DATA() { mp_hal_pin_write(self->bus_config.dc_gpio_num, self->panel_io_config.dc_levels.dc_data_level); }

    #define WR_LOW()  { mp_hal_pin_write(self->bus_config.wr_gpio_num, 0); }
    #define WR_HIGH() { mp_hal_pin_write(self->bus_config.wr_gpio_num, 1); }


    #define WRITE8() {                                                           \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[7], (buf[0] >> 7) & 1); \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[6], (buf[0] >> 6) & 1); \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[5], (buf[0] >> 5) & 1); \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[4], (buf[0] >> 4) & 1); \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[3], (buf[0] >> 3) & 1); \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[2], (buf[0] >> 2) & 1); \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[1], (buf[0] >> 1) & 1); \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[0], buf[0] & 1);        \
    }


    #define WRITE16() {                                                            \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[15], (buf[0] >> 15) & 1); \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[14], (buf[0] >> 14) & 1); \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[13], (buf[0] >> 13) & 1); \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[12], (buf[0] >> 12) & 1); \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[11], (buf[0] >> 11) & 1); \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[10], (buf[0] >> 10) & 1); \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[9], (buf[0] >> 9) & 1);   \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[8], (buf[0] >> 8) & 1);   \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[7], (buf[0] >> 7) & 1);   \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[6], (buf[0] >> 6) & 1);   \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[5], (buf[0] >> 5) & 1);   \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[4], (buf[0] >> 4) & 1);   \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[3], (buf[0] >> 3) & 1);   \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[2], (buf[0] >> 2) & 1);   \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[1], (buf[0] >> 1) & 1);   \
        mp_hal_pin_write(self->bus_config.data_gpio_nums[0], buf[0] & 1);          \
    }
    /* end macros */


    /* forward declarations */
    mp_lcd_err_t i80_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t i80_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size);
    mp_lcd_err_t i80_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update);
    mp_lcd_err_t i80_del(mp_obj_t obj);
    mp_lcd_err_t i80_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits);
    mp_lcd_err_t i80_get_lane_count(mp_obj_t obj, uint8_t *lane_count);

    void write_color8(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size);
    void write_color16(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size);
    void write_color_swap_bytes8(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size);
    void write_color_swap_bytes16(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size);
    void write_rgb565_swap8(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size);
    void write_rgb565_swap16(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size);
    /* end forward declarations */


    /* function definitions */
    /*
     * Because the implimentation is bitbang it is not going to be anywhere near
     * as fast as a hardware implimentation. There is also no DMA support so
     * using double buffering is going to be a waste. There is no data rate that
     * can be set since the code is going to run as fast as possible which is not
     * going to come close to what the maximum bitrate the display is able to use.
     */
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
            ARG_cmd_bits,
            ARG_param_bits,
            ARG_cs_active_high,
            ARG_reverse_color_bits,
            ARG_swap_color_bytes,
            ARG_pclk_active_low,
            ARG_pclk_idle_low,
        };
        /*
         * To keep the api consistant for portability reasons not all parameters
         * are used. The ones that are used are:
         * dc
         * wr
         * data0 - data15
         * cs
         * dc_cmd_high
         * dc_data_high
         * cmd_bits
         * param_bits
         * cs_active_high
         * reverse_color_bits
         * swap_color_bytes
         */
        const mp_arg_t make_new_args[] = {
            { MP_QSTR_dc,                 MP_ARG_OBJ  | MP_ARG_REQUIRED },
            { MP_QSTR_wr,                 MP_ARG_OBJ  | MP_ARG_REQUIRED },
            { MP_QSTR_data0,              MP_ARG_OBJ  | MP_ARG_REQUIRED },
            { MP_QSTR_data1,              MP_ARG_OBJ  | MP_ARG_REQUIRED },
            { MP_QSTR_data2,              MP_ARG_OBJ  | MP_ARG_REQUIRED },
            { MP_QSTR_data3,              MP_ARG_OBJ  | MP_ARG_REQUIRED },
            { MP_QSTR_data4,              MP_ARG_OBJ  | MP_ARG_REQUIRED },
            { MP_QSTR_data5,              MP_ARG_OBJ  | MP_ARG_REQUIRED },
            { MP_QSTR_data6,              MP_ARG_OBJ  | MP_ARG_REQUIRED },
            { MP_QSTR_data7,              MP_ARG_OBJ  | MP_ARG_REQUIRED },
            { MP_QSTR_data8,              MP_ARG_OBJ  | MP_ARG_KW_ONLY,  { .u_obj = mp_const_none } },
            { MP_QSTR_data9,              MP_ARG_OBJ  | MP_ARG_KW_ONLY,  { .u_obj = mp_const_none } },
            { MP_QSTR_data10,             MP_ARG_OBJ  | MP_ARG_KW_ONLY,  { .u_obj = mp_const_none } },
            { MP_QSTR_data11,             MP_ARG_OBJ  | MP_ARG_KW_ONLY,  { .u_obj = mp_const_none } },
            { MP_QSTR_data12,             MP_ARG_OBJ  | MP_ARG_KW_ONLY,  { .u_obj = mp_const_none } },
            { MP_QSTR_data13,             MP_ARG_OBJ  | MP_ARG_KW_ONLY,  { .u_obj = mp_const_none } },
            { MP_QSTR_data14,             MP_ARG_OBJ  | MP_ARG_KW_ONLY,  { .u_obj = mp_const_none } },
            { MP_QSTR_data15,             MP_ARG_OBJ  | MP_ARG_KW_ONLY,  { .u_obj = mp_const_none } },
            { MP_QSTR_cs,                 MP_ARG_OBJ  | MP_ARG_KW_ONLY,  { .u_obj = mp_const_none } },
            { MP_QSTR_freq,               MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = 10000000 } },
            { MP_QSTR_dc_idle_high,       MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false  } },
            { MP_QSTR_dc_cmd_high,        MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false  } },
            { MP_QSTR_dc_dummy_high,      MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false  } },
            { MP_QSTR_dc_data_high,       MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = true   } },
            { MP_QSTR_cs_active_high,     MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false   } },
            { MP_QSTR_reverse_color_bits, MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false   } },
            { MP_QSTR_swap_color_bytes,   MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false   } },
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

        #if !defined(mp_hal_pin_output) && !defined(IDF_VER)
            mp_raise_msg(&mp_type_NotImplementedError, MP_ERROR_TEXT("LCD I80 but is not available for this MCU"));
        #else
            self->panel_io_config.user_ctx = self;
            self->panel_io_config.dc_levels.dc_cmd_level = (unsigned int)args[ARG_dc_cmd_high].u_bool;
            self->panel_io_config.dc_levels.dc_data_level = (unsigned int)args[ARG_dc_data_high].u_bool;
            self->panel_io_config.flags.cs_active_high = (unsigned int)args[ARG_cs_active_high].u_bool;
            self->panel_io_config.flags.reverse_color_bits = (unsigned int)args[ARG_reverse_color_bits].u_bool;
            self->panel_io_config.flags.swap_color_bytes = (unsigned int)args[ARG_swap_color_bytes].u_bool;

            self->bus_config.dc_gpio_num = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_dc].u_obj);
            self->bus_config.wr_gpio_num = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_wr].u_obj);
            mp_hal_pin_output(self->bus_config.dc_gpio_num);
            mp_hal_pin_output(self->bus_config.wr_gpio_num);

            #if defined(MP_HAL_PIN_SPEED_VERY_HIGH) && !defined(MICROPY_INCLUDED_MIMXRT_MPHALPORT_H)
                mp_hal_pin_config_speed(self->bus_config.dc_gpio_num, MP_HAL_PIN_SPEED_VERY_HIGH);
                mp_hal_pin_config_speed(self->bus_config.wr_gpio_num, MP_HAL_PIN_SPEED_VERY_HIGH);
            #endif /* defined(MP_HAL_PIN_SPEED_VERY_HIGH) && !defined(MICROPY_INCLUDED_MIMXRT_MPHALPORT_H) */

            mp_hal_pin_write(self->bus_config.dc_gpio_num, self->panel_io_config.dc_levels.dc_data_level);
            mp_hal_pin_write(self->bus_config.wr_gpio_num, 0);

            self->bus_config.data_gpio_nums[0] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data0].u_obj);
            self->bus_config.data_gpio_nums[1] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data1].u_obj);
            self->bus_config.data_gpio_nums[2] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data2].u_obj);
            self->bus_config.data_gpio_nums[3] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data3].u_obj);
            self->bus_config.data_gpio_nums[4] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data4].u_obj);
            self->bus_config.data_gpio_nums[5] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data5].u_obj);
            self->bus_config.data_gpio_nums[6] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data6].u_obj);
            self->bus_config.data_gpio_nums[7] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data7].u_obj);
            if (args[ARG_data8].u_obj == mp_const_none) {
                self->bus_config.bus_width = 8;
            } else {
                self->bus_config.data_gpio_nums[8] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data8].u_obj);
                self->bus_config.data_gpio_nums[9] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data9].u_obj);
                self->bus_config.data_gpio_nums[10] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data10].u_obj);
                self->bus_config.data_gpio_nums[11] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data11].u_obj);
                self->bus_config.data_gpio_nums[12] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data12].u_obj);
                self->bus_config.data_gpio_nums[13] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data13].u_obj);
                self->bus_config.data_gpio_nums[14] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data14].u_obj);
                self->bus_config.data_gpio_nums[15] = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_data15].u_obj);
                self->bus_config.bus_width = 16;
            }

            mp_hal_pin_obj_t pin;

            for ( uint8_t i = 0; i < self->bus_config.bus_width; i++) {
                pin = self->bus_config.data_gpio_nums[i];
                mp_hal_pin_output(pin);

                #if defined(MP_HAL_PIN_SPEED_VERY_HIGH) && !defined(MICROPY_INCLUDED_MIMXRT_MPHALPORT_H)
                    mp_hal_pin_config_speed(pin, MP_HAL_PIN_SPEED_VERY_HIGH);
                #endif /* defined(MP_HAL_PIN_SPEED_VERY_HIGH) && !defined(MICROPY_INCLUDED_MIMXRT_MPHALPORT_H) */
                mp_hal_pin_write(pin, 0);
            }

            if (args[ARG_cs].u_obj != mp_const_none) {
                self->panel_io_config.cs_gpio_num = (mp_hal_pin_obj_t)mp_hal_get_pin_obj(args[ARG_cs].u_obj);
                mp_hal_pin_output(self->panel_io_config.cs_gpio_num);

                #if defined(MP_HAL_PIN_SPEED_VERY_HIGH) && !defined(MICROPY_INCLUDED_MIMXRT_MPHALPORT_H)
                    mp_hal_pin_config_speed(self->panel_io_config.cs_gpio_num, MP_HAL_PIN_SPEED_VERY_HIGH);
                #endif /* defined(MP_HAL_PIN_SPEED_VERY_HIGH) && !defined(MICROPY_INCLUDED_MIMXRT_MPHALPORT_H) */

                mp_hal_pin_write(self->panel_io_config.cs_gpio_num, !self->panel_io_config.flags.cs_active_high);
            }

            self->panel_io_handle.tx_param = i80_tx_param;
            self->panel_io_handle.tx_color = i80_tx_color;
            self->panel_io_handle.rx_param = i80_rx_param;
            self->panel_io_handle.del = i80_del;
            self->panel_io_handle.init = i80_init;
            self->panel_io_handle.get_lane_count = i80_get_lane_count;
        #endif /* defined(mp_hal_pin_output) || defined(IDF_VER) */

        return MP_OBJ_FROM_PTR(self);
    }


    mp_lcd_err_t i80_tx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        mp_lcd_i80_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

        CS_LOW();
        DC_CMD();

        if (self->bus_config.bus_width == 8) {
            uint8_t *buf = NULL;
            if (self->panel_io_config.lcd_cmd_bits == 8) {
                buf[0] = (uint8_t)lcd_cmd;
                WRITE8();
            } else {
                buf[0] = (uint8_t)((uint16_t)lcd_cmd >> 8);
                WRITE8();
                WR_LOW();
                WR_HIGH();
                buf[0] = (uint8_t)((uint16_t)lcd_cmd & 0xFF);
                WRITE8();
            }
        } else {
            uint16_t *buf = NULL;
            buf[0] = (uint16_t)lcd_cmd;
            WRITE16();
        }

        WR_LOW();
        WR_HIGH();

        DC_DATA();

         if (param != NULL) {
             if (self->bus_config.bus_width == 8) {
                 uint8_t *buf = (uint8_t *)param;
                 uint16_t len = (uint16_t)param_size;
                 while (len--) {
                     WRITE8();
                     WR_LOW();
                     WR_HIGH();
                     buf++;
                 }
             } else {
                 uint16_t *buf = (uint16_t *)param;
                 uint16_t len = (uint16_t)(param_size / 2);
                 while (len--) {
                     WRITE16();
                     WR_LOW();
                     WR_HIGH();
                     buf++;
                 }
             }
        }

        CS_HIGH();
        return LCD_OK;
    }

    mp_lcd_err_t i80_rx_param(mp_obj_t obj, int lcd_cmd, void *param, size_t param_size)
    {
        LCD_UNUSED(obj);
        LCD_UNUSED(lcd_cmd);
        LCD_UNUSED(param);
        LCD_UNUSED(param_size);
        return LCD_OK;
    }


    mp_lcd_err_t i80_tx_color(mp_obj_t obj, int lcd_cmd, void *color, size_t color_size, int x_start, int y_start, int x_end, int y_end, uint8_t rotation, bool last_update)
    {
        LCD_UNUSED(x_start);
        LCD_UNUSED(y_start);
        LCD_UNUSED(x_end);
        LCD_UNUSED(y_end);
        LCD_UNUSED(rotation);
        LCD_UNUSED(last_update);

        mp_lcd_i80_bus_obj_t *self = MP_OBJ_TO_PTR(obj);

        CS_LOW();
        DC_CMD();

        if (self->bus_config.bus_width == 8) {
            uint8_t *buf = NULL;
            if (self->panel_io_config.lcd_cmd_bits == 8) {
                buf[0] = (uint8_t)lcd_cmd;
                WRITE8();
            } else {
                buf[0] = (uint8_t)((uint16_t)lcd_cmd >> 8);
                WRITE8();
                WR_LOW();
                WR_HIGH();
                buf[0] = (uint8_t)((uint16_t)lcd_cmd & 0xFF);
                WRITE8();
            }
        } else {
            uint16_t *buf = NULL;
            buf[0] = (uint16_t)lcd_cmd;
            WRITE16();
        }
        WR_LOW();
        WR_HIGH();
        DC_DATA();

        self->write_color(self, color, color_size);

        if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
            mp_call_function_n_kw(self->callback, 0, 0, NULL);
        }
        self->trans_done = true;

        return LCD_OK;
    }


    mp_lcd_err_t i80_del(mp_obj_t obj)
    {
        return LCD_OK;

    }

    mp_lcd_err_t i80_init(mp_obj_t obj, uint16_t width, uint16_t height, uint8_t bpp, uint32_t buffer_size, bool rgb565_byte_swap, uint8_t cmd_bits, uint8_t param_bits)
    {
        mp_lcd_i80_bus_obj_t *self = MP_OBJ_TO_PTR(obj);
        LCD_UNUSED(rgb565_byte_swap);

        self->buffer_size = buffer_size;
        self->bpp = bpp;
        self->panel_io_config.lcd_cmd_bits = (int)cmd_bits;
        self->panel_io_config.lcd_param_bits = (int)param_bits;

        if (self->rgb565_byte_swap) {
            if (self->bus_config.bus_width == 8) {
                self->write_color = write_rgb565_swap8;
            } else {
                self->write_color = write_rgb565_swap16;
            }
        } else if (self->panel_io_config.flags.swap_color_bytes) {
            if (self->bus_config.bus_width == 8) {
                self->write_color = write_color_swap_bytes8;
            } else {
                self->write_color = write_color_swap_bytes16;
            }
        } else if (self->bus_config.bus_width == 8) {
            self->write_color = write_color8;
        } else {
            self->write_color = write_color16;
        }

        /*
         * This might seem odd, it has a purpose. The byte swapping
         * gets done on a global level, meaning it does it for all busses.
         * because this is a bitbang implimentation and the data needs to be
         * iterated over I am doing the byte byte swap at the same time as the
         * transfer. The code above sets the needed function to handle the byte
         * swapping if it has been set so we do not want to trigger the global
         * byte awapping that is done. So since the correct function is now set we
         * can set this flag to false so the global operation will nt occur.
         */
        self->rgb565_byte_swap = false;

        return LCD_OK;
    }


    mp_lcd_err_t i80_get_lane_count(mp_obj_t obj, uint8_t *lane_count)
    {
        mp_lcd_i80_bus_obj_t *self = MP_OBJ_TO_PTR(obj);
        *lane_count = (uint8_t)self->bus_config.bus_width;
        return LCD_OK;
    }


    /* transfer functions */
    void write_color8(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size)
    {
        uint8_t *buf = (uint8_t *)color;
        uint32_t len = (uint32_t)color_size;
        uint8_t last = 0;

        while (len--) {
            if (buf[0] != last) {
                last = buf[0];
                WRITE8();
            }
            buf++;
            WR_LOW();
            WR_HIGH();
        }
    }


    void write_color16(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size)
    {
        uint16_t *buf = (uint16_t *)color;
        uint16_t len = (uint32_t)(color_size / 2);
        uint16_t last = 0;

        while (len--) {
            if (buf[0] != last) {
                last = buf[0];
                WRITE16();
            }
            buf++;
            WR_LOW();
            WR_HIGH();
        }
    }


    void write_color_swap_bytes8(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size)
    {
        uint8_t last = 0;
        uint8_t *bd = (uint8_t *)color;
        uint32_t len = (uint32_t)color_size;
        uint8_t *buf;

        for (int i = 0; i < len / 2; i++) {
            buf = &bd[i * 2 + 1];
            if (buf[0] != last) {
                last = buf[0];
                WRITE8();
            }
            WR_LOW();
            WR_HIGH();

            buf = &bd[i * 2];

            if (buf[0] != last) {
                last = buf[0];
                WRITE8();
            }
            WR_LOW();
            WR_HIGH();
        }
    }


    void write_color_swap_bytes16(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size)
    {
        uint16_t last = 0;
        uint16_t *bd = (uint16_t *)color;
        uint16_t len = (uint32_t)(color_size / 2);
        uint16_t *buf;

        for (int i = 0; i < len / 2; i++) {
            buf = &bd[i * 2 + 1];
            if (buf[0] != last) {
                last = buf[0];
                WRITE16();
            }
            WR_LOW();
            WR_HIGH();

            buf = &bd[i * 2];

            if (buf[0] != last) {
                last = buf[0];
                WRITE16();
            }
            WR_LOW();
            WR_HIGH();
        }
    }


    void write_rgb565_swap8(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size)
    {
        uint16_t last = 0;
        uint16_t *bd = (uint16_t *)color;
        uint16_t len = (uint16_t)(color_size / 2);
        uint8_t *buf = NULL;

         while (len--) {
            if (bd[0] != last) {
                last = bd[0];
                *buf = bd[0] << 8;
                WRITE8();
                WR_LOW();
                WR_HIGH();
                *buf = bd[0] >> 8;
                WRITE8();
                WR_LOW();
                WR_HIGH();
            }
            WR_LOW();
            WR_HIGH();
            bd++;
        }
    }


    void write_rgb565_swap16(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size)
    {
        uint16_t last = 0;
        uint16_t *buf = (uint16_t *)color;
        uint16_t len = (uint32_t)(color_size / 2);

        while (len--) {
            buf[0] = (buf[0] << 8) | (buf[0] >> 8);
            if (buf[0] != last) {
                last = buf[0];
                WRITE16();
            }
            WR_LOW();
            WR_HIGH();
            buf++;
        }
    }
    /* end transfer functions */
    /* end function definitions */
#endif

/* create micropython class */
MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_i80_bus_type,
    MP_QSTR_I80Bus,
    MP_TYPE_FLAG_NONE,
    make_new, mp_lcd_i80_bus_make_new,
    locals_dict, (mp_obj_dict_t *)&mp_lcd_bus_locals_dict
);
/* end create micropython class */

