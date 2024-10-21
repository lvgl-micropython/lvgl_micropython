//local_includes
#include "lcd_types.h"

// micropython includes
#include "mphalport.h"
#include "py/obj.h"
#include "py/objarray.h"

// esp-idf includes
#include "soc/soc_caps.h"

#ifndef _ESP32_SPI_3WIRE_H_
    #define _ESP32_SPI_3WIRE_H_

    #if SOC_LCD_I80_SUPPORTED || SOC_LCD_RGB_SUPPORTED
    
        #define LCD_SPI_3WIRE_CMD_BITS_MAX       (sizeof(uint32_t) * 8)  // Maximum number of bytes for LCD command
        #define LCD_SPI_3WIRE_PARAM_BITS_MAX     (sizeof(uint32_t) * 8)  // Maximum number of bytes for LCD parameter
        #define LCD_SPI_3WIRE_CLK_MAX      (500 * 1000UL)
        
        #define LCD_SPI_3WIRE_DATA_DC_BIT_0           (0)     // DC bit = 0
        #define LCD_SPI_3WIRE_DATA_DC_BIT_1           (1)     // DC bit = 1
        #define LCD_SPI_3WIRE_DATA_NO_DC_BIT          (2)     // No DC bit
        #define LCD_SPI_3WIRE_WRITE_ORDER_LSB_MASK    (0x01)  // Bit mask for LSB first write order
        #define LCD_SPI_3WIRE_WRITE_ORDER_MSB_MASK    (0x80)  // Bit mask for MSB first write order
        
        
        typedef struct _mp_spi_3wire_obj_t {
            mp_obj_base_t base;
            int cs;
            int scl;
            int sda;

            uint32_t scl_half_period_us;
            uint32_t cmd_bytes: 3;
            uint32_t cmd_dc_bit: 2;
            uint32_t param_bytes: 3;
            uint32_t param_dc_bit: 2;
            uint32_t write_order_mask: 8;
            uint32_t cs_high_active: 1;
            uint32_t sda_scl_idle_high: 1;
            uint32_t scl_active_rising_edge: 1;
            uint32_t del_keep_cs_inactive: 1;

        } mp_spi_3wire_obj_t;

        extern const mp_obj_type_t mp_spi_3wire_type;

        mp_lcd_err_t mp_spi_3wire_init(mp_spi_3wire_obj_t *self, uint8_t cmd_bits, uint8_t param_bits);
        esp_err_t mp_spi_3wire_tx_param(mp_spi_3wire_obj_t *self, int lcd_cmd, const void *param, size_t param_size);
        void mp_spi_3wire_deinit(mp_spi_3wire_obj_t *self);

    #endif /* SOC_LCD_I80_SUPPORTED */
#endif /* _ESP32_SPI_3WIRE_H_ */
