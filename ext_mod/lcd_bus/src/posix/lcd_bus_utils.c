#include "common/lcd_bus_utils.h"
#include "common/lcd_framebuf.h"
#include "common/lcd_common_types.h"
#include "lcd_types.h"


mp_lcd_err_t mp_lcd_verify_frame_buffers(mp_lcd_framebuf_t *fb1, mp_lcd_framebuf_t *fb2)
{
    if (fb2 != NULL) {
        if (fb2->len != fb1->len) return LCD_ERR_INVALID_SIZE;
    }

    return LCD_OK;
}


mp_lcd_err_t mp_lcd_allocate_rotation_buffers(mp_lcd_bus_obj_t *self)
{
    return LCD_OK;
}

void mp_lcd_free_rotation_buffers(mp_lcd_bus_obj_t *self)
{
}
