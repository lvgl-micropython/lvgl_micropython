/**
 * @file struct _lv_obj_tree.h
 *
 */

#ifndef LV_OBJ_TREE_H
#define LV_OBJ_TREE_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stddef.h>
#include <stdbool.h>
#include "../misc/lv_types.h"
#include "../misc/lv_anim.h"
#include "../display/lv_display.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    LV_OBJ_TREE_WALK_NEXT,
    LV_OBJ_TREE_WALK_SKIP_CHILDREN,
    LV_OBJ_TREE_WALK_END,
} lv_obj_tree_walk_res_t;

typedef lv_obj_tree_walk_res_t (*lv_obj_tree_walk_cb_t)(lv_obj_t *, void *);

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Delete an object and all of its children.
 * Also remove the objects from their group and remove all animations (if any).
 * Send `LV_EVENT_DELETED` to deleted objects.
 * @param obj       pointer to an object
 */
// void lv_obj_delete(lv_obj_t * obj);
mp_obj_t mp_lv_obj_delete(mp_obj_t obj_in)
{
    lv_obj_delete((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_const_none;
}

/**
 * Delete all children of an object.
 * Also remove the objects from their group and remove all animations (if any).
 * Send `LV_EVENT_DELETED` to deleted objects.
 * @param obj       pointer to an object
 */
// void lv_obj_clean(lv_obj_t * obj);
mp_obj_t mp_lv_obj_clean(mp_obj_t obj_in)
{
    lv_obj_clean((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_const_none;
}

/**
 * Delete an object after some delay
 * @param obj       pointer to an object
 * @param delay_ms  time to wait before delete in milliseconds
 */
// void lv_obj_delete_delayed(lv_obj_t * obj, uint32_t delay_ms);
mp_obj_t mp_lv_obj_delete_delayed(mp_obj_t obj_in, mp_obj_t delay_ms_in)
{
    lv_obj_t *obj =  (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    uint32_t delay_ms = (uint32_t)mp_obj_get_int_truncated(delay_ms_in);
    lv_obj_delete_delayed(obj, delay_ms);
    return mp_const_none;
}

/**
 * A function to be easily used in animation ready callback to delete an object when the animation is ready
 * @param a         pointer to the animation
 */
// void lv_obj_delete_anim_completed_cb(lv_anim_t * a);
mp_obj_t mp_lv_obj_delete_anim_completed_cb(mp_obj_t a_in)
{
    lv_obj_delete_anim_completed_cb((lv_anim_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(a_in))->obj));
    return mp_const_none;
}
/**
 * Helper function for asynchronously deleting objects.
 * Useful for cases where you can't delete an object directly in an `LV_EVENT_DELETE` handler (i.e. parent).
 * @param obj       object to delete
 * @see lv_async_call
 */
// void lv_obj_delete_async(lv_obj_t * obj);
mp_obj_t mp_lv_obj_delete_async(mp_obj_t obj_in)
{
    lv_obj_delete_async((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_const_none;
}

/**
 * Move the parent of an object. The relative coordinates will be kept.
 *
 * @param obj       pointer to an object whose parent needs to be changed
 * @param parent pointer to the new parent
 */
// void lv_obj_set_parent(lv_obj_t * obj, lv_obj_t * parent);
mp_obj_t mp_lv_obj_set_parent(mp_obj_t obj_in, mp_obj_t parent_in)
{
    lv_obj_t *obj = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_obj_t *parent = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(parent_in))->obj);

    lv_obj_set_parent(obj, parent);
    return mp_const_none;
}
/**
 * Swap the positions of two objects.
 * When used in listboxes, it can be used to sort the listbox items.
 * @param obj1  pointer to the first object
 * @param obj2  pointer to the second object
 */
// void lv_obj_swap(lv_obj_t * obj1, lv_obj_t * obj2);
mp_obj_t mp_lv_obj_swap(mp_obj_t obj1_in, mp_obj_t obj2_in)
{
    lv_obj_t *obj1 = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj1_in))->obj);
    lv_obj_t *obj2 = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj2_in))->obj);

    lv_obj_swap(obj1, obj2);
    return mp_const_none;
}
/**
 * moves the object to the given index in its parent.
 * When used in listboxes, it can be used to sort the listbox items.
 * @param obj  pointer to the object to be moved.
 * @param index  new index in parent. -1 to count from the back
 * @note to move to the background: lv_obj_move_to_index(obj, 0)
 * @note to move forward (up): lv_obj_move_to_index(obj, lv_obj_get_index(obj) - 1)
 */
// void lv_obj_move_to_index(lv_obj_t * obj, int32_t index);
mp_obj_t mp_lv_obj_move_to_index(mp_obj_t obj_in, mp_obj_t index_in)
{
    lv_obj_t *obj =  (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t index = (int32_t)mp_obj_get_int(index_in);
    lv_obj_move_to_index(obj, index);
    return mp_const_none;
}
/**
 * Get the screen of an object
 * @param obj       pointer to an object
 * @return          pointer to the object's screen
 */
// lv_obj_t * lv_obj_get_screen(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_screen(mp_obj_t obj_in)
{
    lv_obj_t * ret = lv_obj_get_screen((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_lv_new_obj(ret);
}

/**
 * Get the display of the object
 * @param obj       pointer to an object
 * @return          pointer to the object's display
 */
// lv_display_t * lv_obj_get_display(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_display(mp_obj_t obj_in)
{
    lv_display_t * ret = lv_obj_get_display((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_lv_new_display(ret);
}
/**
 * Get the parent of an object
 * @param obj       pointer to an object
 * @return          the parent of the object. (NULL if `obj` was a screen)
 */
// lv_obj_t * lv_obj_get_parent(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_parent(mp_obj_t obj_in)
{
    lv_obj_t * ret = lv_obj_get_parent((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_lv_new_obj(ret);
}
/**
 * Get the child of an object by the child's index.
 * @param obj       pointer to an object whose child should be get
 * @param idx       the index of the child.
 *                  0: the oldest (firstly created) child
 *                  1: the second oldest
 *                  child count-1: the youngest
 *                  -1: the youngest
 *                  -2: the second youngest
 * @return          pointer to the child or NULL if the index was invalid
 */
// lv_obj_t * lv_obj_get_child(const lv_obj_t * obj, int32_t idx);
mp_obj_t mp_lv_obj_get_child(mp_obj_t obj_in, mp_obj_t idx_in)
{
    lv_obj_t *obj =  (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t idx = (int32_t)mp_obj_get_int(idx_in);

    lv_obj_t * ret = lv_obj_get_child(obj, idx);
    return mp_lv_new_obj(ret);
}

/**
 * Get the child of an object by the child's index. Consider the children only with a given type.
 * @param obj       pointer to an object whose child should be get
 * @param idx       the index of the child.
 *                  0: the oldest (firstly created) child
 *                  1: the second oldest
 *                  child count-1: the youngest
 *                  -1: the youngest
 *                  -2: the second youngest
 * @param class_p   the type of the children to check
 * @return          pointer to the child or NULL if the index was invalid
 */
// lv_obj_t * lv_obj_get_child_by_type(const lv_obj_t * obj, int32_t idx,
//                                     const lv_obj_class_t * class_p);

mp_obj_t mp_lv_obj_get_child_by_type(mp_obj_t obj_in, mp_obj_t idx_in, mp_obj_t class_p_in)
{
    lv_obj_t *obj =  (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t idx = (int32_t)mp_obj_get_int(idx_in);
    lv_obj_class_t *class_p =  (lv_obj_class_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(class_p_in))->obj);

    lv_obj_t *ret = lv_obj_get_child_by_type(obj, idx, class_p);
    return mp_lv_new_obj(ret);
}

/**
 * Return a sibling of an object
 * @param obj       pointer to an object whose sibling should be get
 * @param idx       0: `obj` itself
 *                  -1: the first older sibling
 *                  -2: the next older sibling
 *                  1: the first younger sibling
 *                  2: the next younger sibling
 *                  etc
 * @return          pointer to the requested sibling  or NULL if there is no such sibling
 */
// lv_obj_t * lv_obj_get_sibling(const lv_obj_t * obj, int32_t idx);
mp_obj_t mp_lv_obj_get_sibling(mp_obj_t obj_in, mp_obj_t idx_in)
{
    lv_obj_t *obj =  (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t idx = (int32_t)mp_obj_get_int(idx_in);

    lv_obj_t *ret = lv_obj_get_sibling(obj, idx);
    return mp_lv_new_obj(ret);
}
/**
 * Return a sibling of an object. Consider the siblings only with a given type.
 * @param obj       pointer to an object whose sibling should be get
 * @param idx       0: `obj` itself
 *                  -1: the first older sibling
 *                  -2: the next older sibling
 *                  1: the first younger sibling
 *                  2: the next younger sibling
 *                  etc
 * @param class_p   the type of the children to check
 * @return          pointer to the requested sibling  or NULL if there is no such sibling
 */
// lv_obj_t * lv_obj_get_sibling_by_type(const lv_obj_t * obj, int32_t idx,
//                                       const lv_obj_class_t * class_p);

mp_obj_t mp_lv_obj_get_sibling_by_type(mp_obj_t obj_in, mp_obj_t idx_in, mp_obj_t class_p_in)
{
    lv_obj_t *obj =  (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    int32_t idx = (int32_t)mp_obj_get_int(idx_in);
    lv_obj_class_t *class_p =  (lv_obj_class_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(class_p_in))->obj);

    lv_obj_t *ret = lv_obj_get_sibling_by_type(obj, idx, class_p);
    return mp_lv_new_obj(ret);
}


/**
 * Get the number of children
 * @param obj       pointer to an object
 * @return          the number of children
 */
// uint32_t lv_obj_get_child_count(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_child_count(mp_obj_t obj_in)
{
    uint32_t ret = lv_obj_get_child_count((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int_from_uint(ret);
}
/**
 * Get the number of children having a given type.
 * @param obj       pointer to an object
 * @param class_p   the type of the children to check
 * @return          the number of children
 */

// uint32_t lv_obj_get_child_count_by_type(const lv_obj_t * obj, const lv_obj_class_t * class_p);
mp_obj_t mp_lv_obj_get_child_count_by_type(mp_obj_t obj_in, mp_obj_t class_p_in)
{
    lv_obj_t *obj =  (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_obj_class_t *class_p =  (lv_obj_class_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(class_p_in))->obj);
    uint32_t ret = lv_obj_get_child_count_by_type(obj, class_p);
    return mp_obj_new_int_from_uint(ret);
}
/**
 * Get the index of a child.
 * @param obj       pointer to an object
 * @return          the child index of the object.
 *                  E.g. 0: the oldest (firstly created child).
 *                  (-1 if child could not be found or no parent exists)
 */
// int32_t lv_obj_get_index(const lv_obj_t * obj);
mp_obj_t mp_lv_obj_get_index(mp_obj_t obj_in)
{
    int32_t ret = lv_obj_get_index((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_int(ret);
}
/**
 * Get the index of a child. Consider the children only with a given type.
 * @param obj       pointer to an object
 * @param class_p   the type of the children to check
 * @return          the child index of the object.
 *                  E.g. 0: the oldest (firstly created child with the given class).
 *                  (-1 if child could not be found or no parent exists)
 */
// int32_t lv_obj_get_index_by_type(const lv_obj_t * obj, const lv_obj_class_t * class_p);
mp_obj_t mp_lv_obj_get_index_by_type(mp_obj_t obj_in, mp_obj_t class_p_in)
{
    lv_obj_t *obj =  (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj);
    lv_obj_class_t *class_p =  (lv_obj_class_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(class_p_in))->obj);
    int32_t ret = lv_obj_get_index_by_type(obj, class_p);
    return mp_obj_new_int(ret);
}
/**
 * Iterate through all children of any object.
 * @param start_obj     start integrating from this object
 * @param cb            call this callback on the objects
 * @param user_data     pointer to any user related data (will be passed to `cb`)
 */

// TODO:
void lv_obj_tree_walk(lv_obj_t * start_obj, lv_obj_tree_walk_cb_t cb, void * user_data);

/**
 * Iterate through all children of any object and print their ID.
 * @param start_obj     start integrating from this object
 */
// void lv_obj_dump_tree(lv_obj_t * start_ob);
mp_obj_t mp_lv_obj_dump_tree(mp_obj_t start_ob_in)
{
    lv_obj_dump_tree((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(start_ob_in))->obj));
    return mp_const_none;
}
/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_OBJ_TREE_H*/
