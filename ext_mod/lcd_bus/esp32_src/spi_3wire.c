#include "spi_3wire.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/runtime.h"

// stdlib includes
#include <string.h>

// esp-idf includes
#include "soc/soc_caps.h"

#if SOC_LCD_I80_SUPPORTED || SOC_LCD_RGB_SUPPORTED
    #include "driver/gpio.h"
    #include "driver/spi_master.h"
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "esp_check.h"
    
    static void _reset_gpios(int64_t gpio_mask) 
    {
        for (int i = 0; i < 64; i++) {
            if (gpio_mask & BIT64(i)) {
                gpio_reset_pin(i);
            }
        }
    }
    
    static void _delay_us(uint32_t delay_us)
    {
        if (delay_us >= 1000) {
            vTaskDelay(pdMS_TO_TICKS(delay_us / 1000));
        } else {
            esp_rom_delay_us(delay_us);
        }
    }
    
    static esp_err_t spi_3wire_write_byte(mp_spi_3wire_obj_t *self, int dc_bit, uint8_t data);
    static esp_err_t spi_3wire_write_package(mp_spi_3wire_obj_t *self, bool is_cmd, uint32_t data);
    
    static mp_obj_t mp_spi_3wire_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
    {
        enum {
            ARG_scl,
            ARG_sda,
            ARG_cs,
            ARG_freq,
            ARG_polarity,
            ARG_phase,
            ARG_use_dc_bit,
            ARG_dc_data_high,
            ARG_lsb_first,
            ARG_cs_active_high,
            ARG_del_keep_cs_active,
        };

        const mp_arg_t make_new_args[] = {
            { MP_QSTR_scl,                MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_sda,                MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_cs,                 MP_ARG_INT  | MP_ARG_KW_ONLY | MP_ARG_REQUIRED       },
            { MP_QSTR_freq,               MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = SPI_3WIRE_CLK_MAX } },
            { MP_QSTR_polarity,           MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = 0       } },
            { MP_QSTR_phase,              MP_ARG_INT  | MP_ARG_KW_ONLY,  { .u_int = 0       } },
            { MP_QSTR_use_dc_bit,         MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false  } },
            { MP_QSTR_dc_data_high,       MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = true   } },
            { MP_QSTR_lsb_first,          MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false  } },
            { MP_QSTR_cs_active_high,     MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false  } },
            { MP_QSTR_del_keep_cs_active, MP_ARG_BOOL | MP_ARG_KW_ONLY,  { .u_bool = false  } },
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
        mp_spi_3wire_obj_t *self = m_new_obj(mp_spi_3wire_obj_t);
        self->base.type = &mp_spi_3wire_type;

        self->scl_half_period_us = 1000000 / ((uint32_t)args[ARG_freq].u_int * 2);

        if ((bool)args[ARG_use_dc_bit].u_bool) {
            self->param_dc_bit = (bool)args[ARG_dc_data_high].u_bool ? SPI_3WIRE_DATA_DC_BIT_1 : SPI_3WIRE_DATA_DC_BIT_0;
            self->cmd_dc_bit = (bool)args[ARG_dc_data_high].u_bool ? SPI_3WIRE_DATA_DC_BIT_0 : SPI_3WIRE_DATA_DC_BIT_1;
        } else {
            self->param_dc_bit = SPI_3WIRE_DATA_NO_DC_BIT;
            self->cmd_dc_bit = SPI_3WIRE_DATA_NO_DC_BIT;
        }
        
        self->write_order_mask = (bool)args[ARG_lsb_first].u_bool ? SPI_3WIRE_WRITE_ORDER_LSB_MASK : SPI_3WIRE_WRITE_ORDER_MSB_MASK;
        self->cs_high_active = (int)args[ARG_cs_active_high].u_bool;
        self->del_keep_cs_inactive = (bool)args[ARG_del_keep_cs_active].u_bool ? 0 : 1;
        
        uint32_t spi_mode = (uint32_t)args[ARG_phase].u_int | ((uint32_t)args[ARG_polarity].u_int << 1);
        self->sda_scl_idle_high = spi_mode & 0x1;
        
        if (self->sda_scl_idle_high) {
            self->scl_active_rising_edge = (spi_mode & 0x2) ? 1 : 0;
        } else {
            self->scl_active_rising_edge = (spi_mode & 0x2) ? 0 : 1;
        }
        
        self->cs = (int)args[ARG_cs].u_int;
        self->scl = (int)args[ARG_scl].u_int;
        self->sda = (int)args[ARG_sda].u_int;

    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("cs=%d\n", self->cs);
        printf("scl=%d\n", self->scl);
        printf("sda=%d\n", self->sda);
        printf("scl_active_rising_edge=%i\n", self->scl_active_rising_edge);
        printf("sda_scl_idle_high=%i\n", self->sda_scl_idle_high);
        printf("spi_mode=%i\n", spi_mode);
        printf("del_keep_cs_inactive=%i\n", self->del_keep_cs_inactive);
        printf("cs_high_active=%i\n", self->cs_high_active);
        printf("write_order_mask=%i\n", self->write_order_mask);
        printf("cmd_dc_bit=%i\n", self->cmd_dc_bit);
        printf("param_dc_bit=%i\n", self->param_dc_bit);
        printf("scl_half_period_us=%i\n", self->scl_half_period_us);
    #endif
        return MP_OBJ_FROM_PTR(self);
    }

    void mp_spi_3wire_init(mp_spi_3wire_obj_t *self, uint8_t cmd_bits, uint8_t param_bits)
    {
    
    #if CONFIG_LCD_ENABLE_DEBUG_LOG
        printf("mp_spi_3wire_init(self, cmd_bits=%i, param_bits=%i)\n", cmd_bits, param_bits);
    #endif
    
        self->cmd_bytes = (uint32_t)cmd_bits / 8;
        self->param_bytes = (uint32_t)param_bits / 8;
        
        int64_t gpio_mask = 0;
        gpio_mask |= BIT64(self->cs);
        gpio_mask |= BIT64(self->scl);
        gpio_mask |= BIT64(self->sda);
    
        // Configure GPIOs
        esp_err_t ret = gpio_config(&((gpio_config_t) {
            .pin_bit_mask = gpio_mask,
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        }));
        
        if (ret != 0) {
            _reset_gpios(gpio_mask);
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(gpio_config)"), ret);
            return;
        } 

        // Set CS, SCL and SDA to idle level
        uint32_t cs_idle_level = self->cs_high_active ? 0 : 1;
        uint32_t sda_scl_idle_level = self->sda_scl_idle_high ? 1 : 0;
        
        ret = gpio_set_level(self->cs, cs_idle_level);
        if (ret != 0) {
            _reset_gpios(gpio_mask);
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(gpio_set_level CS)"), ret);
            return;
        }
        
        ret = gpio_set_level(self->scl, sda_scl_idle_level);
        if (ret != 0) {
            _reset_gpios(gpio_mask);
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(gpio_set_level SCL)"), ret);
            return;
        }

        ret = gpio_set_level(self->sda, sda_scl_idle_level);
        if (ret != 0) {
            _reset_gpios(gpio_mask);
            mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("%d(gpio_set_level SDA)"), ret);
            return;
        }
    }
    
    
    static mp_obj_t spi_3wire_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_cmd_bits, ARG_param_bits };
        const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,             MP_ARG_OBJ  | MP_ARG_REQUIRED },
            { MP_QSTR_cmd_bits,         MP_ARG_INT  | MP_ARG_REQUIRED },
            { MP_QSTR_param_bits,       MP_ARG_INT  | MP_ARG_REQUIRED },
        };
        
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
        
        mp_spi_3wire_obj_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);        
        mp_spi_3wire_init(
            self, 
            (uint8_t)args[ARG_cmd_bits].u_int,
            (uint8_t)args[ARG_param_bits].u_int
        );
        return mp_const_none;
    }
    
    static MP_DEFINE_CONST_FUN_OBJ_KW(spi_3wire_init_obj, 3, spi_3wire_init);
    
    
    void mp_spi_3wire_tx_param(mp_spi_3wire_obj_t *self, int lcd_cmd, const void *param, size_t param_size)
    {    
        // Send command
        if (lcd_cmd >= 0) {
            spi_3wire_write_package(self, true, lcd_cmd);
        }
    
        // Send parameter
        if (param != NULL && param_size > 0) {
            uint32_t param_data = 0;
            uint32_t param_bytes = self->param_bytes;
            size_t param_count = param_size / param_bytes;
    
            // Iteratively get parameter packages and send them one by one
            for (int i = 0; i < param_count; i++) {
                param_data = 0;
                for (int j = 0; j < param_bytes; j++) {
                    param_data |= ((uint8_t *)param)[i * param_bytes + j] << (j * 8);
                }
                spi_3wire_write_package(self, false, param_data);
            }
        }
    }
    
    
    static mp_obj_t spi_3wire_tx_param(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
    {
        enum { ARG_self, ARG_cmd, ARG_params };
        static const mp_arg_t allowed_args[] = {
            { MP_QSTR_self,    MP_ARG_OBJ | MP_ARG_REQUIRED, { .u_obj = mp_const_none } },
            { MP_QSTR_cmd,     MP_ARG_INT | MP_ARG_REQUIRED, { .u_int = -1            } },
            { MP_QSTR_params,  MP_ARG_OBJ, {.u_obj = mp_const_none} },
        };
        mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
        mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
        
        mp_spi_3wire_obj_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);
        
        if (args[ARG_params].u_obj != mp_const_none) {
            mp_buffer_info_t bufinfo;
            mp_get_buffer_raise(args[ARG_params].u_obj, &bufinfo, MP_BUFFER_READ);
            
            mp_spi_3wire_tx_param(self, (int)args[ARG_cmd].u_int, bufinfo.buf, (size_t)bufinfo.len);
        } else {
            mp_spi_3wire_tx_param(self, (int)args[ARG_cmd].u_int, NULL, 0);
        }
        
        return mp_const_none;
    }
    
    static MP_DEFINE_CONST_FUN_OBJ_KW(spi_3wire_tx_param_obj, 2, spi_3wire_tx_param);
       
    
    void mp_spi_3wire_deinit(mp_spi_3wire_obj_t *self)
    { 
        if (!self->del_keep_cs_inactive) {
            gpio_reset_pin(self->cs);
        }
        
        gpio_reset_pin(self->scl);
        gpio_reset_pin(self->sda);
    }
    
    
    static mp_obj_t spi_3wire_deinit(mp_obj_t self_in)
    {
        mp_spi_3wire_obj_t *self = MP_OBJ_TO_PTR(self_in);
        
        mp_spi_3wire_deinit(self);
        
        return mp_const_none;
    }
    
    static MP_DEFINE_CONST_FUN_OBJ_1(spi_3wire_deinit_obj, spi_3wire_deinit);
    

    static esp_err_t spi_3wire_write_byte(mp_spi_3wire_obj_t *self, int dc_bit, uint8_t data)
    {
        uint16_t data_temp = data;
        uint8_t data_bits = (dc_bit != SPI_3WIRE_DATA_NO_DC_BIT) ? 9 : 8;
        uint16_t write_order_mask = self->write_order_mask;
        uint32_t scl_active_before_level = self->scl_active_rising_edge ? 0 : 1;
        uint32_t scl_active_after_level = !scl_active_before_level;
        uint32_t scl_half_period_us = self->scl_half_period_us;
    
        for (uint8_t i = 0; i < data_bits; i++) {
            // Send DC bit first
            if (data_bits == 9 && i == 0) {
                gpio_set_level(self->sda, dc_bit);
            } else { // Then send data bit
                // SDA set to data bit
                gpio_set_level(self->sda, data_temp & write_order_mask);
                // Get next bit
                data_temp = (write_order_mask == SPI_3WIRE_WRITE_ORDER_LSB_MASK) ? data_temp >> 1 : data_temp << 1;
            }
            // Generate SCL active edge
            gpio_set_level(self->scl, scl_active_before_level);
            _delay_us(scl_half_period_us);
            gpio_set_level(self->scl, scl_active_after_level);
            _delay_us(scl_half_period_us);
        }
    
        return ESP_OK;
    }
    
    
    static esp_err_t spi_3wire_write_package(mp_spi_3wire_obj_t *self, bool is_cmd, uint32_t data)
    {
        uint32_t data_bytes = is_cmd ? self->cmd_bytes : self->param_bytes;
        uint32_t cs_idle_level = self->cs_high_active ? 0 : 1;
        uint32_t sda_scl_idle_level = self->sda_scl_idle_high ? 1 : 0;
        uint32_t scl_active_before_level = self->scl_active_rising_edge ? 0 : 1;
        uint32_t time_us = self->scl_half_period_us;
        // Swap command bytes order due to different endianness
        uint32_t swap_data = SPI_SWAP_DATA_TX(data, data_bytes * 8);
        int data_dc_bit = is_cmd ? self->cmd_dc_bit : self->param_dc_bit;
    
        // CS active
        gpio_set_level(self->cs, !cs_idle_level);
               
        _delay_us(time_us);
        
        gpio_set_level(self->scl, scl_active_before_level);
        // Send data byte by byte
        for (int i = 0; i < data_bytes; i++) {
            // Only set DC bit for the first byte
            if (i == 0) {
                spi_3wire_write_byte(self, data_dc_bit, swap_data & 0xff);
            } else {
                spi_3wire_write_byte(self, SPI_3WIRE_DATA_NO_DC_BIT, swap_data & 0xff);
            }
            swap_data >>= 8;
        }
        gpio_set_level(self->scl, sda_scl_idle_level);
        gpio_set_level(self->sda, sda_scl_idle_level);
        _delay_us(time_us);
        // CS inactive
        gpio_set_level(self->cs, cs_idle_level);
        _delay_us(time_us);
    
        return ESP_OK;
    }


    static const mp_rom_map_elem_t mp_spi_3wire_locals_dict_table[] = {
        { MP_ROM_QSTR(MP_QSTR_tx_param),             MP_ROM_PTR(&spi_3wire_tx_param_obj)             },
        { MP_ROM_QSTR(MP_QSTR_init),                 MP_ROM_PTR(&spi_3wire_init_obj)                 },
        { MP_ROM_QSTR(MP_QSTR_deinit),               MP_ROM_PTR(&spi_3wire_deinit_obj)               },
        { MP_ROM_QSTR(MP_QSTR___del__),              MP_ROM_PTR(&spi_3wire_deinit_obj)               }
    };

    static MP_DEFINE_CONST_DICT(mp_spi_3wire_locals_dict, mp_spi_3wire_locals_dict_table);


    MP_DEFINE_CONST_OBJ_TYPE(
        mp_spi_3wire_type,
        MP_QSTR_SPI3Wire,
        MP_TYPE_FLAG_NONE,
        make_new, mp_spi_3wire_make_new,
        locals_dict, (mp_obj_dict_t *)&mp_spi_3wire_locals_dict
    );

#endif
    
    
    
