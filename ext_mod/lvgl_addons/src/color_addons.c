// ../../../lib/lvgl/

#include "lvgl.h"
#include <math.h>
#include <stdlib.h>
#include "../include/color_addons.h"


#define PI 3141
#define TWO_PI 6283
#define INV_TWO_PI 159


int64_t floormod(int64_t x, int64_t y) {
    int64_t res = x;
    res %= y;
    if (res < 0) {
        res += y;
    }
    return res;
}

void lv_conical_gradient(uint8_t *buf, uint16_t radius, const lv_grad_dsc_t *grad, uint16_t angle, uint32_t twist)
{
    uint32_t diameter = radius * 2;
    int32_t circumference = (int32_t)(((diameter * 1000) * PI) / 1000);

    lv_grad_dsc_t dsc = {0};
    for (uint8_t i=0; i < grad->stops_count; i++) {
        dsc.stops[i] = grad->stops[i];
    }
    dsc.dir=LV_GRAD_DIR_HOR;
    dsc.stops_count = grad->stops_count;

    lv_grad_t * gradient = lv_gradient_get(&dsc, circumference, 1);
    uint32_t grad_size = (uint32_t)gradient->size;

    int64_t twst = twist * 1000;

    if (twist > 0) {
        twst = ((uint64_t)(diameter * diameter * diameter * diameter)) / (255000 * (twist * 1000));
    } else {
        twst = (uint64_t)twist * 1000
    }

    int32_t rise;
    int32_t run;
    int64_t t;
    uint32_t step;
    uint32_t ang = angle * 1000;

    lv_color_t color;
    lv_opa_t opa;

    for (uint32_t y=0; y < diameter; y++) {
        rise = (radius - y) * 1000;
        run = radius * 1000;

        for (uint32_t x=0; x < diameter; x++) {
            t = lv_atan2(rise, run) + PI - ang;

            if (twist > 0) {
                t += TWO_PI * lv_sqrt((rise * rise + run * run) / twst);
            }

            t = floormod(t, TWO_PI);
            t *= INV_TWO_PI;

            step = (uint32_t)(t * grad_size) / 1000;

            color = gradient->color_map[step];
            opa = gradient->opa_map[step];

            buf[diameter * y + x] = opa;
            buf[diameter * y + x + 1] = color.red;
            buf[diameter * y + x + 2] = color.green;
            buf[diameter * y + x + 3] = color.blue;

            run -= 1000;
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
            dist = (uint32_t)abs((int)lv_sqrt(pow((radius - x), 2) + pow((radius - y), 2)));

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
