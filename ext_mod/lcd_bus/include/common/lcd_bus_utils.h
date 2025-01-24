#ifndef __LCD_BUS_UTILS_H__
    #define __LCD_BUS_UTILS_H__

    mp_lcd_err_t mp_lcd_verify_frame_buffers(mp_lcd_framebuf_t *fb1, mp_lcd_framebuf_t *fb2);
    mp_lcd_err_t mp_lcd_allocate_rotation_buffers(mp_lcd_bus_obj_t *self);
    void mp_lcd_free_rotation_buffers(mp_lcd_bus_obj_t self);

#endif /* __LCD_BUS_UTILS_H__ */
