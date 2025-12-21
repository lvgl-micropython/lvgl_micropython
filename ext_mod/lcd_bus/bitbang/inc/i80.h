// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "lcd_bus.h"
#include "lcd_types.h"

// port includes
#include "mphalport.h"

// micropython includes
#include "py/obj.h"
#include "py/objarray.h"
#include "py/runtime.h"


#ifndef __I80_H__
    #define __I80_H__

    typedef struct {
        /*
         * GPIO used for CS line, set to -1 will declaim
         * exclusively use of I80 bus
         */
        mp_hal_pin_obj_t cs_gpio_num;

        /*
         * User private data, passed directly to
         * on_color_trans_done's user_ctx
         */
        void *user_ctx;

        /* Bit-width of LCD command */
        int lcd_cmd_bits;

        /* Bit-width of LCD parameter */
        int lcd_param_bits;

        /* Each i80 device might have its own D/C control logic */
        struct {
            /* Level of DC line in IDLE phase */
            unsigned int dc_idle_level: 1;

            /* Level of DC line in CMD phase */
            unsigned int dc_cmd_level: 1;

            /* Level of DC line in DUMMY phase */
            unsigned int dc_dummy_level: 1;

            /* Level of DC line in DATA phase */
            unsigned int dc_data_level: 1;
        } dc_levels;

        /* Panel IO config flags */
        struct {
            /*
             * If set, a high level of CS line will select the device,
             * otherwise, CS line is low level active
             */
            unsigned int cs_active_high: 1;

            /* Reverse the data bits, D[N:0] -> D[0:N] */
            unsigned int reverse_color_bits: 1;

            /* Swap adjacent two color bytes */
            unsigned int swap_color_bytes: 1;

            /*
             * The display will write data lines when there's a falling
             * edge on WR signal (a.k.a the PCLK)
             */
            unsigned int pclk_active_neg: 1;

            /*
             * The WR signal (a.k.a the PCLK)
             * stays at low level in IDLE phase
             */
            unsigned int pclk_idle_low: 1;
        } flags;

    } lcd_panel_io_i80_config_t;


    typedef struct _lcd_i80_bus_handle_t{
        /* GPIO used for D/C line */
        mp_hal_pin_obj_t dc_gpio_num;

        /* GPIO used for WR line */
        mp_hal_pin_obj_t wr_gpio_num;

        /* GPIOs used for data lines */
        mp_hal_pin_obj_t data_gpio_nums[16];

        /* Number of data lines, 8 or 16 */
        size_t bus_width;

        /*
         * Maximum transfer size, this determines
         * the length of internal DMA link
         */
        size_t max_transfer_bytes;

        /* DMA transfer alignment for data allocated from PSRAM */
        size_t psram_trans_align;

        /* DMA transfer alignment for data allocated from SRAM */
        size_t sram_trans_align;
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
