#include "../../../lib/lvgl/lvgl.h"

#ifndef _COLOR_ADDONS_H_
    #define _COLOR_ADDONS_H_

    void lv_conical_gradient(uint8_t *buf, uint16_t radius, const lv_grad_dsc_t *grad, uint16_t angle, uint32_t twist);
    void lv_radial_gradient(uint8_t *buf, uint16_t radius, const lv_grad_dsc_t *grad);
    void lv_rgb565_dither(uint8_t *buf, uint16_t width, uint16_t height, lv_color_format_t format);

#endif