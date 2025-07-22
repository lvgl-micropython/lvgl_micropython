// Auto-Generated file, DO NOT EDIT!
//
// Command line:
// {cmd_line}
//
// Preprocessing command:
// {pp_cmd}
//
// Generating Objects: {objs}

//stdlib includes
#include <stdlib.h>
#include <string.h>

// MicroPython includes
#include "py/obj.h"
#include "py/objint.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/objarray.h"
#include "py/objtype.h"
#include "py/objexcept.h"

// LVGL includes
{lv_headers}


#define LV_OBJ_T lv_obj_t

typedef struct mp_lv_obj_type_t {
    const lv_obj_class_t *lv_obj_class;
    const mp_obj_type_t *mp_obj_type;
} mp_lv_obj_type_t;


static const mp_lv_obj_type_t mp_lv_obj_t_type;
static const mp_lv_obj_type_t *mp_lv_obj_types[];


static const mp_obj_type_t *get_BaseObj_type()
{
    return mp_lv_obj_t_type.mp_obj_type;
}


MP_DEFINE_EXCEPTION(LvError, Exception)
MP_DEFINE_EXCEPTION(LvReferenceError, LvError)
MP_DEFINE_EXCEPTION(LvPointerError, LvError)

#define _RAISE_ERROR_MSG(msg)  msg

#define RAISE_ValueError(msg, ...)           mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_TypeError(msg, ...)            mp_raise_msg_varg(&mp_type_TypeError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_OSError(msg, ...)              mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_SyntaxError(msg, ...)          mp_raise_msg_varg(&mp_type_SyntaxError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_Exception(msg, ...)            mp_raise_msg_varg(&mp_type_Exception, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_IndexError(msg, ...)           mp_raise_msg_varg(&mp_type_IndexError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_KeyError(msg, ...)             mp_raise_msg_varg(&mp_type_KeyError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_NotImplementedError(msg, ...)  mp_raise_msg_varg(&mp_type_NotImplementedError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_RuntimeError(msg, ...)         mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_AttributeError(msg, ...)       mp_raise_msg_varg(&mp_type_AttributeError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)

#define RAISE_LvError(msg, ...)              mp_raise_msg_varg(&mp_type_LvError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_LvReferenceError(msg, ...)     mp_raise_msg_varg(&mp_type_LvReferenceError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)
#define RAISE_LvPointerError(msg, ...)       mp_raise_msg_varg(&mp_type_LvPointerError, MP_ERROR_TEXT(_RAISE_ERROR_MSG(msg)), __VA_ARGS__)


// Helper functions
#ifndef GENMPY_UNUSED
#ifdef __GNUC__
#define GENMPY_UNUSED __attribute__ ((unused))
#else
#define GENMPY_UNUSED
#endif // __GNUC__
#endif // GENMPY_UNUSED


// Custom function mp object
typedef mp_obj_t (*mp_fun_ptr_t)(size_t n, const mp_obj_t *, void *ptr);


typedef struct mp_lv_obj_fun_t {
    mp_obj_base_t base;
    mp_uint_t n_args;
    mp_fun_ptr_t mp_fun;
    void *lv_fun;
} mp_lv_obj_fun_t;


static mp_obj_t lv_fun_call(mp_obj_t self_in, size_t n_args,
                            size_t n_kw, const mp_obj_t *args);

static mp_int_t mp_func_get_buffer(mp_obj_t self_in,
                                   mp_buffer_info_t *bufinfo, mp_uint_t flags);


GENMPY_UNUSED static MP_DEFINE_CONST_OBJ_TYPE(
    mp_lv_type_fun,
    MP_QSTR_function,
    MP_TYPE_FLAG_NONE,
    call, lv_fun_call,
    buffer, mp_func_get_buffer
);


static mp_obj_t lv_fun_call(mp_obj_t self_in, size_t n_args,
                            size_t n_kw, const mp_obj_t *args)
{
    mp_lv_obj_fun_t *self = MP_OBJ_TO_PTR(self_in);
    mp_arg_check_num(n_args, n_kw, self->n_args, self->n_args, false);

    return self->mp_fun(n_args, args, self->lv_fun);
}


static mp_int_t mp_func_get_buffer(mp_obj_t self_in,
                                   mp_buffer_info_t *bufinfo, mp_uint_t flags)
{
    (void)flags;

    mp_lv_obj_fun_t *self = MP_OBJ_TO_PTR(self_in);

    bufinfo->buf = &self->lv_fun;
    bufinfo->len = sizeof(self->lv_fun);
    bufinfo->typecode = BYTEARRAY_TYPECODE;

    return 0;
}


#define MP_DEFINE_CONST_C_FUN_OBJ(obj_name, n_args, mp_fun, lv_fun) \
const mp_lv_obj_fun_t obj_name = { { &mp_lv_type_fun }, n_args, mp_fun, lv_fun }



// Casting
typedef struct mp_lv_struct_t
{
    mp_obj_base_t base;
    void *data;
} mp_lv_struct_t;


static const mp_lv_struct_t mp_lv_null_obj;


#ifdef LV_OBJ_T
static mp_int_t mp_lv_obj_get_buffer(mp_obj_t self_in,
                                     mp_buffer_info_t *bufinfo, mp_uint_t flags);
#else
static mp_int_t mp_lv_obj_get_buffer(mp_obj_t self_in,
                                     mp_buffer_info_t *bufinfo, mp_uint_t flags)
{
    return 0;
}
#endif


static mp_int_t mp_blob_get_buffer(mp_obj_t self_in,
                                   mp_buffer_info_t *bufinfo, mp_uint_t flags);


static mp_obj_t get_native_obj(mp_obj_t mp_obj)
{
    if (!MP_OBJ_IS_OBJ(mp_obj)) return mp_obj;

    const mp_obj_type_t *native_type = ((mp_obj_base_t*)mp_obj)->type;

    if (native_type == NULL) return NULL;

    if (MP_OBJ_TYPE_GET_SLOT_OR_NULL(native_type, parent) == NULL ||
        (MP_OBJ_TYPE_GET_SLOT_OR_NULL(native_type, buffer) == mp_blob_get_buffer) ||
        (MP_OBJ_TYPE_GET_SLOT_OR_NULL(native_type, buffer) == mp_lv_obj_get_buffer)) {
        return mp_obj;
    }

    while (MP_OBJ_TYPE_GET_SLOT_OR_NULL(native_type, parent)) {
        native_type = MP_OBJ_TYPE_GET_SLOT(native_type, parent);
    }

    return mp_obj_cast_to_native_base(mp_obj, MP_OBJ_FROM_PTR(native_type));
}


static mp_obj_t dict_to_struct(mp_obj_t dict, const mp_obj_type_t *type);


static mp_obj_t make_new_lv_struct(const mp_obj_type_t *type, size_t n_args,
                                   size_t n_kw, const mp_obj_t *args);


static mp_obj_t cast(mp_obj_t mp_obj, const mp_obj_type_t *mp_type)
{
    mp_obj_t res = NULL;

    mp_make_new_fun_t make_new = MP_OBJ_TYPE_GET_SLOT_OR_NULL(mp_type, make_new);

    if (mp_obj == mp_const_none && make_new == &make_new_lv_struct) {
        res = MP_OBJ_FROM_PTR(&mp_lv_null_obj);
    } else if (MP_OBJ_IS_OBJ(mp_obj)) {
        res = get_native_obj(mp_obj);

        if (res) {
            const mp_obj_type_t *res_type = ((mp_obj_base_t*)res)->type;

            if (res_type != mp_type) {
                make_new = MP_OBJ_TYPE_GET_SLOT_OR_NULL(mp_type, make_new);
                if (res_type == &mp_type_dict && make_new == &make_new_lv_struct) {
                    res = dict_to_struct(res, mp_type);
                } else {
                    res = NULL;
                }
            }
        }
    }

    if (res == NULL) {
        RAISE_ValueError("Can't convert %s to %s!",
                         mp_obj_get_type_str(mp_obj), qstr_str(mp_type->name)));
    }
    return res;
}

// object handling
// This section is enabled only when objects are supported

#ifdef LV_OBJ_T

typedef struct mp_lv_obj_t {
    mp_obj_base_t base;
    LV_OBJ_T *lv_obj;
    LV_OBJ_T *callbacks;
} mp_lv_obj_t;


static inline LV_OBJ_T *mp_to_lv(mp_obj_t mp_obj)
{
    if (mp_obj == NULL || mp_obj == mp_const_none) return NULL;

    mp_obj_t native_obj = get_native_obj(mp_obj);

    mp_buffer_fun_t buf = MP_OBJ_TYPE_GET_SLOT_OR_NULL(
                                           mp_obj_get_type(native_obj), buffer);

    if (buf != mp_lv_obj_get_buffer) return NULL;

    mp_lv_obj_t *mp_lv_obj = MP_OBJ_TO_PTR(native_obj);
    if (mp_lv_obj->lv_obj == NULL) {
        RAISE_LvReferenceError("Referenced object was deleted!");
    }

    return mp_lv_obj->lv_obj;
}


static LV_OBJ_T *mp_get_callbacks(mp_obj_t mp_obj)
{
    if (mp_obj == NULL || mp_obj == mp_const_none) return NULL;

    mp_lv_obj_t *mp_lv_obj = MP_OBJ_TO_PTR(get_native_obj(mp_obj));
    if (mp_lv_obj == NULL) {
        RAISE_TypeError("'user_data' argument must be either a dict or None!");
    }

    if (!mp_lv_obj->callbacks) mp_lv_obj->callbacks = mp_obj_new_dict(0);

    return mp_lv_obj->callbacks;
}


static const mp_obj_type_t *get_BaseObj_type();


static void mp_lv_delete_cb(lv_event_t * e)
{
    LV_OBJ_T *lv_obj = e->current_target;

    if (lv_obj) {
        mp_lv_obj_t *self = lv_obj->user_data;

        if (self) self->lv_obj = NULL;
    }
}


static mp_obj_t lv_to_mp(LV_OBJ_T *lv_obj)
{
    if (lv_obj == NULL) return mp_const_none;

    mp_lv_obj_t *self = (mp_lv_obj_t*)lv_obj->user_data;
    if (!self) {
        // Find the object type
        const mp_obj_type_t *mp_obj_type = get_BaseObj_type();
        const lv_obj_class_t *lv_obj_class = lv_obj_get_class(lv_obj);
        const mp_lv_obj_type_t **iter = &mp_lv_obj_types[0];

        for (; *iter; iter++) {
            if ((*iter)->lv_obj_class == lv_obj_class) {
                mp_obj_type = (*iter)->mp_obj_type;
                break;
            }
        }

        // Create the MP object
        self = m_new_obj(mp_lv_obj_t);

        *self = (mp_lv_obj_t){
            .base = { (const mp_obj_type_t *)mp_obj_type },
            .lv_obj = lv_obj,
            .callbacks = NULL,
        };

        // Register the Python object in user_data
        lv_obj->user_data = self;

        // Register a "Delete" event callback
        lv_obj_add_event_cb(lv_obj, mp_lv_delete_cb, LV_EVENT_DELETE, NULL);
    }

    return MP_OBJ_FROM_PTR(self);
}


static void* mp_to_ptr(mp_obj_t self_in);


static mp_obj_t cast_obj_type(const mp_obj_type_t* type, mp_obj_t obj)
{
    mp_lv_obj_t *self = m_new_obj(mp_lv_obj_t);

    *self = (mp_lv_obj_t){
        .base = { type },
        .lv_obj = mp_to_ptr(obj),
        .callbacks = NULL,
    };

    if (!self->lv_obj) return mp_const_none;

    return MP_OBJ_FROM_PTR(self);
}


static mp_obj_t cast_obj(mp_obj_t type_obj, mp_obj_t obj)
{
    return cast_obj_type((const mp_obj_type_t *)type_obj, obj);
}


static mp_obj_t make_new(const mp_lv_obj_fun_t *lv_obj_var, const mp_obj_type_t *type,
                         size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_obj_t lv_obj;

    if (n_args == 0 && n_kw == 0) { // allow no args, and pass NULL as parent in such case
        const mp_obj_t no_args[] = {mp_const_none};
        lv_obj = mp_call_function_n_kw(MP_OBJ_FROM_PTR(lv_obj_var), 1, 0, no_args);
    } else {
        lv_obj = mp_call_function_n_kw(MP_OBJ_FROM_PTR(lv_obj_var), n_args, n_kw, args);
    }

    if (!lv_obj) return mp_const_none;

    mp_lv_obj_t *self = MP_OBJ_TO_PTR(lv_obj);

    if (self->base.type != type) return cast_obj_type(type, lv_obj);

    return lv_obj;
}

static MP_DEFINE_CONST_FUN_OBJ_2(cast_obj_obj, cast_obj);
static MP_DEFINE_CONST_CLASSMETHOD_OBJ(cast_obj_class_method, MP_ROM_PTR(&cast_obj_obj));


static mp_int_t mp_lv_obj_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags)
{
    (void)flags;
    mp_lv_obj_t *self = MP_OBJ_TO_PTR(self_in);

    bufinfo->buf = &self->lv_obj;
    bufinfo->len = sizeof(self->lv_obj);
    bufinfo->typecode = BYTEARRAY_TYPECODE;

    return 0;
}


static mp_obj_t mp_lv_obj_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in)
{
    mp_lv_obj_t *lhs = MP_OBJ_TO_PTR(lhs_in);
    mp_lv_obj_t *rhs = MP_OBJ_TO_PTR(rhs_in);

    switch (op) {
        case MP_BINARY_OP_EQUAL:
            return mp_obj_new_bool(lhs->lv_obj == rhs->lv_obj);

        case MP_BINARY_OP_NOT_EQUAL:
            return mp_obj_new_bool(lhs->lv_obj != rhs->lv_obj);

        default:
            return MP_OBJ_NULL;
    }
}

// Register LVGL root pointers
MP_REGISTER_ROOT_POINTER(void *mp_lv_roots);
MP_REGISTER_ROOT_POINTER(void *mp_lv_user_data);

void *mp_lv_roots;


void mp_lv_init_gc()
{
    static bool mp_lv_roots_initialized = false;

    if (!mp_lv_roots_initialized) {
        mp_lv_roots = MP_STATE_VM(mp_lv_roots) = m_new0(lv_global_t, 1);
        mp_lv_roots_initialized = true;
    }
}

#else // LV_OBJ_T

typedef struct mp_lv_obj_type_t {
    mp_obj_type_t *mp_obj_type;
} mp_lv_obj_type_t;

#endif


static mp_obj_t convert_to_bool(bool b)
{
    return b ? mp_const_true : mp_const_false;
}


static mp_obj_t convert_to_str(const char *str)
{
    return str ? mp_obj_new_str(str, strlen(str)) : mp_const_none;
}


static const char *convert_from_str(mp_obj_t str)
{
    if (str == NULL || str == mp_const_none) return NULL;

    if (MP_OBJ_IS_TYPE(str, &mp_type_bytearray) ||
        MP_OBJ_IS_TYPE(str, &mp_type_memoryview)) {
            mp_buffer_info_t buffer_info;

            if (mp_get_buffer(str, &buffer_info, MP_BUFFER_READ)) {
                return buffer_info.buf;
            }
    }

    return mp_obj_str_get_str(str);
}


// struct handling
static mp_lv_struct_t *mp_to_lv_struct(mp_obj_t mp_obj)
{
    if (mp_obj == NULL || mp_obj == mp_const_none) return NULL;

    mp_obj_t native_obj = get_native_obj(mp_obj);

    mp_make_new_fun_t make_new = MP_OBJ_TYPE_GET_SLOT_OR_NULL(
                                        mp_obj_get_type(native_obj), make_new);

    if (!MP_OBJ_IS_OBJ(native_obj) || make_new != &make_new_lv_struct) {
        RAISE_TypeError("Expected Struct object!");
    }

    mp_lv_struct_t *mp_lv_struct = MP_OBJ_TO_PTR(native_obj);
    return mp_lv_struct;
}


static inline size_t get_lv_struct_size(const mp_obj_type_t *type)
{
    mp_obj_dict_t *self = MP_OBJ_TO_PTR(MP_OBJ_TYPE_GET_SLOT(type, locals_dict));
    mp_map_elem_t *elem = mp_map_lookup(&self->map,
                                        MP_OBJ_NEW_QSTR(MP_QSTR___SIZE__),
                                        MP_MAP_LOOKUP);

    if (elem == NULL) return 0;
    else return (size_t)mp_obj_get_int(elem->value);
}


static mp_obj_t make_new_lv_struct(const mp_obj_type_t *type, size_t n_args,
                                   size_t n_kw, const mp_obj_t *args)
{

    mp_make_new_fun_t make_new = MP_OBJ_TYPE_GET_SLOT_OR_NULL(type, make_new);

    if ((!MP_OBJ_IS_TYPE(type, &mp_type_type)) || make_new != &make_new_lv_struct) {
        RAISE_TypeError("Argument is not a struct type!");
    }

    size_t size = get_lv_struct_size(type);
    mp_arg_check_num(n_args, n_kw, 0, 1, false);

    mp_lv_struct_t *self = m_new_obj(mp_lv_struct_t);
    mp_lv_struct_t *other = (n_args > 0) && (!mp_obj_is_int(args[0])) ? mp_to_lv_struct(cast(args[0], type)) : NULL;
    size_t count = (n_args > 0) && (mp_obj_is_int(args[0]))? mp_obj_get_int(args[0]): 1;

    *self = (mp_lv_struct_t){
        .base = { type },
        .data = (size == 0 || (other && other->data == NULL)) ? NULL : m_malloc(size * count)
    };

    if (self->data) {
        if (other) memcpy(self->data, other->data, size * count);
        else memset(self->data, 0, size * count);
    }

    return MP_OBJ_FROM_PTR(self);
}


static mp_obj_t lv_struct_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in)
{
    mp_lv_struct_t *lhs = MP_OBJ_TO_PTR(lhs_in);
    mp_lv_struct_t *rhs = MP_OBJ_TO_PTR(rhs_in);

    switch (op) {
        case MP_BINARY_OP_EQUAL:
            return mp_obj_new_bool(lhs->data == rhs->data);

        case MP_BINARY_OP_NOT_EQUAL:
            return mp_obj_new_bool(lhs->data != rhs->data);

        default:
            return MP_OBJ_NULL;
    }
}


static mp_obj_t lv_struct_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    mp_lv_struct_t *self = mp_to_lv_struct(self_in);

    if (!self || !self->data) return NULL;

    if (!mp_obj_is_int(index)) {
        RAISE_IndexError("Subscript index must be an integer!");
    }

    const mp_obj_type_t *type = mp_obj_get_type(self_in);
    size_t element_size = get_lv_struct_size(type);

    if (element_size == 0) return mp_const_none;

    size_t element_index = mp_obj_get_int(index);
    void *element_addr = (byte*)self->data + element_size*element_index;

    if (value == MP_OBJ_NULL) {
        memset(element_addr, 0, element_size);
        return self_in;
    }

    mp_lv_struct_t *element_at_index = m_new_obj(mp_lv_struct_t);

    *element_at_index = (mp_lv_struct_t){
        .base = { type },
        .data = element_addr
    };

    if (value != MP_OBJ_SENTINEL) {
        mp_lv_struct_t *other = mp_to_lv_struct(cast(value, type));

        if ((!other) || (!other->data)) return NULL;

        memcpy(element_at_index->data, other->data, element_size);
    }

    return MP_OBJ_FROM_PTR(element_at_index);
}


GENMPY_UNUSED static void *copy_buffer(const void *buffer, size_t size)
{
    void *new_buffer = m_malloc(size);
    memcpy(new_buffer, buffer, size);
    return new_buffer;
}


// Reference an existing lv struct (or part of it)
static mp_obj_t lv_to_mp_struct(const mp_obj_type_t *type, void *lv_struct)
{
    if (lv_struct == NULL) return mp_const_none;

    mp_lv_struct_t *self = m_new_obj(mp_lv_struct_t);

    *self = (mp_lv_struct_t){
        .base = { type },
        .data = lv_struct
    };

    return MP_OBJ_FROM_PTR(self);
}


static void call_parent_methods(mp_obj_t obj, qstr attr, mp_obj_t *dest)
{
    const mp_obj_type_t *type = mp_obj_get_type(obj);

    while (MP_OBJ_TYPE_HAS_SLOT(type, locals_dict)) {
        // generic method lookup
        // this is a lookup in the object (ie not class or type)

        // MicroPython restriction, for now
        assert(MP_OBJ_TYPE_GET_SLOT(type, locals_dict)->base.type == &mp_type_dict);

        mp_map_t *locals_map = &MP_OBJ_TYPE_GET_SLOT(type, locals_dict)->map;
        mp_map_elem_t *elem = mp_map_lookup(locals_map,
                                            MP_OBJ_NEW_QSTR(attr), MP_MAP_LOOKUP);

        if (elem != NULL) {
            mp_convert_member_lookup(obj, type, elem->value, dest);
            break;
        }

        if (MP_OBJ_TYPE_GET_SLOT_OR_NULL(type, parent) == NULL) break;

        // search parents
        type = MP_OBJ_TYPE_GET_SLOT(type, parent);
    }
}


// Convert dict to struct
static mp_obj_t dict_to_struct(mp_obj_t dict, const mp_obj_type_t *type)
{
    mp_obj_t mp_struct = make_new_lv_struct(type, 0, 0, NULL);
    mp_obj_t native_dict = cast(dict, &mp_type_dict);
    mp_map_t *map = mp_obj_dict_get_map(native_dict);

    if (map == NULL) return mp_const_none;

    for (uint i = 0; i < map->alloc; i++) {
        mp_obj_t key = map->table[i].key;
        mp_obj_t value = map->table[i].value;

        if (key != MP_OBJ_NULL) {
            mp_obj_t dest[] = {MP_OBJ_SENTINEL, value};
            MP_OBJ_TYPE_GET_SLOT(type, attr)(mp_struct,
                                             mp_obj_str_get_qstr(key), dest);

            if (dest[0]) {
                RAISE_AttributeError("Cannot set field %s on struct %s!",
                                     qstr_str(mp_obj_str_get_qstr(key)),
                                     qstr_str(type->name));
            }
        }
    }
    return mp_struct;
}


// Convert mp object to ptr
static void* mp_to_ptr(mp_obj_t self_in)
{
    mp_buffer_info_t buffer_info;

    if (self_in == NULL || self_in == mp_const_none) return NULL;

//    if (MP_OBJ_IS_INT(self_in))
//        return (void*)mp_obj_get_int(self_in);

    // If an object is user instance, take it as is so it could be used as user_data
    if (mp_obj_is_instance_type(mp_obj_get_type(self_in))) {
        return MP_OBJ_TO_PTR(self_in);
    }

    if (!mp_get_buffer(self_in, &buffer_info, MP_BUFFER_READ)) {
        // No buffer protocol - this is not a Struct or a Blob, it's some other
        // mp object. We only allow setting dict directly, since it's useful to
        // setting user_data for passing data to C. On other cases throw an
        // exception, to avoid a crash later
        if (MP_OBJ_IS_TYPE(self_in, &mp_type_dict)) {
            return MP_OBJ_TO_PTR(self_in);
        } else {
            RAISE_LvPointerError("Cannot convert '%s' to pointer!",
                                 mp_obj_get_type_str(self_in));
        }
    }

    if (MP_OBJ_IS_STR_OR_BYTES(self_in) ||
        MP_OBJ_IS_TYPE(self_in, &mp_type_bytearray) ||
        MP_OBJ_IS_TYPE(self_in, &mp_type_memoryview)) {

            return buffer_info.buf;
    } else {
        void *result;
        if (buffer_info.len != sizeof(result) ||
            buffer_info.typecode != BYTEARRAY_TYPECODE) {

            RAISE_LvPointerError(
                "Cannot convert %s to pointer! (buffer does not represent a pointer)",
                mp_obj_get_type_str(self_in));
        }

        memcpy(&result, buffer_info.buf, sizeof(result));

        return result;
    }
}


// Blob is a wrapper for void*
static mp_int_t mp_blob_get_buffer(mp_obj_t self_in,
                                   mp_buffer_info_t *bufinfo, mp_uint_t flags)
{
    (void)flags;
    mp_lv_struct_t *self = MP_OBJ_TO_PTR(self_in);

    bufinfo->buf = &self->data;
    bufinfo->len = sizeof(self->data);
    bufinfo->typecode = BYTEARRAY_TYPECODE;

    return 0;
}


static const mp_obj_fun_t mp_lv_dereference_obj;


// Sometimes (but not always!) Blob represents a Micropython object.
// In such cases it's safe to cast the Blob back to the Micropython object
// cast argument is the underlying object type, and it's optional.
static mp_obj_t mp_blob_cast(size_t argc, const mp_obj_t *argv)
{
    mp_obj_t self = argv[0];
    void *ptr = mp_to_ptr(self);

    if (argc == 1) return MP_OBJ_FROM_PTR(ptr);

    mp_obj_t type = argv[1];

    if (!MP_OBJ_IS_TYPE(type, &mp_type_type)) {
        RAISE_TypeError("Cast argument must be a type!");
    }

    return cast(MP_OBJ_FROM_PTR(ptr), type);
}


static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_blob_cast_obj, 1, 2, mp_blob_cast);


static const mp_rom_map_elem_t mp_blob_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___dereference__), MP_ROM_PTR(&mp_lv_dereference_obj) },
    { MP_ROM_QSTR(MP_QSTR___cast__), MP_ROM_PTR(&mp_blob_cast_obj) },
};

static MP_DEFINE_CONST_DICT(mp_blob_locals_dict, mp_blob_locals_dict_table);


static MP_DEFINE_CONST_OBJ_TYPE(
    mp_blob_type,
    MP_QSTR_Blob,
    MP_TYPE_FLAG_NONE,
    binary_op, lv_struct_binary_op,
    locals_dict, &mp_blob_locals_dict,
    buffer, mp_blob_get_buffer
);


static const mp_lv_struct_t mp_lv_null_obj = { {&mp_blob_type}, NULL };


static mp_obj_t ptr_to_mp(void *data)
{
    return lv_to_mp_struct(&mp_blob_type, data);
}


// Cast pointer to struct
static mp_obj_t mp_lv_cast(mp_obj_t type_obj, mp_obj_t ptr_obj)
{
    void *ptr = mp_to_ptr(ptr_obj);

    if (!ptr) return mp_const_none;

    mp_lv_struct_t *self = m_new_obj(mp_lv_struct_t);

    *self = (mp_lv_struct_t){
        .base = { (const mp_obj_type_t*)type_obj },
        .data = ptr
    };

    return MP_OBJ_FROM_PTR(self);
}


// Cast instance. Can be used in ISR when memory allocation is prohibited
static mp_obj_t mp_lv_cast_instance(mp_obj_t self_in, mp_obj_t ptr_obj)
{
    mp_lv_struct_t *self = MP_OBJ_TO_PTR(self_in);
    self->data = mp_to_ptr(ptr_obj);
    return self_in;
}


static MP_DEFINE_CONST_FUN_OBJ_2(mp_lv_cast_obj, mp_lv_cast);

static MP_DEFINE_CONST_CLASSMETHOD_OBJ(mp_lv_cast_class_method,
                                       MP_ROM_PTR(&mp_lv_cast_obj));

static MP_DEFINE_CONST_FUN_OBJ_2(mp_lv_cast_instance_obj, mp_lv_cast_instance);


// Dereference a struct/blob. This allows access to the raw data the struct holds
static mp_obj_t mp_lv_dereference(size_t argc, const mp_obj_t *argv)
{
    mp_obj_t self_in = argv[0];
    mp_obj_t size_in = argc > 1 ? argv[1] : mp_const_none;

    mp_lv_struct_t *self = MP_OBJ_TO_PTR(self_in);
    size_t size = 0;

    if (size_in == mp_const_none) {
        const mp_obj_type_t *type = self->base.type;
        size = get_lv_struct_size(type);
    } else {
        size = (size_t)mp_obj_get_int(size_in);
    }

    if (size == 0) return mp_const_none;

    mp_obj_array_t *view = MP_OBJ_TO_PTR(
                   mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, self->data));

    // used to indicate writable buffer
    view->typecode |= 0x80;

    return MP_OBJ_FROM_PTR(view);
}

static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_lv_dereference_obj, 1,
                                           2, mp_lv_dereference);


// Callback function handling
// Callback is either a callable object or a pointer. If it's a callable object,
// set user_data to the callback. Multiple callbacks are kept per object/struct
// using a dict that associate callback name with callback object. In case of an
// lv_obj_t, user_data is mp_lv_obj_t which contains a member "callbacks" for
// that dict. In case of a struct, user_data is a pointer to that dict directly
static mp_obj_t get_callback_dict_from_user_data(void *user_data)
{
    if (user_data) {
        mp_obj_t obj = MP_OBJ_FROM_PTR(user_data);
#ifdef LV_OBJ_T
        return MP_OBJ_IS_TYPE(obj, &mp_type_dict) ? obj : mp_get_callbacks(obj);
#else
        return obj;
#endif
    }

    return NULL;
}


typedef void *(*mp_lv_get_user_data)(void *);
typedef void (*mp_lv_set_user_data)(void *, void *);


static void *mp_lv_callback(mp_obj_t mp_callback, void *lv_callback,
                            qstr callback_name, void **user_data_ptr,
                            void *containing_struct,
                            mp_lv_get_user_data get_user_data,
                            mp_lv_set_user_data set_user_data)
{
    if (lv_callback && mp_obj_is_callable(mp_callback)) {
        void *user_data = NULL;

        if (user_data_ptr) {
            // user_data is either a dict of callbacks in case of struct,
            // or a pointer to mp_lv_obj_t in case of lv_obj_t
            if (!(*user_data_ptr)) {
                // if it's NULL - it's a dict for a struct
                *user_data_ptr = MP_OBJ_TO_PTR(mp_obj_new_dict(0));
            }
            user_data = *user_data_ptr;

        } else if (get_user_data && set_user_data) {
            user_data = get_user_data(containing_struct);

            if (!user_data) {
                user_data = MP_OBJ_TO_PTR(mp_obj_new_dict(0));
                set_user_data(containing_struct, user_data);
            }
        }

        if (user_data) {
            mp_obj_t callbacks = get_callback_dict_from_user_data(user_data);
            mp_obj_dict_store(callbacks,
                              MP_OBJ_NEW_QSTR(callback_name), mp_callback);
        }

        return lv_callback;

    } else {
        return mp_to_ptr(mp_callback);
    }
}


static int _nesting = 0;


// Function pointers wrapper
static mp_obj_t mp_lv_funcptr(const mp_lv_obj_fun_t *mp_fun, void *lv_fun,
                              void *lv_callback, qstr func_name, void *user_data)
{
    if (lv_fun == NULL) return mp_const_none;

    if (lv_fun == lv_callback) {
        mp_obj_t callbacks = get_callback_dict_from_user_data(user_data);

        if (callbacks) return mp_obj_dict_get(callbacks,
                                              MP_OBJ_NEW_QSTR(func_name));
    }

    mp_lv_obj_fun_t *funcptr = m_new_obj(mp_lv_obj_fun_t);
    *funcptr = *mp_fun;
    funcptr->lv_fun = lv_fun;

    return MP_OBJ_FROM_PTR(funcptr);
}


// Missing implementation for 64bit integer conversion
static unsigned long long mp_obj_get_ull(mp_obj_t obj)
{
    if (mp_obj_is_small_int(obj)) return MP_OBJ_SMALL_INT_VALUE(obj);

    unsigned long long val = 0;
    bool big_endian = !(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);
    mp_obj_int_to_bytes_impl(obj, big_endian, sizeof(val), (byte*)&val);

    return val;
}


// Array of natives
typedef struct mp_lv_array_t
{
    mp_lv_struct_t base;
    size_t element_size;
    bool is_signed;

} mp_lv_array_t;


static mp_obj_t lv_array_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    mp_lv_array_t *self = MP_OBJ_TO_PTR(self_in);

    if (!self || !self->base.data) return NULL;

    if (!mp_obj_is_int(index)) RAISE_IndexError("Subscript index must be an integer!");

    size_t element_size = self->element_size;
    size_t element_index = mp_obj_get_int(index);
    void *element_addr = (byte*)self->base.data + element_size * element_index;
    bool is_signed = self->is_signed;

    union {
        long long val;
        unsigned long long uval;
    } element;

    memset(&element, 0, sizeof(element));

    if (value == MP_OBJ_NULL) memset(element_addr, 0, element_size);

    else if (value == MP_OBJ_SENTINEL) {
        memcpy(&element, element_addr, element_size);

        return is_signed ? mp_obj_new_int_from_ll(element.val) : mp_obj_new_int_from_ull(element.uval);

    } else {
        if (!mp_obj_is_int(value)) {
            RAISE_TypeError("Value '%s' must be an integer!", mp_obj_get_type_str(value));
        }

        element.uval = mp_obj_get_ull(value);
        memcpy(element_addr, &element, element_size);
    }

    return self_in;
}


static const mp_rom_map_elem_t mp_base_struct_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___cast__),          MP_ROM_PTR(&mp_lv_cast_class_method) },
    { MP_ROM_QSTR(MP_QSTR___cast_instance__), MP_ROM_PTR(&mp_lv_cast_instance_obj) },
    { MP_ROM_QSTR(MP_QSTR___dereference__),   MP_ROM_PTR(&mp_lv_dereference_obj)   },
};

static MP_DEFINE_CONST_DICT(mp_base_struct_locals_dict, mp_base_struct_locals_dict_table);


static MP_DEFINE_CONST_OBJ_TYPE(
    mp_lv_base_struct_type,
    MP_QSTR_Struct,
    MP_TYPE_FLAG_NONE,
    binary_op, lv_struct_binary_op,
    subscr, lv_struct_subscr,
    buffer, mp_blob_get_buffer,
    locals_dict, &mp_base_struct_locals_dict
);


// TODO: provide constructor
static MP_DEFINE_CONST_OBJ_TYPE(
    mp_lv_array_type,
    MP_QSTR_C_Array,
    MP_TYPE_FLAG_NONE,
    binary_op, lv_struct_binary_op,
    subscr, lv_array_subscr,
    buffer, mp_blob_get_buffer,
    locals_dict, &mp_base_struct_locals_dict
);


GENMPY_UNUSED static mp_obj_t mp_array_from_ptr(void *lv_arr,
                                                size_t element_size,
                                                bool is_signed)
{
    mp_lv_array_t *self = m_new_obj(mp_lv_array_t);

    *self = (mp_lv_array_t){
        { { &mp_lv_array_type }, lv_arr },
        element_size,
        is_signed
    };

    return MP_OBJ_FROM_PTR(self);
}


GENMPY_UNUSED static void *mp_array_to_ptr(mp_obj_t *mp_arr,
                                           size_t element_size,
                                           GENMPY_UNUSED bool is_signed)
{
    if (MP_OBJ_IS_STR_OR_BYTES(mp_arr) ||
        MP_OBJ_IS_TYPE(mp_arr, &mp_type_bytearray) ||
        MP_OBJ_IS_TYPE(mp_arr, &mp_type_memoryview)) {

            return mp_to_ptr(mp_arr);
    }

    mp_obj_t mp_len = mp_obj_len_maybe(mp_arr);

    if (mp_len == MP_OBJ_NULL) return mp_to_ptr(mp_arr);

    mp_int_t len = mp_obj_get_int(mp_len);
    void *lv_arr = m_malloc(len * element_size);
    byte *element_addr = (byte*)lv_arr;
    mp_obj_t iter = mp_getiter(mp_arr, NULL);
    mp_obj_t item;

    while ((item = mp_iternext(iter)) != MP_OBJ_STOP_ITERATION) {
        union {
            long long val;
            unsigned long long uval;
        } element;

        if (!mp_obj_is_int(item)) {
            RAISE_TypeError("Value '%s' must be an integer!",
                            mp_obj_get_type_str(item));
        }

        element.uval = mp_obj_get_ull(item);
        memcpy(element_addr, &element, element_size);
        element_addr += element_size;
    }

    return lv_arr;
}


#define MP_ARRAY_CONVERTOR(name, size, is_signed)                  \
GENMPY_UNUSED static mp_obj_t mp_array_from_ ## name(void *lv_arr) \
{                                                                  \
    return mp_array_from_ptr(lv_arr, size, is_signed);             \
}                                                                  \
GENMPY_UNUSED static void *mp_array_to_ ## name(mp_obj_t mp_arr)   \
{                                                                  \
    return mp_array_to_ptr(mp_arr, size, is_signed);               \
}


MP_ARRAY_CONVERTOR(u8ptr, 1, false)
MP_ARRAY_CONVERTOR(i8ptr, 1, true)
MP_ARRAY_CONVERTOR(u16ptr, 2, false)
MP_ARRAY_CONVERTOR(i16ptr, 2, true)
MP_ARRAY_CONVERTOR(u32ptr, 4, false)
MP_ARRAY_CONVERTOR(i32ptr, 4, true)
MP_ARRAY_CONVERTOR(u64ptr, 8, false)
MP_ARRAY_CONVERTOR(i64ptr, 8, true)