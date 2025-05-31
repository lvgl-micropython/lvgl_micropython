// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef __I80_BUS_H__
    #define __I80_BUS_H__

    //local_includes
    #include "modlcd_bus.h"
    #include "mphalport.h"
    #include "lcd_types.h"

    // micropython includes
    #include "py/obj.h"
    #include "py/objarray.h"
    #include "py/runtime.h"



    //#ifdef MP_PORT_UNIX
    typedef struct _mp_lcd_i80_bus_obj_t {
        mp_obj_base_t base;

        mp_obj_t callback;

        mp_obj_array_t *view1;
        mp_obj_array_t *view2;

        uint32_t buffer_flags;

        uint8_t trans_done: 1;
        uint8_t num_lanes: 5;

        lcd_task_t task;
        lcd_init_t init;
        lcd_bufs_t bufs;

        lcd_tx_data_t tx_data;
        lcd_tx_cmds_t tx_cmds;

        rotation_data_t r_data;

        internal_cb_funcs_t internal_cb_funcs;
    } mp_lcd_i80_bus_obj_t;
    /*
    #else
        typedef struct {
            mp_hal_pin_obj_t cs_gpio_num;
            void *user_ctx;
            int lcd_cmd_bits;
            int lcd_param_bits;
            struct {
                unsigned int dc_idle_level: 1;
                unsigned int dc_cmd_level: 1;
                unsigned int dc_dummy_level: 1;
                unsigned int dc_data_level: 1;
            } dc_levels;
            struct {
                unsigned int cs_active_high: 1;
                unsigned int reverse_color_bits: 1;
                unsigned int swap_color_bytes: 1;
                unsigned int pclk_active_neg: 1;
                unsigned int pclk_idle_low: 1;
            } flags;
        } lcd_panel_io_i80_config_t;


        typedef struct _lcd_i80_bus_handle_t{
            mp_hal_pin_obj_t dc_gpio_num; //!< GPIO used for D/C line
            mp_hal_pin_obj_t wr_gpio_num; //!< GPIO used for WR line
            mp_hal_pin_obj_t data_gpio_nums[16]; //!< GPIOs used for data lines
            size_t bus_width;          //!< Number of data lines, 8 or 16
            size_t max_transfer_bytes; //!< Maximum transfer size, this determines the length of internal DMA link
            size_t psram_trans_align;  //!< DMA transfer alignment for data allocated from PSRAM
            size_t sram_trans_align;   //!< DMA transfer alignment for data allocated from SRAM
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
    */

    extern const mp_obj_type_t mp_lcd_i80_bus_type;
#endif /* _I80_BUS_H_ */