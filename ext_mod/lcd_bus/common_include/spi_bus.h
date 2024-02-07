#ifndef _SPI_BUS_H_
    #define _SPI_BUS_H_

    //local_includes
    #include "modlcd_bus.h"
    #include "mphalport.h"
    // Fix for MicroPython > 1.21 https://github.com/ricksorensen
#if MICROPY_VERSION_MAJOR >= 1 && MICROPY_VERSION_MINOR > 21
    #include "extmod/modmachine.h"
#else
    #include "extmod/machine_spi.h"
#endif
    // micropython includes
    #include "py/objarray.h"
    #include "py/obj.h"

    typedef struct _lcd_panel_io_spi_config_t {
        mp_hal_pin_obj_t cs_gpio_num;
        mp_hal_pin_obj_t dc_gpio_num;
        void (*spi_transfer)(mp_obj_base_t *obj, size_t len, const uint8_t *src, uint8_t *dest);
        int lcd_cmd_bits;
        int lcd_param_bits;
        struct {
            unsigned int dc_low_on_data: 1;
            unsigned int lsb_first: 1;
            unsigned int cs_high_active: 1;
        } flags;
    } lcd_panel_io_spi_config_t;


    typedef struct _mp_lcd_spi_bus_obj_t mp_lcd_spi_bus_obj_t;

    struct _mp_lcd_spi_bus_obj_t {
        mp_obj_base_t base;

        /* callback function that gets called after the buffer
         * has finished being sent. This only gets called after
         * sending the frame buffer
         */
        mp_obj_t callback;

        void *buf1;
        void *buf2;

        bool trans_done;
        bool rgb565_byte_swap;

        /* stores function pointers to carry out work to be done */
        lcd_panel_io_t panel_io_handle;
        /* config settings */
        lcd_panel_io_spi_config_t panel_io_config;

        /* stores the SPI instance that gets created.
         * SPI that is internal to micropython is what is being used
         */
        mp_obj_base_t *bus_handle;

        /* these function pointers get set based on
         * panel_io_config.lcd_cmd_bits and panel_io_config.lcd_param_bits
         */
        void (*send_cmd)(mp_lcd_spi_bus_obj_t *self, int lcd_cmd);
        void (*send_param)(mp_lcd_spi_bus_obj_t *self, void *param, size_t param_size);

    };

    extern const mp_obj_type_t mp_lcd_spi_bus_type;
#endif /* _SPI_BUS_H_ */