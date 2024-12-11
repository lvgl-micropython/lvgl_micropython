/**
 * @file lv_obj_scroll.h
 *
 */

#ifndef LV_OBJ_SCROLL_H
#define LV_OBJ_SCROLL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../misc/lv_area.h"
#include "../misc/lv_anim.h"
#include "../misc/lv_types.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/*Can't include lv_obj.h because it includes this header file*/

/** Scrollbar modes: shows when should the scrollbars be visible*/
enum _lv_scrollbar_mode_t {
    LV_SCROLLBAR_MODE_OFF,      /**< Never show scrollbars*/
    LV_SCROLLBAR_MODE_ON,       /**< Always show scrollbars*/
    LV_SCROLLBAR_MODE_ACTIVE,   /**< Show scroll bars when object is being scrolled*/
    LV_SCROLLBAR_MODE_AUTO,     /**< Show scroll bars when the content is large enough to be scrolled*/
};

#ifdef DOXYGEN
typedef _lv_scrollbar_mode_t lv_scrollbar_mode_t;
#else
typedef uint8_t lv_scrollbar_mode_t;
#endif /*DOXYGEN*/

/** Scroll span align options. Tells where to align the snappable children when scroll stops.*/
enum _lv_scroll_snap_t {
    LV_SCROLL_SNAP_NONE,    /**< Do not align, leave where it is*/
    LV_SCROLL_SNAP_START,   /**< Align to the left/top*/
    LV_SCROLL_SNAP_END,     /**< Align to the right/bottom*/
    LV_SCROLL_SNAP_CENTER   /**< Align to the center*/
};

#ifdef DOXYGEN
typedef _lv_scroll_snap_t lv_scroll_snap_t;
#else
typedef uint8_t lv_scroll_snap_t;
#endif /*DOXYGEN*/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/*=====================
 * Setter functions
 *====================*/

/**
 * Set how the scrollbars should behave.
 * @param obj       pointer to an object
 * @param mode      LV_SCROLL_MODE_ON/OFF/AUTO/ACTIVE
 */
// void lv_obj_set_scrollbar_mode(lv_obj_t * obj, lv_scrollbar_mode_t mode);
mp_obj_t mp_lv_obj_set_scrollbar_mode(mp_obj_t obj_in, mp_obj_t mode_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_scrollbar_mode_t mode = (lv_scrollbar_mode_t)mp_obj_get_int(mode_in);

    lv_obj_set_scrollbar_mode(obj, mode);
    return mp_const_none;
}
/**
 * Set the object in which directions can be scrolled
 * @param obj       pointer to an object
 * @param dir       the allow scroll directions. An element or OR-ed values of `lv_dir_t`
 */
// void lv_obj_set_scroll_dir(lv_obj_t * obj, lv_dir_t dir);
mp_obj_t mp_lv_obj_set_scroll_dir(mp_obj_t obj_in, mp_obj_t dir_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_dir_t dir = (lv_dir_t)mp_obj_get_int(dir_in);

    lv_obj_set_scroll_dir(obj, dir);
    return mp_const_none;
}
/**
 * Set where to snap the children when scrolling ends horizontally
 * @param obj       pointer to an object
 * @param align     the snap align to set from `lv_scroll_snap_t`
 */
// void lv_obj_set_scroll_snap_x(lv_obj_t * obj, lv_scroll_snap_t align);
mp_obj_t mp_lv_obj_set_scroll_snap_x(mp_obj_t obj_in, mp_obj_t align_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_scroll_snap_t align = (lv_scroll_snap_t)mp_obj_get_int(align_in);

    lv_obj_set_scroll_snap_x(obj, align);
    return mp_const_none;
}
/**
 * Set where to snap the children when scrolling ends vertically
 * @param obj       pointer to an object
 * @param align     the snap align to set from `lv_scroll_snap_t`
 */
// void lv_obj_set_scroll_snap_y(lv_obj_t * obj, lv_scroll_snap_t align);
mp_obj_t mp_lv_obj_set_scroll_snap_y(mp_obj_t obj_in, mp_obj_t align_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_scroll_snap_t align = (lv_scroll_snap_t)mp_obj_get_int(align_in);

    lv_obj_set_scroll_snap_y(obj, align);
    return mp_const_none;
}
/*=====================
 * Getter functions
 *====================*/

/**
 * Get the current scroll mode (when to hide the scrollbars)
 * @param obj       pointer to an object
 * @return          the current scroll mode from `lv_scrollbar_mode_t`
 */
// lv_scrollbar_mode_t lv_obj_get_scrollbar_mode(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_scrollbar_mode(mp_obj_t obj_in)
{
    lv_scrollbar_mode_t res = lv_obj_get_scrollbar_mode((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int((int)res);
}
/**
 * Get the object in which directions can be scrolled
 * @param obj       pointer to an object
 * @param dir       the allow scroll directions. An element or OR-ed values of `lv_dir_t`
 */
// lv_dir_t lv_obj_get_scroll_dir(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_scroll_dir(mp_obj_t obj_in)
{
    lv_dir_t res = lv_obj_get_scroll_dir((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int((int)res);
}
/**
 * Get where to snap the children when scrolling ends horizontally
 * @param obj       pointer to an object
 * @return          the current snap align from `lv_scroll_snap_t`
 */
// lv_scroll_snap_t lv_obj_get_scroll_snap_x(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_scroll_snap_x(mp_obj_t obj_in)
{
    lv_scroll_snap_t res = lv_obj_get_scroll_snap_x((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int((int)res);
}
/**
 * Get where to snap the children when scrolling ends vertically
 * @param  obj      pointer to an object
 * @return          the current snap align from `lv_scroll_snap_t`
 */
// lv_scroll_snap_t lv_obj_get_scroll_snap_y(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_scroll_snap_y(mp_obj_t obj_in)
{
    lv_scroll_snap_t res = lv_obj_get_scroll_snap_y((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int((int)res);
}
/**
 * Get current X scroll position.
 * @param obj       pointer to an object
 * @return          the current scroll position from the left edge.
 *                  If the object is not scrolled return 0
 *                  If scrolled return > 0
 *                  If scrolled in (elastic scroll) return < 0
 */
// int32_t lv_obj_get_scroll_x(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_scroll_x(mp_obj_t obj_in)
{
    int32_t res = lv_obj_get_scroll_x((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(res);
}
/**
 * Get current Y scroll position.
 * @param obj       pointer to an object
 * @return          the current scroll position from the top edge.
 *                  If the object is not scrolled return 0
 *                  If scrolled return > 0
 *                  If scrolled inside return < 0
 */
// int32_t lv_obj_get_scroll_y(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_scroll_y(mp_obj_t obj_in)
{
    int32_t res = lv_obj_get_scroll_y((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(res);
}
/**
 * Return the height of the area above the object.
 * That is the number of pixels the object can be scrolled down.
 * Normally positive but can be negative when scrolled inside.
 * @param obj       pointer to an object
 * @return          the scrollable area above the object in pixels
 */
// int32_t lv_obj_get_scroll_top(lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_scroll_top(mp_obj_t obj_in)
{
    int32_t res = lv_obj_get_scroll_top((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(res);
}
/**
 * Return the height of the area below the object.
 * That is the number of pixels the object can be scrolled down.
 * Normally positive but can be negative when scrolled inside.
 * @param obj       pointer to an object
 * @return          the scrollable area below the object in pixels
 */
// int32_t lv_obj_get_scroll_bottom(lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_scroll_bottom(mp_obj_t obj_in)
{
    int32_t res = lv_obj_get_scroll_bottom((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(res);
}
/**
 * Return the width of the area on the left the object.
 * That is the number of pixels the object can be scrolled down.
 * Normally positive but can be negative when scrolled inside.
 * @param obj       pointer to an object
 * @return          the scrollable area on the left the object in pixels
 */
// int32_t lv_obj_get_scroll_left(lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_scroll_left(mp_obj_t obj_in)
{
    int32_t res = lv_obj_get_scroll_left((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(res);
}
/**
 * Return the width of the area on the right the object.
 * That is the number of pixels the object can be scrolled down.
 * Normally positive but can be negative when scrolled inside.
 * @param obj       pointer to an object
 * @return          the scrollable area on the right the object in pixels
 */
// int32_t lv_obj_get_scroll_right(lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_scroll_right(mp_obj_t obj_in)
{
    int32_t res = lv_obj_get_scroll_right((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(res);
}
/**
 * Get the X and Y coordinates where the scrolling will end for this object if a scrolling animation is in progress.
 * If no scrolling animation, give the current `x` or `y` scroll position.
 * @param obj       pointer to an object
 * @param end       pointer to store the result
 */
// void lv_obj_get_scroll_end(lv_obj_t * obj, lv_point_t * end);
mp_obj_t mp_lv_obj_get_scroll_end(mp_obj_t obj_in, mp_obj_t end_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_point_t *end = (lv_point_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(end_in))->obj);

    lv_obj_get_scroll_end(obj, end);
    return mp_const_none;
}
/*=====================
 * Other functions
 *====================*/

/**
 * Scroll by a given amount of pixels
 * @param obj       pointer to an object to scroll
 * @param x         pixels to scroll horizontally
 * @param y         pixels to scroll vertically
 * @param anim_en   LV_ANIM_ON: scroll with animation; LV_ANIM_OFF: scroll immediately
 * @note            > 0 value means scroll right/bottom (show the more content on the right/bottom)
 * @note            e.g. dy = -20 means scroll down 20 px
 */
// void lv_obj_scroll_by(lv_obj_t * obj, int32_t x, int32_t y, lv_anim_enable_t anim_en);
mp_obj_t mp_lv_obj_scroll_by(mp_obj_t obj_in, mp_obj_t x_in, mp_obj_t y_in, mp_obj_t anim_en_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t x = (int32_t)mp_obj_get_int(x_in);
    int32_t y = (int32_t)mp_obj_get_int(y_in);
    lv_anim_enable_t anim_en = (lv_anim_enable_t)mp_obj_get_int(anim_en_in);

    lv_obj_scroll_by(obj, x, y, anim_en);
    return mp_const_none;
}
/**
 * Scroll by a given amount of pixels.
 * `dx` and `dy` will be limited internally to allow scrolling only on the content area.
 * @param obj       pointer to an object to scroll
 * @param dx        pixels to scroll horizontally
 * @param dy        pixels to scroll vertically
 * @param anim_en   LV_ANIM_ON: scroll with animation; LV_ANIM_OFF: scroll immediately
 * @note            e.g. dy = -20 means scroll down 20 px
 */
// void lv_obj_scroll_by_bounded(lv_obj_t * obj, int32_t dx, int32_t dy, lv_anim_enable_t anim_en);
mp_obj_t mp_lv_obj_scroll_by_bounded(mp_obj_t obj_in, mp_obj_t dx_in, mp_obj_t dy_in, mp_obj_t anim_en_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t dx = (int32_t)mp_obj_get_int(dx_in);
    int32_t dy = (int32_t)mp_obj_get_int(dy_in);
    lv_anim_enable_t anim_en = (lv_anim_enable_t)mp_obj_get_int(anim_en_in);

    lv_obj_scroll_by_bounded(obj, dx, dy, anim_en);
    return mp_const_none;
}
/**
 * Scroll to a given coordinate on an object.
 * `x` and `y` will be limited internally to allow scrolling only on the content area.
 * @param obj       pointer to an object to scroll
 * @param x         pixels to scroll horizontally
 * @param y         pixels to scroll vertically
 * @param anim_en   LV_ANIM_ON: scroll with animation; LV_ANIM_OFF: scroll immediately
 */
// void lv_obj_scroll_to(lv_obj_t * obj, int32_t x, int32_t y, lv_anim_enable_t anim_en);
mp_obj_t mp_lv_obj_scroll_to(mp_obj_t obj_in, mp_obj_t x_in, mp_obj_t y_in, mp_obj_t anim_en_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t x = (int32_t)mp_obj_get_int(x_in);
    int32_t y = (int32_t)mp_obj_get_int(y_in);
    lv_anim_enable_t anim_en = (lv_anim_enable_t)mp_obj_get_int(anim_en_in);

    lv_obj_scroll_to(obj, x, y, anim_en);
    return mp_const_none;
}
/**
 * Scroll to a given X coordinate on an object.
 * `x` will be limited internally to allow scrolling only on the content area.
 * @param obj       pointer to an object to scroll
 * @param x         pixels to scroll horizontally
 * @param anim_en   LV_ANIM_ON: scroll with animation; LV_ANIM_OFF: scroll immediately
 */
// void lv_obj_scroll_to_x(lv_obj_t * obj, int32_t x, lv_anim_enable_t anim_en);
mp_obj_t mp_lv_obj_scroll_to_x(mp_obj_t obj_in, mp_obj_t x_in, mp_obj_t anim_en_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t x = (int32_t)mp_obj_get_int(x_in);
    lv_anim_enable_t anim_en = (lv_anim_enable_t)mp_obj_get_int(anim_en_in);

    lv_obj_scroll_to_x(obj, x, anim_en);
    return mp_const_none;
}
/**
 * Scroll to a given Y coordinate on an object
 * `y` will be limited internally to allow scrolling only on the content area.
 * @param obj       pointer to an object to scroll
 * @param y         pixels to scroll vertically
 * @param anim_en   LV_ANIM_ON: scroll with animation; LV_ANIM_OFF: scroll immediately
 */
// void lv_obj_scroll_to_y(lv_obj_t * obj, int32_t y, lv_anim_enable_t anim_en);
mp_obj_t mp_lv_obj_scroll_to_y(mp_obj_t obj_in, mp_obj_t y_in, mp_obj_t anim_en_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t y = (int32_t)mp_obj_get_int(y_in);
    lv_anim_enable_t anim_en = (lv_anim_enable_t)mp_obj_get_int(anim_en_in);

    lv_obj_scroll_to_y(obj, y, anim_en);
    return mp_const_none;
}
/**
 * Scroll to an object until it becomes visible on its parent
 * @param obj       pointer to an object to scroll into view
 * @param anim_en   LV_ANIM_ON: scroll with animation; LV_ANIM_OFF: scroll immediately
 */
// void lv_obj_scroll_to_view(lv_obj_t * obj, lv_anim_enable_t anim_en);
mp_obj_t mp_lv_obj_scroll_to_view(mp_obj_t obj_in, mp_obj_t anim_en_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_anim_enable_t anim_en = (lv_anim_enable_t)mp_obj_get_int(anim_en_in);

    lv_obj_scroll_to_view(obj, anim_en);
    return mp_const_none;
}
/**
 * Scroll to an object until it becomes visible on its parent.
 * Do the same on the parent's parent, and so on.
 * Therefore the object will be scrolled into view even it has nested scrollable parents
 * @param obj       pointer to an object to scroll into view
 * @param anim_en   LV_ANIM_ON: scroll with animation; LV_ANIM_OFF: scroll immediately
 */
// void lv_obj_scroll_to_view_recursive(lv_obj_t * obj, lv_anim_enable_t anim_en);
mp_obj_t mp_lv_obj_scroll_to_view_recursive(mp_obj_t obj_in, mp_obj_t anim_en_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_anim_enable_t anim_en = (lv_anim_enable_t)mp_obj_get_int(anim_en_in);

    lv_obj_scroll_to_view_recursive(obj, anim_en);
    return mp_const_none;
}
/**
 * Low level function to scroll by given x and y coordinates.
 * `LV_EVENT_SCROLL` is sent.
 * @param obj       pointer to an object to scroll
 * @param x         pixels to scroll horizontally
 * @param y         pixels to scroll vertically
 * @return          `LV_RESULT_INVALID`: to object was deleted in `LV_EVENT_SCROLL`;
 *                  `LV_RESULT_OK`: if the object is still valid
 */
// lv_result_t _lv_obj_scroll_by_raw(lv_obj_t * obj, int32_t x, int32_t y);

/**
 * Tell whether an object is being scrolled or not at this moment
 * @param obj   pointer to an object
 * @return      true: `obj` is being scrolled
 */
// bool lv_obj_is_scrolling(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_is_scrolling(mp_obj_t obj_in)
{
    bool res = lv_obj_is_scrolling((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_bool(res);
}
/**
 * Check the children of `obj` and scroll `obj` to fulfill the scroll_snap settings
 * @param obj       an object whose children needs to checked and snapped
 * @param anim_en   LV_ANIM_ON/OFF
 */
// void lv_obj_update_snap(lv_obj_t * obj, lv_anim_enable_t anim_en);
mp_obj_t mp_lv_obj_update_snap(mp_obj_t obj_in, mp_obj_t anim_en_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_anim_enable_t anim_en = (lv_anim_enable_t)mp_obj_get_int(anim_en_in);

    lv_obj_update_snap(obj, anim_en);
    return mp_const_none;
}
/**
 * Get the area of the scrollbars
 * @param obj   pointer to an object
 * @param hor   pointer to store the area of the horizontal scrollbar
 * @param ver   pointer to store the area of the vertical  scrollbar
 */
// void lv_obj_get_scrollbar_area(lv_obj_t * obj, lv_area_t * hor, lv_area_t * ver);
mp_obj_t mp_lv_obj_get_scrollbar_area(mp_obj_t obj_in, mp_obj_t hor_in, mp_obj_t ver_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_area_t *hor = (lv_area_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(hor_in))->obj);
    lv_area_t *ver = (lv_area_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(ver_in))->obj);

    lv_obj_get_scrollbar_area(obj, hor, ver);
    return mp_const_none;
}
/**
 * Invalidate the area of the scrollbars
 * @param obj       pointer to an object
 */
// void lv_obj_scrollbar_invalidate(lv_obj_t * obj);
mp_obj_t mp_lv_obj_scrollbar_invalidate(mp_obj_t obj_in)
{
    lv_obj_scrollbar_invalidate((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_const_none;
}
/**
 * Checks if the content is scrolled "in" and adjusts it to a normal position.
 * @param obj       pointer to an object
 * @param anim_en   LV_ANIM_ON/OFF
 */
// void lv_obj_readjust_scroll(lv_obj_t * obj, lv_anim_enable_t anim_en);
mp_obj_t mp_lv_obj_readjust_scroll(mp_obj_t obj_in, mp_obj_t anim_en_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_anim_enable_t anim_en = (lv_anim_enable_t)mp_obj_get_int(anim_en_in);

    lv_obj_readjust_scroll(obj, anim_en);
    return mp_const_none;
}
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_OBJ_SCROLL_H*/
