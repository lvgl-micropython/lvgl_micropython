/**
 * @file lv_obj_event.h
 *
 */

#ifndef LV_OBJ_EVENT_H
#define LV_OBJ_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>
#include "../misc/lv_types.h"
#include "../misc/lv_event.h"
#include "../indev/lv_indev.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**
 * Used as the event parameter of ::LV_EVENT_HIT_TEST to check if an `point` can click the object or not.
 * `res` should be set like this:
 *   - If already set to `false` an other event wants that point non clickable. If you want to respect it leave it as `false` or set `true` to overwrite it.
 *   - If already set `true` and `point` shouldn't be clickable set to `false`
 *   - If already set to `true` you agree that `point` can click the object leave it as `true`
 */
typedef struct {
    const lv_point_t * point;   /**< A point relative to screen to check if it can click the object or not*/
    bool res;                   /**< true: `point` can click the object; false: it cannot*/
} lv_hit_test_info_t;

/** Cover check results.*/
typedef enum {
    LV_COVER_RES_COVER      = 0,
    LV_COVER_RES_NOT_COVER  = 1,
    LV_COVER_RES_MASKED     = 2,
} lv_cover_res_t;

/**
 * Used as the event parameter of ::LV_EVENT_COVER_CHECK to check if an area is covered by the object or not.
 * In the event use `const lv_area_t * area = lv_event_get_cover_area(e)` to get the area to check
 * and `lv_event_set_cover_res(e, res)` to set the result.
 */
typedef struct {
    lv_cover_res_t res;
    const lv_area_t * area;
} lv_cover_check_info_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Send an event to the object
 * @param obj           pointer to an object
 * @param event_code    the type of the event from `lv_event_t`
 * @param param         arbitrary data depending on the widget type and the event. (Usually `NULL`)
 * @return LV_RESULT_OK: `obj` was not deleted in the event; LV_RESULT_INVALID: `obj` was deleted in the event_code
 */
// lv_result_t lv_obj_send_event(lv_obj_t * obj, lv_event_code_t event_code, void * param);
mp_obj_t mp_lv_obj_send_event(mp_obj_t obj_in, mp_obj_t event_code_in, mp_obj_t param_in)
{
    lv_obj_t * obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_event_code_t event_code = (lv_event_code_t)mp_obj_get_int(event_code_in);

    lv_result_t ret = lv_obj_send_event(obj, event_code, (void *)param_in);
    return mp_obj_new_int((int)ret);
}

/**
 * Used by the widgets internally to call the ancestor widget types's event handler
 * @param class_p   pointer to the class of the widget (NOT the ancestor class)
 * @param e         pointer to the event descriptor
 * @return          LV_RESULT_OK: the target object was not deleted in the event; LV_RESULT_INVALID: it was deleted in the event_code
 */
// lv_result_t lv_obj_event_base(const lv_obj_class_t * class_p, lv_event_t * e);
mp_obj_t mp_lv_obj_event_base(mp_obj_t class_p_in, mp_obj_t e_in)
{
    lv_obj_class_t * class_p = (lv_obj_class_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(class_p_in))->obj);
    lv_event_t * e = (lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj);

    lv_result_t ret = lv_obj_event_base(class_p, e);
    return mp_obj_new_int((int)ret);
}
/**
 * Get the current target of the event. It's the object which event handler being called.
 * If the event is not bubbled it's the same as "original" target.
 * @param e     pointer to the event descriptor
 * @return      the target of the event_code
 */
// lv_obj_t * lv_event_get_current_target_obj(lv_event_t * e);
mp_obj_t mp_lv_event_get_current_target_obj(mp_obj_t e_in)
{
    lv_obj_t *ret = lv_event_get_current_target_obj((lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj));
    return mp_lv_new_obj(ret);
}

/**
 * Get the object originally targeted by the event. It's the same even if the event is bubbled.
 * @param e     pointer to the event descriptor
 * @return      pointer to the original target of the event_code
 */
// mlv_obj_t * lv_event_get_target_obj(lv_event_t * e);
mp_obj_t mp_lv_event_get_target_obj(mp_obj_t e_in)
{
    lv_obj_t *ret = lv_event_get_target_obj((lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj));
    return mp_lv_new_obj(ret);
}
/**
 * Add an event handler function for an object.
 * Used by the user to react on event which happens with the object.
 * An object can have multiple event handler. They will be called in the same order as they were added.
 * @param obj       pointer to an object
 * @param filter    an event code (e.g. `LV_EVENT_CLICKED`) on which the event should be called. `LV_EVENT_ALL` can be used to receive all the events.
 * @param event_cb  the new event function
 * @param           user_data custom data data will be available in `event_cb`
 * @return          handler to the event. It can be used in `lv_obj_remove_event_dsc`.
 */
// TODO:
lv_event_dsc_t * lv_obj_add_event_cb(lv_obj_t * obj, lv_event_cb_t event_cb, lv_event_code_t filter, void * user_data);

// uint32_t lv_obj_get_event_count(lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_event_count(mp_obj_t obj_in)
{
    uint32_t ret = lv_obj_get_event_count((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int_from_uint(ret);
}

// lv_event_dsc_t * lv_obj_get_event_dsc(lv_obj_t * obj, uint32_t index);
mp_obj_t mp_lv_obj_get_event_dsc(mp_obj_t obj_in, mp_obj_t index_in)
{
    lv_obj_t * obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    uint32_t index = (uint32_t)mp_obj_get_int_truncated(index_in);

    lv_event_dsc_t *ret = lv_obj_get_event_dsc(obj, index);
    return mp_lv_new_event_dsc(ret);
}

// bool lv_obj_remove_event(lv_obj_t * obj, uint32_t index);
mp_obj_t mp_lv_obj_remove_event(mp_obj_t obj_in, mp_obj_t index_in)
{
    lv_obj_t * obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    uint32_t index = (uint32_t)mp_obj_get_int_truncated(index_in);

    bool ret = lv_obj_remove_event(obj, index);
    return mp_obj_new_bool(ret);
}

// TODO:
bool lv_obj_remove_event_cb(lv_obj_t * obj, lv_event_cb_t event_cb);

// bool lv_obj_remove_event_dsc(lv_obj_t * obj, lv_event_dsc_t * dsc);
mp_obj_t mp_lv_obj_remove_event_dsc(mp_obj_t obj_in, mp_obj_t dsc_in)
{
    lv_obj_t * obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_event_dsc_t * dsc = (lv_event_dsc_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(dsc_in))->obj);

    bool ret = lv_obj_remove_event_dsc(obj, dsc);
    return mp_obj_new_bool(ret);
}

/**
 * Remove an event_cb with user_data
 * @param obj           pointer to a obj
 * @param event_cb      the event_cb of the event to remove
 * @param user_data     user_data
 * @return              the count of the event removed
 */
// TODO:
uint32_t lv_obj_remove_event_cb_with_user_data(lv_obj_t * obj, lv_event_cb_t event_cb, void * user_data);

/**
 * Get the input device passed as parameter to indev related events.
 * @param e     pointer to an event
 * @return      the indev that triggered the event or NULL if called on a not indev related event
 */
// lv_indev_t * lv_event_get_indev(lv_event_t * e);
mp_obj_t mp_lv_event_get_indev(mp_obj_t e_in)
{
    lv_indev_t *ret = lv_event_get_indev((lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj));
    return mp_lv_new_indev(ret);
}

/**
 * Get the draw context which should be the first parameter of the draw functions.
 * Namely: `LV_EVENT_DRAW_MAIN/POST`, `LV_EVENT_DRAW_MAIN/POST_BEGIN`, `LV_EVENT_DRAW_MAIN/POST_END`
 * @param e     pointer to an event
 * @return      pointer to a draw context or NULL if called on an unrelated event
 */
// lv_layer_t * lv_event_get_layer(lv_event_t * e);
mp_obj_t mp_lv_event_get_layer(mp_obj_t e_in)
{
    lv_layer_t *ret = lv_event_get_layer((lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj));
    return mp_lv_new_layer(ret);
}

/**
 * Get the old area of the object before its size was changed. Can be used in `LV_EVENT_SIZE_CHANGED`
 * @param e     pointer to an event
 * @return      the old absolute area of the object or NULL if called on an unrelated event
 */
// const lv_area_t * lv_event_get_old_size(lv_event_t * e);
mp_obj_t mp_lv_event_get_old_size(mp_obj_t e_in)
{
    lv_area_t *ret = lv_event_get_old_size((lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj));
    return mp_lv_new_area(ret);
}

/**
 * Get the key passed as parameter to an event. Can be used in `LV_EVENT_KEY`
 * @param e     pointer to an event
 * @return      the triggering key or NULL if called on an unrelated event
 */
// uint32_t lv_event_get_key(lv_event_t * e);
mp_obj_t mp_lv_event_get_key(mp_obj_t e_in)
{
    uint32_t ret = lv_event_get_key((lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj));
    return mp_obj_new_int_from_uint(ret);
}

/**
 * Get the signed rotary encoder diff. passed as parameter to an event. Can be used in `LV_EVENT_ROTARY`
 * @param e     pointer to an event
 * @return      the triggering key or NULL if called on an unrelated event
 */
// int32_t lv_event_get_rotary_diff(lv_event_t * e);
mp_obj_t mp_lv_event_get_rotary_diff(mp_obj_t e_in)
{
    int32_t ret = lv_event_get_rotary_diff((lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj));
    return mp_obj_new_int(ret);
}

/**
 * Get the animation descriptor of a scrolling. Can be used in `LV_EVENT_SCROLL_BEGIN`
 * @param e     pointer to an event
 * @return      the animation that will scroll the object. (can be modified as required)
 */
// lv_anim_t * lv_event_get_scroll_anim(lv_event_t * e);
mp_obj_t mp_lv_event_get_scroll_anim(mp_obj_t e_in)
{
    lv_anim_t *ret = lv_event_get_scroll_anim((lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj));
    return mp_lv_new_anim(ret);
}
/**
 * Set the new extra draw size. Can be used in `LV_EVENT_REFR_EXT_DRAW_SIZE`
 * @param e     pointer to an event
 * @param size  The new extra draw size
 */
// void lv_event_set_ext_draw_size(lv_event_t * e, int32_t size);
mp_obj_t mp_lv_event_set_ext_draw_size(mp_obj_t e_in, mp_obj_t size_in)
{
    lv_event_t * e = (lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj)
    int32_t size = (int32_t)mp_obj_get_int(size_in);
    lv_event_set_ext_draw_size(e, size);
    return mp_const_none;
}
/**
 * Get a pointer to an `lv_point_t` variable in which the self size should be saved (width in `point->x` and height `point->y`).
 * Can be used in `LV_EVENT_GET_SELF_SIZE`
 * @param e     pointer to an event
 * @return      pointer to `lv_point_t` or NULL if called on an unrelated event
 */
// lv_point_t * lv_event_get_self_size_info(lv_event_t * e);
mp_obj_t mp_lv_event_get_self_size_info(mp_obj_t e_in)
{
    lv_point_t *ret = lv_event_get_self_size_info((lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj));
    return mp_lv_new_point(ret);
}
/**
 * Get a pointer to an `lv_hit_test_info_t` variable in which the hit test result should be saved. Can be used in `LV_EVENT_HIT_TEST`
 * @param e     pointer to an event
 * @return      pointer to `lv_hit_test_info_t` or NULL if called on an unrelated event
 */
// lv_hit_test_info_t * lv_event_get_hit_test_info(lv_event_t * e);
mp_obj_t mp_lv_event_get_hit_test_info(mp_obj_t e_in)
{
    lv_hit_test_info_t *ret = lv_event_get_hit_test_info((lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj));
    return mp_lv_new_hit_test_info(ret);
}

/**
 * Get a pointer to an area which should be examined whether the object fully covers it or not.
 * Can be used in `LV_EVENT_HIT_TEST`
 * @param e     pointer to an event
 * @return      an area with absolute coordinates to check
 */
// const lv_area_t * lv_event_get_cover_area(lv_event_t * e);
mp_obj_t mp_lv_event_get_cover_area(mp_obj_t e_in)
{
    lv_area_t *ret = lv_event_get_cover_area((lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj));
    return mp_lv_new_area(ret);
}

/**
 * Set the result of cover checking. Can be used in `LV_EVENT_COVER_CHECK`
 * @param e     pointer to an event
 * @param res   an element of ::lv_cover_check_info_t
 */
// void lv_event_set_cover_res(lv_event_t * e, lv_cover_res_t res);
mp_obj_t mp_lv_event_set_cover_res(mp_obj_t e_in, mp_obj_t res_in)
{
    lv_event_t * e = (lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj);
    lv_cover_res_t res = (lv_cover_res_t)mp_obj_get_int(res_in);
    lv_event_set_cover_res(e, res);
    return mp_const_none;
}

/**
 * Get the draw task which was just added.
 * Can be used in `LV_EVENT_DRAW_TASK_ADDED event`
 * @param e     pointer to an event
 * @return      the added draw task
 */
// lv_draw_task_t * lv_event_get_draw_task(lv_event_t * e);
mp_obj_t mp_lv_event_get_draw_task(mp_obj_t e_in)
{
    lv_draw_task_t *ret = lv_event_get_draw_task((lv_event_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(e_in))->obj));
    return mp_lv_new_draw_task(ret);
}

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_OBJ_EVENT_H*/
