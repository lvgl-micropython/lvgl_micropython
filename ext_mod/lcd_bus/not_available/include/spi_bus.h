// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef _SPI_BUS_H_
    #define _SPI_BUS_H_

    //local_includes
    #include "modlcd_bus.h"
    #include "mphalport.h"

    // micropython includes
    #include "py/objarray.h"
    #include "py/obj.h"

    typedef struct _mp_lcd_spi_bus_obj_t {
        mp_obj_base_t base;
    } mp_lcd_spi_bus_obj_t;


    extern const mp_obj_type_t mp_lcd_spi_bus_type;
#endif /* _SPI_BUS_H_ */