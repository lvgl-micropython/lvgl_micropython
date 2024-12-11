/**
 * @file lv_obj_draw.h
 *
 */

#ifndef LV_OBJ_DRAW_H
#define LV_OBJ_DRAW_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../misc/lv_types.h"
#include "../draw/lv_draw.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    LV_LAYER_TYPE_NONE,
    LV_LAYER_TYPE_SIMPLE,
    LV_LAYER_TYPE_TRANSFORM,
} lv_layer_type_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize a rectangle draw descriptor from an object's styles in its current state
 * @param obj       pointer to an object
 * @param part      part of the object, e.g. `LV_PART_MAIN`, `LV_PART_SCROLLBAR`, `LV_PART_KNOB`, etc
 * @param draw_dsc  the descriptor to initialize.
 *                  If an `..._opa` field is set to `LV_OPA_TRANSP` the related properties won't be initialized.
 *                  Should be initialized with `lv_draw_rect_dsc_init(draw_dsc)`.
 * @note Only the relevant fields will be set.
 *       E.g. if `border width == 0` the other border properties won't be evaluated.
 */
void lv_obj_init_draw_rect_dsc(lv_obj_t * obj, uint32_t part, lv_draw_rect_dsc_t * draw_dsc);
mp_obj_t mp_lv_obj_init_draw_rect_dsc(mp_obj_t obj_in, mp_obj_t part_in, mp_obj_t draw_dsc_in)
{
    lv_obj_t * obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    uint32_t part = (uint32_t)mp_obj_get_int_truncated(part_in);
    lv_draw_rect_dsc_t * draw_dsc = (lv_draw_rect_dsc_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(draw_dsc_in))->obj);

    lv_obj_init_draw_rect_dsc(obj, part, draw_dsc);
    return mp_const_none;
}

/**
 * Initialize a label draw descriptor from an object's styles in its current state
 * @param obj       pointer to an object
 * @param part      part of the object, e.g. `LV_PART_MAIN`, `LV_PART_SCROLLBAR`, `LV_PART_KNOB`, etc
 * @param draw_dsc  the descriptor to initialize.
 *                  If the `opa` field is set to or the property is equal to `LV_OPA_TRANSP` the rest won't be initialized.
 *                  Should be initialized with `lv_draw_label_dsc_init(draw_dsc)`.
 */
// void lv_obj_init_draw_label_dsc(lv_obj_t * obj, uint32_t part, lv_draw_label_dsc_t * draw_dsc);
mp_obj_t mp_lv_obj_init_draw_label_dsc(mp_obj_t obj_in, mp_obj_t part_in, mp_obj_t draw_dsc_in)
{
    lv_obj_t * obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    uint32_t part = (uint32_t)mp_obj_get_int_truncated(part_in);
    lv_draw_label_dsc_t * draw_dsc = (lv_draw_label_dsc_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(draw_dsc_in))->obj);

    lv_obj_init_draw_label_dsc(obj, part, draw_dsc);
    return mp_const_none;
}

/**
 * Initialize an image draw descriptor from an object's styles in its current state
 * @param obj       pointer to an object
 * @param part      part of the object, e.g. `LV_PART_MAIN`, `LV_PART_SCROLLBAR`, `LV_PART_KNOB`, etc
 * @param draw_dsc  the descriptor to initialize.
 *                  Should be initialized with `lv_draw_image_dsc_init(draw_dsc)`.
 */
// void lv_obj_init_draw_image_dsc(lv_obj_t * obj, uint32_t part, lv_draw_image_dsc_t * draw_dsc);
mp_obj_t mp_lv_obj_init_draw_image_dsc(mp_obj_t obj_in, mp_obj_t part_in, mp_obj_t draw_dsc_in)
{
    lv_obj_t * obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    uint32_t part = (uint32_t)mp_obj_get_int_truncated(part_in);
    lv_draw_image_dsc_t * draw_dsc = (lv_draw_image_dsc_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(draw_dsc_in))->obj);

    lv_obj_init_draw_image_dsc(obj, part, draw_dsc);
    return mp_const_none;
}

/**
 * Initialize a line draw descriptor from an object's styles in its current state
 * @param obj pointer to an object
 * @param part      part of the object, e.g. `LV_PART_MAIN`, `LV_PART_SCROLLBAR`, `LV_PART_KNOB`, etc
 * @param draw_dsc  the descriptor to initialize.
 *                  Should be initialized with `lv_draw_line_dsc_init(draw_dsc)`.
 */
// void lv_obj_init_draw_line_dsc(lv_obj_t * obj, uint32_t part, lv_draw_line_dsc_t * draw_dsc);
mp_obj_t mp_lv_obj_init_draw_line_dsc(mp_obj_t obj_in, mp_obj_t part_in, mp_obj_t draw_dsc_in)
{
    lv_obj_t * obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    uint32_t part = (uint32_t)mp_obj_get_int_truncated(part_in);
    lv_draw_line_dsc_t * draw_dsc = (lv_draw_line_dsc_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(draw_dsc_in))->obj);

    lv_obj_init_draw_line_dsc(obj, part, draw_dsc);
    return mp_const_none;
}

/**
 * Initialize an arc draw descriptor from an object's styles in its current state
 * @param obj       pointer to an object
 * @param part      part of the object, e.g. `LV_PART_MAIN`, `LV_PART_SCROLLBAR`, `LV_PART_KNOB`, etc
 * @param draw_dsc  the descriptor to initialize.
 *                  Should be initialized with `lv_draw_arc_dsc_init(draw_dsc)`.
 */
// void lv_obj_init_draw_arc_dsc(lv_obj_t * obj, uint32_t part, lv_draw_arc_dsc_t * draw_dsc);
mp_obj_t mp_lv_obj_init_draw_arc_dsc(mp_obj_t obj_in, mp_obj_t part_in, mp_obj_t draw_dsc_in)
{
    lv_obj_t * obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    uint32_t part = (uint32_t)mp_obj_get_int_truncated(part_in);
    lv_draw_arc_dsc_t * draw_dsc = (lv_draw_arc_dsc_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(draw_dsc_in))->obj);

    lv_obj_init_draw_arc_dsc(obj, part, draw_dsc);
    return mp_const_none;
}

/**
 * Get the required extra size (around the object's part) to draw shadow, outline, value etc.
 * @param obj       pointer to an object
 * @param part      part of the object
 * @return          the extra size required around the object
 */
// int32_t lv_obj_calculate_ext_draw_size(lv_obj_t * obj, uint32_t part);
mp_obj_t mp_lv_obj_calculate_ext_draw_size(mp_obj_t obj_in, mp_obj_t part_in)
{
    lv_obj_t * obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    uint32_t part = (uint32_t)mp_obj_get_int_truncated(part_in);

    int32_t ret = lv_obj_calculate_ext_draw_size(obj, part);
    return mp_obj_new_int(ret);
}

/**
 * Send a 'LV_EVENT_REFR_EXT_DRAW_SIZE' Call the ancestor's event handler to the object to refresh the value of the extended draw size.
 * The result will be saved in `obj`.
 * @param obj       pointer to an object
 */
// void lv_obj_refresh_ext_draw_size(lv_obj_t * obj);
mp_obj_t mp_lv_obj_refresh_ext_draw_size(mp_obj_t obj_in)
{
    lv_obj_refresh_ext_draw_size((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_const_none;
}

/**
 * Get the extended draw area of an object.
 * @param obj       pointer to an object
 * @return          the size extended draw area around the real coordinates
 */
// int32_t _lv_obj_get_ext_draw_size(const lv_obj_t * obj);


// lv_layer_type_t _lv_obj_get_layer_type(const lv_obj_t * obj);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_OBJ_DRAW_H*/
