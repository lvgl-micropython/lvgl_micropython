/**
 * @file lv_obj_class.h
 *
 */

#ifndef LV_OBJ_CLASS_H
#define LV_OBJ_CLASS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>
#include <stdbool.h>
#include "../misc/lv_types.h"
#include "../misc/lv_area.h"
#include "lv_obj_property.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    LV_OBJ_CLASS_EDITABLE_INHERIT,      /**< Check the base class. Must have 0 value to let zero initialized class inherit*/
    LV_OBJ_CLASS_EDITABLE_TRUE,
    LV_OBJ_CLASS_EDITABLE_FALSE,
} lv_obj_class_editable_t;

typedef enum {
    LV_OBJ_CLASS_GROUP_DEF_INHERIT,      /**< Check the base class. Must have 0 value to let zero initialized class inherit*/
    LV_OBJ_CLASS_GROUP_DEF_TRUE,
    LV_OBJ_CLASS_GROUP_DEF_FALSE,
} lv_obj_class_group_def_t;

typedef enum {
    LV_OBJ_CLASS_THEME_INHERITABLE_FALSE,    /**< Do not inherit theme from base class. */
    LV_OBJ_CLASS_THEME_INHERITABLE_TRUE,
} lv_obj_class_theme_inheritable_t;

typedef void (*lv_obj_class_event_cb_t)(lv_obj_class_t * class_p, lv_event_t * e);
/**
 * Describe the common methods of every object.
 * Similar to a C++ class.
 */
struct _lv_obj_class_t {
    const lv_obj_class_t * base_class;
    /*class_p is the final class while obj->class_p is the class currently being [de]constructed.*/
    void (*constructor_cb)(const lv_obj_class_t * class_p, lv_obj_t * obj);
    void (*destructor_cb)(const lv_obj_class_t * class_p, lv_obj_t * obj);

    /*class_p is the class in which event is being processed.*/
    void (*event_cb)(const lv_obj_class_t * class_p, lv_event_t * e);  /**< Widget type specific event function*/

#if LV_USE_OBJ_PROPERTY
    uint32_t prop_index_start;
    uint32_t prop_index_end;
    const lv_property_ops_t * properties;
    uint32_t properties_count;
#endif

    void * user_data;
    const char * name;
    int32_t width_def;
    int32_t height_def;
    uint32_t editable : 2;             /**< Value from ::lv_obj_class_editable_t*/
    uint32_t group_def : 2;            /**< Value from ::lv_obj_class_group_def_t*/
    uint32_t instance_size : 16;
    uint32_t theme_inheritable : 1;    /**< Value from ::lv_obj_class_theme_inheritable_t*/
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an object form a class descriptor
 * @param class_p   pointer to a class
 * @param parent    pointer to an object where the new object should be created
 * @return          pointer to the created object
 */
// lv_obj_t * lv_obj_class_create_obj(const lv_obj_class_t * class_p, lv_obj_t * parent);
mp_obj_t mp_lv_obj_class_create_obj(mp_obj_t class_p_in, mp_obj_t parent_in)
{
    lv_obj_class_t *class_p = (lv_obj_class_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(class_p_in))->obj);
    lv_obj_t *parent = (lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(parent_in))->obj);

    lv_obj_t *ret = lv_obj_class_create_obj(class_p, parent);
    return mp_lv_new_obj(ret);
}


// void lv_obj_class_init_obj(lv_obj_t * obj);
mp_obj_t mp_lv_obj_class_init_obj(mp_obj_t obj_in)
{
    lv_obj_class_init_obj((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_const_none;
}

// void _lv_obj_destruct(lv_obj_t * obj);

// bool lv_obj_is_editable(lv_obj_t * obj);
mp_obj_t mp_lv_obj_is_editable(mp_obj_t obj_in)
{
    bool ret = lv_obj_is_editable((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_bool(ret);
}

// bool lv_obj_is_group_def(lv_obj_t * obj);
mp_obj_t mp_lv_obj_is_group_def(mp_obj_t obj_in)
{
    bool ret = lv_obj_is_group_def((lv_obj_t *)((mp_lv_struct_t *)(MP_OBJ_TO_PTR(obj_in))->obj));
    return mp_obj_new_bool(ret);
}

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_OBJ_CLASS_H*/
