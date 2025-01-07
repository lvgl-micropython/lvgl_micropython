// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _I80_BUS_H_
    #define _I80_BUS_H_

    //local_includes
    #include "modlcd_bus.h"
    #include "mphalport.h"

    // micropython includes
    #include "py/obj.h"
    #include "py/objarray.h"
    #include "py/runtime.h"


    typedef struct _mp_lcd_i80_bus_obj_t mp_lcd_i80_bus_obj_t;

    #ifdef MP_PORT_UNIX
        struct _mp_lcd_i80_bus_obj_t {
            mp_obj_base_t base;

            mp_obj_t callback;

            void *buf1;
            void *buf2;
            uint32_t buffer_flags;

            bool trans_done;
            bool rgb565_byte_swap;

            lcd_panel_io_t panel_io_handle;

            void *panel_io_config;
            void *bus_config;
            void *bus_handle;

            uint32_t buffer_size;
            uint8_t bpp;

            void (*write_color)(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size);
        };

    #else
        typedef struct {
            mp_hal_pin_obj_t cs_gpio_num;         /*!< GPIO used for CS line, set to -1 will declaim exclusively use of I80 bus */
            void *user_ctx;    /*!< User private data, passed directly to on_color_trans_done's user_ctx */
            int lcd_cmd_bits;   /*!< Bit-width of LCD command */
            int lcd_param_bits; /*!< Bit-width of LCD parameter */
            struct {
                unsigned int dc_idle_level: 1;  /*!< Level of DC line in IDLE phase */
                unsigned int dc_cmd_level: 1;   /*!< Level of DC line in CMD phase */
                unsigned int dc_dummy_level: 1; /*!< Level of DC line in DUMMY phase */
                unsigned int dc_data_level: 1;  /*!< Level of DC line in DATA phase */
            } dc_levels; /*!< Each i80 device might have its own D/C control logic */
            struct {
                unsigned int cs_active_high: 1;     /*!< If set, a high level of CS line will select the device, otherwise, CS line is low level active */
                unsigned int reverse_color_bits: 1; /*!< Reverse the data bits, D[N:0] -> D[0:N] */
                unsigned int swap_color_bytes: 1;   /*!< Swap adjacent two color bytes */
                unsigned int pclk_active_neg: 1;    /*!< The display will write data lines when there's a falling edge on WR signal (a.k.a the PCLK) */
                unsigned int pclk_idle_low: 1;      /*!< The WR signal (a.k.a the PCLK) stays at low level in IDLE phase */
            } flags;                                /*!< Panel IO config flags */
        } lcd_panel_io_i80_config_t;


        typedef struct _lcd_i80_bus_handle_t{
            mp_hal_pin_obj_t dc_gpio_num; /*!< GPIO used for D/C line */
            mp_hal_pin_obj_t wr_gpio_num; /*!< GPIO used for WR line */
            mp_hal_pin_obj_t data_gpio_nums[16]; /*!< GPIOs used for data lines */
            size_t bus_width;          /*!< Number of data lines, 8 or 16 */
            size_t max_transfer_bytes; /*!< Maximum transfer size, this determines the length of internal DMA link */
            size_t psram_trans_align;  /*!< DMA transfer alignment for data allocated from PSRAM */
            size_t sram_trans_align;   /*!< DMA transfer alignment for data allocated from SRAM */
        } lcd_i80_bus_config_t;

        struct _mp_lcd_i80_bus_obj_t {
            mp_obj_base_t base;

            mp_obj_t callback;

            void *buf1;
            void *buf2;

            bool trans_done;
            bool rgb565_byte_swap;

            lcd_panel_io_t panel_io_handle;

            lcd_panel_io_i80_config_t panel_io_config;
            lcd_i80_bus_config_t bus_config;
            void *bus_handle;

            uint32_t buffer_size;
            uint8_t bpp;

            void (*write_color)(mp_lcd_i80_bus_obj_t *self, void *color, size_t color_size);
        };

    #endif

    extern const mp_obj_type_t mp_lcd_i80_bus_type;
#endif /* _I80_BUS_H_ */