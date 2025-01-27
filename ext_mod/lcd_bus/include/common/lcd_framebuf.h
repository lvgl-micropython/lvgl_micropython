// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objarray.h"


#ifndef __LCD_FRAMEBUF_H__
    #define __LCD_FRAMEBUF_H__

    typedef struct _mp_lcd_framebuf_t {
        mp_obj_base_t base;
        size_t typecode : 8;
        size_t free : MP_OBJ_ARRAY_FREE_SIZE_BITS;
        size_t len; // in elements
        void *items;
        uint32_t caps;
    } mp_lcd_framebuf_t;


    extern const mp_obj_type_t mp_lcd_framebuf_type;

#endif /* __LCD_FRAMEBUF_H__ */