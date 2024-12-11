/**
 * @file lv_obj_pos.h
 *
 */

#ifndef LV_OBJ_POS_H
#define LV_OBJ_POS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../misc/lv_area.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    /** No flags */
    LV_OBJ_POINT_TRANSFORM_FLAG_NONE = 0x00,

    /** Consider the transformation properties of the parents too */
    LV_OBJ_POINT_TRANSFORM_FLAG_RECURSIVE = 0x01,

    /** Execute the inverse of the transformation (-angle and 1/zoom) */
    LV_OBJ_POINT_TRANSFORM_FLAG_INVERSE = 0x02,

    /** Both inverse and recursive*/
    LV_OBJ_POINT_TRANSFORM_FLAG_INVERSE_RECURSIVE = 0x03,
} lv_obj_point_transform_flag_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Set the position of an object relative to the set alignment.
 * @param obj       pointer to an object
 * @param x         new x coordinate
 * @param y         new y coordinate
 * @note            With default alignment it's the distance from the top left corner
 * @note            E.g. LV_ALIGN_CENTER alignment it's the offset from the center of the parent
 * @note            The position is interpreted on the content area of the parent
 * @note            The values can be set in pixel or in percentage of parent size with `lv_pct(v)`
 */
// void lv_obj_set_pos(lv_obj_t * obj, int32_t x, int32_t y);
mp_obj_t mp_lv_obj_set_pos(mp_obj_t obj_in, mp_obj_t x_in, mp_obj_t y_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t x = (int32_t)mp_obj_get_int(x_in);
    int32_t y = (int32_t)mp_obj_get_int(y_in);

    lv_obj_set_pos(obj, x, y);
    return mp_const_none;
}

/**
 * Set the x coordinate of an object
 * @param obj       pointer to an object
 * @param x         new x coordinate
 * @note            With default alignment it's the distance from the top left corner
 * @note            E.g. LV_ALIGN_CENTER alignment it's the offset from the center of the parent
 * @note            The position is interpreted on the content area of the parent
 * @note            The values can be set in pixel or in percentage of parent size with `lv_pct(v)`
 */
// void lv_obj_set_x(lv_obj_t * obj, int32_t x);
mp_obj_t mp_lv_obj_set_x(mp_obj_t obj_in, mp_obj_t x_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t x = (int32_t)mp_obj_get_int(x_in);

    lv_obj_set_x(obj, x);
    return mp_const_none;
}

/**
 * Set the y coordinate of an object
 * @param obj       pointer to an object
 * @param y         new y coordinate
 * @note            With default alignment it's the distance from the top left corner
 * @note            E.g. LV_ALIGN_CENTER alignment it's the offset from the center of the parent
 * @note            The position is interpreted on the content area of the parent
 * @note            The values can be set in pixel or in percentage of parent size with `lv_pct(v)`
 */
// void lv_obj_set_y(lv_obj_t * obj, int32_t y);
mp_obj_t mp_lv_obj_set_y(mp_obj_t obj_in, mp_obj_t y_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t y = (int32_t)mp_obj_get_int(y_in);

    lv_obj_set_y(obj, y);
    return mp_const_none;
}

/**
 * Set the size of an object.
 * @param obj       pointer to an object
 * @param w         the new width
 * @param h         the new height
 * @note            possible values are:
 *                  pixel               simple set the size accordingly
 *                  LV_SIZE_CONTENT     set the size to involve all children in the given direction
 *                  lv_pct(x)           to set size in percentage of the parent's content area size (the size without paddings).
 *                                      x should be in [0..1000]% range
 */
// void lv_obj_set_size(lv_obj_t * obj, int32_t w, int32_t h);
mp_obj_t mp_lv_obj_set_size(mp_obj_t obj_in, mp_obj_t w_in, mp_obj_t h_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t w = (int32_t)mp_obj_get_int(w_in);
    int32_t h = (int32_t)mp_obj_get_int(h_in);

    lv_obj_set_size(obj, w, h);
    return mp_const_none;
}
/**
 * Recalculate the size of the object
 * @param obj       pointer to an object
 * @return          true: the size has been changed
 */
// bool lv_obj_refr_size(lv_obj_t * obj);
mp_obj_t mp_lv_obj_refr_size(mp_obj_t obj_in)
{
    bool ret = lv_obj_refr_size((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_bool(ret);
}

/**
 * Set the width of an object
 * @param obj       pointer to an object
 * @param w         the new width
 * @note            possible values are:
 *                  pixel               simple set the size accordingly
 *                  LV_SIZE_CONTENT     set the size to involve all children in the given direction
 *                  lv_pct(x)           to set size in percentage of the parent's content area size (the size without paddings).
 *                                      x should be in [0..1000]% range
 */
// void lv_obj_set_width(lv_obj_t * obj, int32_t w);
mp_obj_t mp_lv_obj_set_width(mp_obj_t obj_in, mp_obj_t w_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t w = (int32_t)mp_obj_get_int(w_in);

    lv_obj_set_width(obj, w);
    return mp_const_none;
}
/**
 * Set the height of an object
 * @param obj       pointer to an object
 * @param h         the new height
 * @note            possible values are:
 *                  pixel               simple set the size accordingly
 *                  LV_SIZE_CONTENT     set the size to involve all children in the given direction
 *                  lv_pct(x)           to set size in percentage of the parent's content area size (the size without paddings).
 *                                      x should be in [0..1000]% range
 */
// void lv_obj_set_height(lv_obj_t * obj, int32_t h);
mp_obj_t mp_lv_obj_set_height(mp_obj_t obj_in, mp_obj_t h_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t h = (int32_t)mp_obj_get_int(h_in);

    lv_obj_set_height(obj, h);
    return mp_const_none;
}
/**
 * Set the width reduced by the left and right padding and the border width.
 * @param obj       pointer to an object
 * @param w         the width without paddings in pixels
 */
// void lv_obj_set_content_width(lv_obj_t * obj, int32_t w);
mp_obj_t mp_lv_obj_set_content_width(mp_obj_t obj_in, mp_obj_t w_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t w = (int32_t)mp_obj_get_int(w_in);

    lv_obj_set_content_width(obj, w);
    return mp_const_none;
}
/**
 * Set the height reduced by the top and bottom padding and the border width.
 * @param obj       pointer to an object
 * @param h         the height without paddings in pixels
 */
// void lv_obj_set_content_height(lv_obj_t * obj, int32_t h);
mp_obj_t mp_lv_obj_set_content_height(mp_obj_t obj_in, mp_obj_t h_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t h = (int32_t)mp_obj_get_int(h_in);

    lv_obj_set_content_height(obj, h);
    return mp_const_none;
}
/**
 * Set a layout for an object
 * @param obj       pointer to an object
 * @param layout    pointer to a layout descriptor to set
 */
// void lv_obj_set_layout(lv_obj_t * obj, uint32_t layout);
mp_obj_t mp_lv_obj_set_layout(mp_obj_t obj_in, mp_obj_t layout_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    uint32_t layout = (uint32_t)mp_obj_get_int_truncated(layout_in);

    lv_obj_set_layout(obj, layout);
    return mp_const_none;
}
/**
 * Test whether the and object is positioned by a layout or not
 * @param obj       pointer to an object to test
 * @return true:    positioned by a layout; false: not positioned by a layout
 */
// bool lv_obj_is_layout_positioned(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_is_layout_positioned(mp_obj_t obj_in)
{
    bool ret = lv_obj_is_layout_positioned((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_bool(ret);
}
/**
 * Mark the object for layout update.
 * @param obj      pointer to an object whose children needs to be updated
 */
// void lv_obj_mark_layout_as_dirty(lv_obj_t * obj);
mp_obj_t mp_lv_obj_mark_layout_as_dirty(mp_obj_t obj_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);

    lv_obj_mark_layout_as_dirty(obj);
    return mp_const_none;
}
/**
 * Update the layout of an object.
 * @param obj      pointer to an object whose children needs to be updated
 */
// void lv_obj_update_layout(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_update_layout(mp_obj_t obj_in)
{
    lv_obj_update_layout((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_const_none;
}
/**
 * Change the alignment of an object.
 * @param obj       pointer to an object to align
 * @param align     type of alignment (see 'lv_align_t' enum) `LV_ALIGN_OUT_...` can't be used.
 */
// void lv_obj_set_align(lv_obj_t * obj, lv_align_t align);
mp_obj_t mp_lv_obj_set_align(mp_obj_t obj_in, mp_obj_t align_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_align_t align = (lv_align_t)mp_obj_get_int(align_in);

    lv_obj_set_align(obj, align);
    return mp_const_none;
}
/**
 * Change the alignment of an object and set new coordinates.
 * Equivalent to:
 * lv_obj_set_align(obj, align);
 * lv_obj_set_pos(obj, x_ofs, y_ofs);
 * @param obj       pointer to an object to align
 * @param align     type of alignment (see 'lv_align_t' enum) `LV_ALIGN_OUT_...` can't be used.
 * @param x_ofs     x coordinate offset after alignment
 * @param y_ofs     y coordinate offset after alignment
 */
// void lv_obj_align(lv_obj_t * obj, lv_align_t align, int32_t x_ofs, int32_t y_ofs);
mp_obj_t mp_lv_obj_align(mp_obj_t obj_in, mp_obj_t align_in, mp_obj_t x_ofs_in, mp_obj_t y_ofs_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_align_t align = (lv_align_t)mp_obj_get_int(align_in);
    int32_t x_ofs = (int32_t)mp_obj_get_int(x_ofs_in);
    int32_t y_ofs = (int32_t)mp_obj_get_int(y_ofs_in);

    lv_obj_align(obj, align, x_ofs, y_ofs);
    return mp_const_none;
}


/**
 * Align an object to an other object.
 * @param obj       pointer to an object to align
 * @param base      pointer to an other object (if NULL `obj`s parent is used). 'obj' will be aligned to it.
 * @param align     type of alignment (see 'lv_align_t' enum)
 * @param x_ofs     x coordinate offset after alignment
 * @param y_ofs     y coordinate offset after alignment
 * @note            if the position or size of `base` changes `obj` needs to be aligned manually again
 */
//void lv_obj_align_to(lv_obj_t * obj, const lv_obj_t * base, lv_align_t align, int32_t x_ofs,
//                     int32_t y_ofs);

mp_obj_t mp_lv_obj_align_to(mp_obj_t obj_in, mp_obj_t base_in, mp_obj_t align_in, mp_obj_t x_ofs_in, mp_obj_t y_ofs_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_obj_t *base = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(base_in))->obj);
    lv_align_t align = (lv_align_t)mp_obj_get_int(align_in);
    int32_t x_ofs = (int32_t)mp_obj_get_int(x_ofs_in);
    int32_t y_ofs = (int32_t)mp_obj_get_int(y_ofs_in);

    lv_obj_align_to(obj, base, align, x_ofs, y_ofs);
    return mp_const_none;
}

/**
 * Align an object to the center on its parent.
 * @param obj       pointer to an object to align
 * @note            if the parent size changes `obj` needs to be aligned manually again
 */
//static inline void lv_obj_center(lv_obj_t * obj)
//{
//    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
//}
mp_obj_t mp_lv_obj_center(mp_obj_t obj_in)
{
    lv_obj_center((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_const_none;
}

/**
 * Copy the coordinates of an object to an area
 * @param obj       pointer to an object
 * @param coords    pointer to an area to store the coordinates
 */
// void lv_obj_get_coords(const lv_obj_t * obj, lv_area_t * coords);
mp_obj_t mp_lv_obj_get_coords(mp_obj_t obj_in, mp_obj_t coords_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_area_t *coords = (lv_area_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(coords_in))->obj);
    
    lv_obj_get_coords(obj, coords);
    return mp_const_none;
}
/**
 * Get the x coordinate of object.
 * @param obj       pointer to an object
 * @return          distance of `obj` from the left side of its parent plus the parent's left padding
 * @note            The position of the object is recalculated only on the next redraw. To force coordinate recalculation
 *                  call `lv_obj_update_layout(obj)`.
 * @note            Zero return value means the object is on the left padding of the parent, and not on the left edge.
 * @note            Scrolling of the parent doesn't change the returned value.
 * @note            The returned value is always the distance from the parent even if `obj` is positioned by a layout.
 */
// int32_t lv_obj_get_x(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_x(mp_obj_t obj_in)
{   
    int32_t ret = lv_obj_get_x((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(ret);
}

/**
 * Get the x2 coordinate of object.
 * @param obj       pointer to an object
 * @return          distance of `obj` from the right side of its parent plus the parent's right padding
 * @note            The position of the object is recalculated only on the next redraw. To force coordinate recalculation
 *                  call `lv_obj_update_layout(obj)`.
 * @note            Zero return value means the object is on the right padding of the parent, and not on the right edge.
 * @note            Scrolling of the parent doesn't change the returned value.
 * @note            The returned value is always the distance from the parent even if `obj` is positioned by a layout.
 */
// int32_t lv_obj_get_x2(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_x2(mp_obj_t obj_in)
{   
    int32_t ret = lv_obj_get_x2((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(ret);
}

/**
 * Get the y coordinate of object.
 * @param obj       pointer to an object
 * @return          distance of `obj` from the top side of its parent plus the parent's top padding
 * @note            The position of the object is recalculated only on the next redraw. To force coordinate recalculation
 *                  call `lv_obj_update_layout(obj)`.
 * @note            Zero return value means the object is on the top padding of the parent, and not on the top edge.
 * @note            Scrolling of the parent doesn't change the returned value.
 * @note            The returned value is always the distance from the parent even if `obj` is positioned by a layout.
 */
// int32_t lv_obj_get_y(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_y(mp_obj_t obj_in)
{   
    int32_t ret = lv_obj_get_y((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(ret);
}

/**
 * Get the y2 coordinate of object.
 * @param obj       pointer to an object
 * @return          distance of `obj` from the bottom side of its parent plus the parent's bottom padding
 * @note            The position of the object is recalculated only on the next redraw. To force coordinate recalculation
 *                  call `lv_obj_update_layout(obj)`.
 * @note            Zero return value means the object is on the bottom padding of the parent, and not on the bottom edge.
 * @note            Scrolling of the parent doesn't change the returned value.
 * @note            The returned value is always the distance from the parent even if `obj` is positioned by a layout.
 */
// int32_t lv_obj_get_y2(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_y2(mp_obj_t obj_in)
{   
    int32_t ret = lv_obj_get_y2((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(ret);
}

/**
 * Get the actually set x coordinate of object, i.e. the offset form the set alignment
 * @param obj       pointer to an object
 * @return          the set x coordinate
 */
// int32_t lv_obj_get_x_aligned(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_x_aligned(mp_obj_t obj_in)
{   
    int32_t ret = lv_obj_get_x_aligned((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(ret);
}

/**
 * Get the actually set y coordinate of object, i.e. the offset form the set alignment
 * @param obj       pointer to an object
 * @return          the set y coordinate
 */
// int32_t lv_obj_get_y_aligned(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_y_aligned(mp_obj_t obj_in)
{   
    int32_t ret = lv_obj_get_y_aligned((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(ret);
}

/**
 * Get the width of an object
 * @param obj       pointer to an object
 * @note            The position of the object is recalculated only on the next redraw. To force coordinate recalculation
 *                  call `lv_obj_update_layout(obj)`.
 * @return          the width in pixels
 */
// int32_t lv_obj_get_width(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_width(mp_obj_t obj_in)
{   
    int32_t ret = lv_obj_get_width((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(ret);
}

/**
 * Get the height of an object
 * @param obj       pointer to an object
 * @note            The position of the object is recalculated only on the next redraw. To force coordinate recalculation
 *                  call `lv_obj_update_layout(obj)`.
 * @return          the height in pixels
 */
// int32_t lv_obj_get_height(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_height(mp_obj_t obj_in)
{   
    int32_t ret = lv_obj_get_height((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(ret);
}

/**
 * Get the width reduced by the left and right padding and the border width.
 * @param obj       pointer to an object
 * @note            The position of the object is recalculated only on the next redraw. To force coordinate recalculation
 *                  call `lv_obj_update_layout(obj)`.
 * @return          the width which still fits into its parent without causing overflow (making the parent scrollable)
 */
// int32_t lv_obj_get_content_width(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_content_width(mp_obj_t obj_in)
{   
    int32_t ret = lv_obj_get_content_width((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(ret);
}

/**
 * Get the height reduced by the top and bottom padding and the border width.
 * @param obj       pointer to an object
 * @note            The position of the object is recalculated only on the next redraw. To force coordinate recalculation
 *                  call `lv_obj_update_layout(obj)`.
 * @return          the height which still fits into the parent without causing overflow (making the parent scrollable)
 */
// int32_t lv_obj_get_content_height(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_content_height(mp_obj_t obj_in)
{   
    int32_t ret = lv_obj_get_content_height((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(ret);
}

/**
 * Get the area reduced by the paddings and the border width.
 * @param obj       pointer to an object
 * @note            The position of the object is recalculated only on the next redraw. To force coordinate recalculation
 *                  call `lv_obj_update_layout(obj)`.
 * @param area      the area which still fits into the parent without causing overflow (making the parent scrollable)
 */
// void lv_obj_get_content_coords(const lv_obj_t * obj, lv_area_t * area);
mp_obj_t mp_lv_obj_get_content_coords(mp_obj_t obj_in, mp_obj_t coords_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_area_t *coords = (lv_area_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(coords_in))->obj);
    
    lv_obj_get_content_coords(obj, coords);
    return mp_const_none;
}

/**
 * Get the width occupied by the "parts" of the widget. E.g. the width of all columns of a table.
 * @param obj       pointer to an objects
 * @return          the width of the virtually drawn content
 * @note            This size independent from the real size of the widget.
 *                  It just tells how large the internal ("virtual") content is.
 */
// int32_t lv_obj_get_self_width(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_self_width(mp_obj_t obj_in)
{   
    int32_t ret = lv_obj_get_self_width((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(ret);
}

/**
 * Get the height occupied by the "parts" of the widget. E.g. the height of all rows of a table.
 * @param obj       pointer to an objects
 * @return          the width of the virtually drawn content
 * @note            This size independent from the real size of the widget.
 *                  It just tells how large the internal ("virtual") content is.
 */
// int32_t lv_obj_get_self_height(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_self_height(mp_obj_t obj_in)
{   
    int32_t ret = lv_obj_get_self_height((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(ret);
}

/**
 * Handle if the size of the internal ("virtual") content of an object has changed.
 * @param obj       pointer to an object
 * @return          false: nothing happened; true: refresh happened
 */
// bool lv_obj_refresh_self_size(lv_obj_t * obj);
mp_obj_t mp_lv_obj_refresh_self_size(mp_obj_t obj_in)
{   
    bool ret = lv_obj_refresh_self_size((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_bool(ret);
}

// void lv_obj_refr_pos(lv_obj_t * obj);
mp_obj_t mp_lv_obj_refr_pos(mp_obj_t obj_in)
{   
    lv_obj_refr_pos((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_const_none;
}

// void lv_obj_move_to(lv_obj_t * obj, int32_t x, int32_t y);
mp_obj_t mp_lv_obj_move_to(mp_obj_t obj_in, mp_obj_t x_in, mp_obj_t y_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t x = (int32_t)mp_obj_get_int(x_in);
    int32_t y = (int32_t)mp_obj_get_int(y_in);

    lv_obj_move_to(obj, x, y);
    return mp_const_none;
}

// void lv_obj_move_children_by(lv_obj_t * obj, int32_t x_diff, int32_t y_diff, bool ignore_floating);
mp_obj_t mp_lv_obj_move_children_by(mp_obj_t obj_in, mp_obj_t x_diff_in, mp_obj_t y_diff_in, mp_obj_t ignore_floating_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t x_diff = (int32_t)mp_obj_get_int(x_diff_in);
    int32_t y_diff = (int32_t)mp_obj_get_int(y_diff_in);
    bool ignore_floating = (bool)mp_obj_get_int(ignore_floating_in);
    
    lv_obj_move_children_by(obj, x_diff, y_diff, ignore_floating);
    return mp_const_none;
}

/**
 * Transform a point using the angle and zoom style properties of an object
 * @param obj           pointer to an object whose style properties should be used
 * @param p             a point to transform, the result will be written back here too
 * @param flags         OR-ed valued of :cpp:enum:`lv_obj_point_transform_flag_t`
 */
// void lv_obj_transform_point(const lv_obj_t * obj, lv_point_t * p, lv_obj_point_transform_flag_t flags);
mp_obj_t mp_lv_obj_transform_point(mp_obj_t obj_in, mp_obj_t p_in, mp_obj_t flags_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_point_t *p = (lv_point_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(p_in))->obj);
    lv_obj_point_transform_flag_t flags = (lv_obj_point_transform_flag_t)mp_obj_get_int(flags_in);
    
    lv_obj_transform_point(obj, p, flags);
    return mp_const_none;
}

/**
 * Transform an array of points using the angle and zoom style properties of an object
 * @param obj           pointer to an object whose style properties should be used
 * @param points        the array of points to transform, the result will be written back here too
 * @param count         number of points in the array
 * @param flags         OR-ed valued of :cpp:enum:`lv_obj_point_transform_flag_t`
 */
// TODO:
void lv_obj_transform_point_array(const lv_obj_t * obj, lv_point_t points[], size_t count,
                                  lv_obj_point_transform_flag_t flags);

/**
 * Transform an area using the angle and zoom style properties of an object
 * @param obj           pointer to an object whose style properties should be used
 * @param area          an area to transform, the result will be written back here too
 * @param flags         OR-ed valued of :cpp:enum:`lv_obj_point_transform_flag_t`
 */
// void lv_obj_get_transformed_area(const lv_obj_t * obj, lv_area_t * area, lv_obj_point_transform_flag_t flags);
mp_obj_t mp_lv_obj_get_transformed_area(mp_obj_t obj_in, mp_obj_t p_in, mp_obj_t flags_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_point_t *p = (lv_point_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(p_in))->obj);
    lv_obj_point_transform_flag_t flags = (lv_obj_point_transform_flag_t)mp_obj_get_int(flags_in);
    
    lv_obj_get_transformed_area(obj, p, flags);
    return mp_const_none;
}

/**
 * Mark an area of an object as invalid.
 * The area will be truncated to the object's area and marked for redraw.
 * @param obj       pointer to an object
 * @param           area the area to redraw
 */
// void lv_obj_invalidate_area(const lv_obj_t * obj, const lv_area_t * area);
mp_obj_t mp_lv_obj_invalidate_area(mp_obj_t obj_in, mp_obj_t area_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_area_t *area = (lv_area_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(area_in))->obj);
    
    lv_obj_invalidate_area(obj, area);
    return mp_const_none;
}

/**
 * Mark the object as invalid to redrawn its area
 * @param obj       pointer to an object
 */
// void lv_obj_invalidate(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_invalidate(mp_obj_t obj_in)
{
    lv_obj_invalidate((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_const_none;
}

/**
 * Tell whether an area of an object is visible (even partially) now or not
 * @param obj       pointer to an object
 * @param area      the are to check. The visible part of the area will be written back here.
 * @return true     visible; false not visible (hidden, out of parent, on other screen, etc)
 */
// bool lv_obj_area_is_visible(const lv_obj_t * obj, lv_area_t * area);
mp_obj_t mp_lv_obj_area_is_visible(mp_obj_t obj_in, mp_obj_t area_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_area_t *area = (lv_area_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(area_in))->obj);
    
    bool ret = lv_obj_area_is_visible(obj, area);
    return mp_obj_new_bool(ret);
}

/**
 * Tell whether an object is visible (even partially) now or not
 * @param obj       pointer to an object
 * @return      true: visible; false not visible (hidden, out of parent, on other screen, etc)
 */
// bool lv_obj_is_visible(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_is_visible(mp_obj_t obj_in)
{    
    bool ret = lv_obj_is_visible((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_bool(ret);
}

/**
 * Set the size of an extended clickable area
 * @param obj       pointer to an object
 * @param size      extended clickable area in all 4 directions [px]
 */
// void lv_obj_set_ext_click_area(lv_obj_t * obj, int32_t size);
mp_obj_t mp_lv_obj_set_ext_click_area(mp_obj_t obj_in, mp_obj_t size_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t size = (int32_t)mp_obj_get_int(size_in);
    
    lv_obj_set_ext_click_area(obj, size);
    return mp_const_none;
}

/**
 * Get the an area where to object can be clicked.
 * It's the object's normal area plus the extended click area.
 * @param obj       pointer to an object
 * @param area      store the result area here
 */
// void lv_obj_get_click_area(const lv_obj_t * obj, lv_area_t * area);
mp_obj_t mp_lv_obj_get_click_area(mp_obj_t obj_in, mp_obj_t area_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_area_t *area = (lv_area_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(area_in))->obj);
    
    lv_obj_get_click_area(obj, area);
    return mp_const_none;
}

/**
 * Hit-test an object given a particular point in screen space.
 * @param obj       object to hit-test
 * @param point     screen-space point (absolute coordinate)
 * @return          true: if the object is considered under the point
 */
// bool lv_obj_hit_test(lv_obj_t * obj, const lv_point_t * point);
mp_obj_t mp_lv_obj_hit_test(mp_obj_t obj_in, mp_obj_t point_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_point_t *point = (lv_point_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(point_in))->obj);
    
    bool ret = lv_obj_hit_test(obj, point);
    return mp_obj_new_bool(ret);
}
/**
 * Clamp a width between min and max width. If the min/max width is in percentage value use the ref_width
 * @param width         width to clamp
 * @param min_width     the minimal width
 * @param max_width     the maximal width
 * @param ref_width     the reference width used when min/max width is in percentage
 * @return              the clamped width
 */
// int32_t lv_clamp_width(int32_t width, int32_t min_width, int32_t max_width, int32_t ref_width);
mp_obj_t mp_lv_clamp_width(mp_obj_t width_in, mp_obj_t min_width_in, mp_obj_t max_width_in, mp_obj_t ref_width_in)
{
    int32_t width = (int32_t)mp_obj_get_int(width_in);
    int32_t min_width = (int32_t)mp_obj_get_int(min_width_in);
    int32_t max_width = (int32_t)mp_obj_get_int(max_width_in);
    int32_t ref_width = (int32_t)mp_obj_get_int(ref_width_in);

    int32_t ret = lv_clamp_width(width, min_width, max_width, ref_width);
    return mp_obj_new_int(ret);
}

/**
 * Clamp a height between min and max height. If the min/max height is in percentage value use the ref_height
 * @param height         height to clamp
 * @param min_height     the minimal height
 * @param max_height     the maximal height
 * @param ref_height     the reference height used when min/max height is in percentage
 * @return              the clamped height
 */
// int32_t lv_clamp_height(int32_t height, int32_t min_height, int32_t max_height, int32_t ref_height);
mp_obj_t mp_lv_clamp_height(mp_obj_t height_in, mp_obj_t min_height_in, mp_obj_t max_height_in, mp_obj_t ref_height_in)
{
    int32_t height = (int32_t)mp_obj_get_int(height_in);
    int32_t min_height = (int32_t)mp_obj_get_int(min_height_in);
    int32_t max_height = (int32_t)mp_obj_get_int(max_height_in);
    int32_t ref_height = (int32_t)mp_obj_get_int(ref_height_in);

    int32_t ret = lv_clamp_height(height, min_height, max_height, ref_height);
    return mp_obj_new_int(ret);
}

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_OBJ_POS_H*/
