
#include "../../../lib/lvgl/lvgl.h"
#include <math.h>
#include <stdlib.h>
#include "../include/color_addons.h"

#define PI 3.141592653589793f
#define TWO_PI 6.28318530717958647693f
#define INV_TWO_PI 0.15915494309189533576876437577476f

#ifdef MP_SOFT_ATAN2

// Approximates atan2(y, x) normalized to the [0,4) range
// with a maximum error of 0.1620 degrees
float soft_atan2( float y, float x )
{
    static const uint32_t sign_mask = 0x80000000;
    static const float b = 0.596227f;

    // Extract the sign bits
    uint32_t ux_s  = sign_mask & (uint32_t &)x;
    uint32_t uy_s  = sign_mask & (uint32_t &)y;

    // Determine the quadrant offset
    float q = (float)( ( ~ux_s & uy_s ) >> 29 | ux_s >> 30 );

    // Calculate the arctangent in the first quadrant
    float bxy_a = fabs( b * x * y );
    float num = bxy_a + y * y;
    float atan_1q =  num / ( x * x + bxy_a + num );

    // Translate it to the proper quadrant
    uint32_t uatan_2q = (ux_s ^ uy_s) | (uint32_t &)atan_1q;
    return q + (float &)uatan_2q;
}

#endif /* MP_SOFT_ATAN */


float floormod(float x, float y) {
    int32_t res = (int32_t)x;
    res %= (int32_t)y;
    if (res < 0) {
        res += (int32_t)y;
    }
    return (float)res;
}

void lv_conical_gradient(uint8_t *buf, uint16_t radius, const lv_grad_dsc_t *grad, uint16_t angle, uint32_t twist)
{
    uint32_t diameter = radius * 2;
    int32_t circumference = (int32_t)((float)diameter * PI);

    lv_grad_dsc_t dsc = {0};
    for (uint8_t i=0; i < grad->stops_count; i++) {
        dsc.stops[i] = grad->stops[i];
    }
    dsc.dir=LV_GRAD_DIR_HOR;
    dsc.stops_count = grad->stops_count;

    lv_grad_t * gradient = lv_gradient_get(&dsc, circumference, 1);
    float grad_size = (float)gradient->size;

    if (twist > 0) {
        twist = (uint32_t)((float)(diameter * diameter * diameter * diameter) / (255.0f * (float)twist));
    }

    int32_t rise;
    int32_t run;
    float t;
    uint32_t step;
    lv_color_t color;
    lv_opa_t opa;

    for (uint32_t y=0; y < diameter; y++) {
        rise = radius - y;
        run = radius;

        for (uint32_t x=0; x < diameter; x++) {
            #ifdef MP_SOFT_ATAN2
                t = (float)soft_atan2((float)rise, (float)run) + (float)PI - (float)angle;
            #else
                t = (float)atan2((float)rise, (float)run) + (float)PI - (float)angle;
            #endif

            if (twist > 0) {
                t += TWO_PI * sqrtf((float)(rise * rise + run * run) / (float)twist);
            }

            t = floormod(t, TWO_PI);
            t *= INV_TWO_PI;

            step = (uint32_t)(t * grad_size);

            color = gradient->color_map[step];
            opa = gradient->opa_map[step];

            buf[diameter * y + x] = opa;
            buf[diameter * y + x + 1] = color.red;
            buf[diameter * y + x + 2] = color.green;
            buf[diameter * y + x + 3] = color.blue;

            run -= 1;
        }
    }
}


void lv_radial_gradient(uint8_t *buf, uint16_t radius, const lv_grad_dsc_t *grad)
{
    lv_grad_dsc_t dsc = {0};
    for (uint8_t i=0; i < grad->stops_count; i++) {
        dsc.stops[i] = grad->stops[i];
    }
    dsc.dir=LV_GRAD_DIR_HOR;
    dsc.stops_count = grad->stops_count;

    lv_grad_t * gradient = lv_gradient_get(&dsc, (int32_t)radius, 1);

    uint32_t diameter = radius * 2;
    uint32_t dist;
    lv_color_t color;
    lv_opa_t opa;

    for (uint32_t y=0; y < diameter; y++) {
        for (uint32_t x=0; x < diameter; x++) {
            dist = (uint32_t)abs((int)sqrtf(powf((float)(radius - x), 2.0f) + powf((float)(radius - y), 2.0f)));
            if (dist >= gradient->size){
                continue;
            }

            color = gradient->color_map[dist];
            opa = gradient->opa_map[dist];

            buf[diameter * y + x] = opa;
            buf[diameter * y + x + 1] = color.red;
            buf[diameter * y + x + 2] = color.green;
            buf[diameter * y + x + 3] = color.blue;
        }
    }
}


uint8_t RED_THRESH[] = {
  1, 7, 3, 5, 0, 8, 2, 6,
  7, 1, 5, 3, 8, 0, 6, 2,
  3, 5, 0, 8, 2, 6, 1, 7,
  5, 3, 8, 0, 6, 2, 7, 1,
  0, 8, 2, 6, 1, 7, 3, 5,
  8, 0, 6, 2, 7, 1, 5, 3,
  2, 6, 1, 7, 3, 5, 0, 8,
  6, 2, 7, 1, 5, 3, 8, 0
};

uint8_t GREEN_THRESH[] = {
  1, 3, 2, 2, 3, 1, 2, 2,
  2, 2, 0, 4, 2, 2, 4, 0,
  3, 1, 2, 2, 1, 3, 2, 2,
  2, 2, 4, 0, 2, 2, 0, 4,
  1, 3, 2, 2, 3, 1, 2, 2,
  2, 2, 0, 4, 2, 2, 4, 0,
  3, 1, 2, 2, 1, 3, 2, 2,
  2, 2, 4, 0, 2, 2, 0, 4
};

uint8_t BLUE_THRESH[] = {
  5, 3, 8, 0, 6, 2, 7, 1,
  3, 5, 0, 8, 2, 6, 1, 7,
  8, 0, 6, 2, 7, 1, 5, 3,
  0, 8, 2, 6, 1, 7, 3, 5,
  6, 2, 7, 1, 5, 3, 8, 0,
  2, 6, 1, 7, 3, 5, 0, 8,
  7, 1, 5, 3, 8, 0, 6, 2,
  1, 7, 3, 5, 0, 8, 2, 6
};


uint8_t closest_rb(uint8_t c)
{
    return c >> 3 << 3;
}


uint8_t closest_g(uint8_t c)
{
    return c >> 2 << 2;
}


void rgb565_dither_pixel(uint16_t x, uint16_t y, lv_color_t *col)
{
    uint8_t threshold_id = (uint8_t)(((y & 7) << 3) + (x & 7));
    col->red = closest_rb(((col->red & 0xF8) + RED_THRESH[threshold_id]) & 0xFF) & 0xF8;
    col->green = closest_g(((col->green & 0xFC) + GREEN_THRESH[threshold_id]) & 0xFF) & 0xFC;
    col->blue = closest_rb(((col->blue & 0xF8) + BLUE_THRESH[threshold_id]) & 0xFF) & 0xF8;
}


void lv_rgb565_dither(uint8_t *buf, uint16_t width, uint16_t height, lv_color_format_t format) {
    lv_color_t *color;
    uint8_t *p;
    for (uint16_t y=0; y < height; y++) {
        for (uint16_t x=0; x < width; x++) {
            if (format == LV_COLOR_FORMAT_RGB888) {
                p = &buf[height * y + x];
                color = (lv_color_t *)p;
            } else if ((format == LV_COLOR_FORMAT_ARGB8888) || (format == LV_COLOR_FORMAT_XRGB8888)) {
                p = &buf[height * y + x + 1];
                color = (lv_color_t *)p;
            } else {
                continue;
            }
            rgb565_dither_pixel(x, y, color);
        }
    }
}
