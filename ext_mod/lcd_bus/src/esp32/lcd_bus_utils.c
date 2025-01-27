#include "common/lcd_bus_utils.h"
#include "common/lcd_framebuf.h"
#include "common/lcd_common_types.h"
#include "lcd_types.h"


mp_lcd_err_t mp_lcd_verify_frame_buffers(mp_lcd_framebuf_t *fb1, mp_lcd_framebuf_t *fb2)
{
    if (fb2 != NULL) {
        if (fb2->caps != fb1->caps) return LCD_ERR_INVALID_ARG;
        if (fb2->len != fb1->len) return LCD_ERR_INVALID_SIZE;
    }

    return LCD_OK;
}


mp_lcd_err_t mp_lcd_allocate_rotation_buffers(mp_lcd_bus_obj_t *self)
{
    mp_lcd_framebuf_t *fb1 = self->fb1;
    mp_lcd_framebuf_t *fb2 = self->fb2;
    
    mp_lcd_sw_rotation_buffers_t *buffers = &self->sw_rot.buffers;

    uint32_t caps = MALLOC_CAP_SPIRAM;

    if (fb2 != NULL) caps |= MALLOC_CAP_DMA;

    if (!self->sw_rotate && !self->sw_rot.data.rgb565_swap && fb2 != NULL) {
        if (!(fb1->caps & MALLOC_CAP_DMA)) {
            free(fb1->items);

            if (!(fb1->caps & MALLOC_CAP_INTERNAL) && !(fb1->caps & MALLOC_CAP_SPIRAM)) {
                fb1->items = heap_caps_malloc(fb1->len, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
                fb1->caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA;

                if (fb1->items == NULL) {
                    fb1->items = heap_caps_malloc(fb1->len, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
                    fb1->caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA;
                }
            } else {
                fb1->items = heap_caps_malloc(fb1->len, fb1->caps | MALLOC_CAP_DMA);
                fb1->caps = fb1->caps | MALLOC_CAP_DMA;
            }
        }
    } else {
        if (fb1->caps & MALLOC_CAP_INTERNAL || fb1->caps & MALLOC_CAP_SPIRAM) {
            free(fb1->items);
            fb1->items = heap_caps_malloc(fb1->len, MALLOC_CAP_INTERNAL);
            fb1->caps = MALLOC_CAP_INTERNAL;

            if (fb1->items == NULL) {
                fb1->items = heap_caps_malloc(fb1->len, MALLOC_CAP_SPIRAM);
                fb1->caps = MALLOC_CAP_SPIRAM;
            }
        }
    }

    buffers->active = (uint8_t *)heap_caps_malloc(fb1->len, caps);
    buffers->idle = buffers->active;

    if (fb1->items == NULL || buffers->active == NULL) {
        return LCD_ERR_NO_MEM;
    }

    if (fb2 != NULL) {
        if (fb2->caps != fb1->caps) {
            free(fb2->items);
            fb2->items = heap_caps_malloc(fb2->len, fb1->caps);
            fb2->caps = fb1->caps;
        }

        buffers->idle = heap_caps_malloc(fb2->len, caps);

        if (fb2->items == NULL || buffers->idle == NULL) {
            return LCD_ERR_NO_MEM;
        }
    }

    return LCD_OK;
}

void mp_lcd_free_rotation_buffers(mp_lcd_bus_obj_t *self)
{
    mp_lcd_sw_rotation_buffers_t *buffers = &self->sw_rot.buffers;

    if (buffers->idle != buffers->active) {
        free(buffers->idle);
    }

    free(buffers->active);
    buffers->active = NULL;
    buffers->idle = NULL;
}
