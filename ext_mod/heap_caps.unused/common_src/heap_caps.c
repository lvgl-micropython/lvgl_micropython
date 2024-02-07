#include "py/obj.h"
#include "py/runtime.h"
#include "py/objarray.h"
#include "py/binary.h"
#include "py/misc.h"

#include <string.h>

#define HEAP_CAPS_UNUSED(x) ((void)x)


STATIC mp_obj_t mp_heap_caps_malloc(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_size, ARG_caps };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_size,         MP_ARG_INT | MP_ARG_REQUIRED  },
        { MP_QSTR_caps,         MP_ARG_INT | MP_ARG_REQUIRED  }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    size_t size = (size_t)args[ARG_size].u_int;

    void *buf = m_malloc(size);

    if (buf == NULL) {
        return mp_const_none;
    } else {
        // return mp_obj_new_bytearray_by_ref(size, buf);
        mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, buf));
        view->typecode |= 0x80; // used to indicate writable buffer
        return MP_OBJ_FROM_PTR(view);
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_heap_caps_malloc_obj, 2, mp_heap_caps_malloc);


STATIC mp_obj_t mp_heap_caps_free(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_buf };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_buf,          MP_ARG_OBJ | MP_ARG_REQUIRED  },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_array_t *buf = (mp_obj_array_t *)args[ARG_buf].u_obj;
    m_free((void *)buf->items);

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_heap_caps_free_obj, 1, mp_heap_caps_free);


STATIC mp_obj_t mp_heap_caps_realloc(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_buf, ARG_size, ARG_caps };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_buf,          MP_ARG_OBJ | MP_ARG_REQUIRED  },
        { MP_QSTR_size,         MP_ARG_INT | MP_ARG_REQUIRED  },
        { MP_QSTR_caps,         MP_ARG_INT | MP_ARG_REQUIRED  }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_obj_array_t *buf = (mp_obj_array_t *)args[ARG_buf].u_obj;
    size_t size = (size_t)args[ARG_size].u_int;

    void *new_buf = m_realloc((void *)buf->items, size);

    if (new_buf == NULL) {
        return mp_const_none;
    } else {
        mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, new_buf));
        view->typecode |= 0x80; // used to indicate writable buffer
        return MP_OBJ_FROM_PTR(view);
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_heap_caps_realloc_obj, 3, mp_heap_caps_realloc);


STATIC mp_obj_t mp_heap_caps_aligned_alloc(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    HEAP_CAPS_UNUSED(n_args);
    HEAP_CAPS_UNUSED(pos_args);
    HEAP_CAPS_UNUSED(kw_args);
    mp_raise_msg(&mp_type_NotImplementedError, MP_ERROR_TEXT("aligned_alloc but is not available for this MCU"));
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_heap_caps_aligned_alloc_obj, 3, mp_heap_caps_aligned_alloc);


STATIC mp_obj_t mp_heap_caps_aligned_calloc(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    HEAP_CAPS_UNUSED(n_args);
    HEAP_CAPS_UNUSED(pos_args);
    HEAP_CAPS_UNUSED(kw_args);
    mp_raise_msg(&mp_type_NotImplementedError, MP_ERROR_TEXT("aligned_calloc but is not available for this MCU"));
    return mp_const_none;
}


STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_heap_caps_aligned_calloc_obj, 4, mp_heap_caps_aligned_calloc);


STATIC mp_obj_t mp_heap_caps_calloc(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    HEAP_CAPS_UNUSED(n_args);
    HEAP_CAPS_UNUSED(pos_args);
    HEAP_CAPS_UNUSED(kw_args);
    mp_raise_msg(&mp_type_NotImplementedError, MP_ERROR_TEXT("calloc but is not available for this MCU"));
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_heap_caps_calloc_obj, 3, mp_heap_caps_calloc);


STATIC const mp_map_elem_t mp_module_heap_caps_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),                            MP_OBJ_NEW_QSTR(MP_QSTR_heap_caps)                         },
    { MP_ROM_QSTR(MP_QSTR_malloc),                              (mp_obj_t)&mp_heap_caps_malloc_obj                         },
    { MP_ROM_QSTR(MP_QSTR_free),                                (mp_obj_t)&mp_heap_caps_free_obj                           },
    { MP_ROM_QSTR(MP_QSTR_realloc),                             (mp_obj_t)&mp_heap_caps_realloc_obj                        },
    { MP_ROM_QSTR(MP_QSTR_aligned_alloc),                       (mp_obj_t)&mp_heap_caps_aligned_alloc_obj                  },
    { MP_ROM_QSTR(MP_QSTR_aligned_calloc),                      (mp_obj_t)&mp_heap_caps_aligned_calloc_obj                 },
    { MP_ROM_QSTR(MP_QSTR_calloc),                              (mp_obj_t)&mp_heap_caps_calloc_obj                         }
};

STATIC MP_DEFINE_CONST_DICT(mp_module_heap_caps_globals, mp_module_heap_caps_globals_table);


const mp_obj_module_t mp_module_heap_caps = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_heap_caps_globals,
};

MP_REGISTER_MODULE(MP_QSTR_heap_caps, mp_module_heap_caps);
