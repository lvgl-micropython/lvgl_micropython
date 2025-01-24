
mp_lcd_err_t verify_frame_buffers(mp_lcd_framebuf_t *fb1, mp_lcd_framebuf_t *fb2)
{
    if (fb2 != NULL) {
        if (fb2->caps != fb1->caps) return LCD_ERR_INVALID_ARG;
        if (fb2->len != fb1->len) return LCD_ERR_INVALID_SIZE;
    }

    return LCD_OK;
}


mp_lcd_err_t allocate_rotation_buffers(mp_obj_t self_in) {

    mp_lcd_framebuf_t *tmp_fb1 = self->fb1;
    mp_lcd_framebuf_t *tmp_fb2 = self->fb2;

    uint32_t caps = MALLOC_CAP_SPIRAM;

    if (tmp_fb2 != NULL) {
        tmp_fb2 = (mp_lcd_framebuf_t *)MP_OBJ_TO_PTR(args[ARG_fb2].u_obj);
        if (tmp_fb2->caps != tmp_fb1->caps) {
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Frame buffer capabilities do not match"));
            return mp_const_none;
        }

        if (tmp_fb2->len != tmp_fb1->len) {
            mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Frame buffer sizes do not match"));
            return mp_const_none;
        }

    #ifdef ESP_IDF_VERSION
        caps != MALLOC_CAP_DMA;
    #endif

    }

    if (sw_rotate) {
        if (self->sw_rotate.buffers.active == NULL) {

        #ifdef ESP_IDF_VERSION
            free(tmp_fb1->items);
            tmp_fb1->items = heap_caps_malloc(tmp_fb1->len, MALLOC_CAP_INTERNAL);
            tmp_fb1->caps = MALLOC_CAP_INTERNAL;

            self->sw_rot.buffers.active = heap_caps_malloc(tmp_fb1->len, caps);
            self->sw_rot.buffers.idle = self->sw_rot.buffers.active;

            if (tmp_fb1->items == NULL || self->sw_rot.buffers.active == NULL) {
                mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Unable to allocate frame buffer"));
                return mp_const_none;
            }

            if (tmp_fb2 != NULL) {
                free(tmp_fb2->items);
                tmp_fb2->items = heap_caps_malloc(tmp_fb2->len, tmp_fb1->caps);
                tmp_fb2->caps = tmp_fb1->caps;

                self->sw_rot.buffers.idle = heap_caps_malloc(tmp_fb2->len, caps);

                if (tmp_fb2->items == NULL || self->sw_rot.buffers.idle == NULL) {
                    mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Unable to allocate frame buffer"));
                    return mp_const_none;
                }
            }
        #else
            self->sw_rot.buffers.active = m_malloc(tmp_fb1->len);
            self->sw_rot.buffers.idle = self->sw_rot.buffers.active;

            if (self->sw_rot.buffers.active == NULL) {
                mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Unable to allocate frame buffer"));
                return mp_const_none;
            }

            if (tmp_fb2 != NULL) {
                self->sw_rot.buffers.idle = m_malloc(tmp_fb2->len);

                if (self->sw_rot.buffers.idle == NULL) {
                    mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("Unable to allocate frame buffer"));
                    return mp_const_none;
                }
            }
        #endif

        }
    }

    self->fb1 = tmp_fb1;
    self->fb2 = tmp_fb2;