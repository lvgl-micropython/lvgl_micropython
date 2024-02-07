#include "py/obj.h"
#include "py/runtime.h"
#include "py/objarray.h"
#include "py/binary.h"

#include "esp_heap_caps.h"

#include <string.h>


STATIC mp_obj_t mp_heap_caps_malloc(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_size, ARG_caps };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_size,         MP_ARG_INT | MP_ARG_REQUIRED  },
        { MP_QSTR_caps,         MP_ARG_INT | MP_ARG_REQUIRED  }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    size_t size = (size_t)args[ARG_size].u_int;
    uint32_t caps = (uint32_t)args[ARG_caps].u_int;

    void *buf = heap_caps_calloc(1, size, caps);

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

    if (mp_obj_is_type(args[ARG_buf].u_obj, &mp_type_list)) {
        volatile mp_obj_t seq = args[ARG_buf].u_obj;
        mp_obj_t *seq_items;
        size_t seq_len;
        mp_obj_get_array(seq, &seq_len, &seq_items);

        mp_obj_array_t *buf = NULL;

        for (size_t i = 0; i < seq_len; i++) {
            buf = (mp_obj_array_t *)seq_items[i];
            heap_caps_free((void *)buf->items);
        }
    } else {
        mp_obj_array_t *buf = (mp_obj_array_t *)args[ARG_buf].u_obj;
        heap_caps_free((void *)buf->items);
    }

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
    uint32_t caps = (uint32_t)args[ARG_caps].u_int;

    void *new_buf = heap_caps_realloc((void *)buf->items, size, caps);

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
    enum { ARG_alignment, ARG_size, ARG_caps };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_alignment,    MP_ARG_INT | MP_ARG_REQUIRED  },
        { MP_QSTR_size,         MP_ARG_INT | MP_ARG_REQUIRED  },
        { MP_QSTR_caps,         MP_ARG_INT | MP_ARG_REQUIRED  }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    size_t alignment = (size_t)args[ARG_alignment].u_int;
    size_t size = (size_t)args[ARG_size].u_int;
    uint32_t caps = (uint32_t)args[ARG_caps].u_int;

    void *buf = heap_caps_aligned_alloc(alignment, size, caps);

    if (buf == NULL) {
        return mp_const_none;
    } else {
        mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, buf));
        view->typecode |= 0x80; // used to indicate writable buffer
        return MP_OBJ_FROM_PTR(view);
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_heap_caps_aligned_alloc_obj, 3, mp_heap_caps_aligned_alloc);


STATIC mp_obj_t mp_heap_caps_aligned_calloc(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_alignment, ARG_count, ARG_size, ARG_caps };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_alignment,    MP_ARG_INT | MP_ARG_REQUIRED  },
        { MP_QSTR_count,            MP_ARG_INT | MP_ARG_REQUIRED  },
        { MP_QSTR_size,         MP_ARG_INT | MP_ARG_REQUIRED  },
        { MP_QSTR_caps,         MP_ARG_INT | MP_ARG_REQUIRED  }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    size_t alignment = (size_t)args[ARG_alignment].u_int;
    size_t count = (size_t)args[ARG_count].u_int;
    size_t size = (size_t)args[ARG_size].u_int;
    uint32_t caps = (uint32_t)args[ARG_caps].u_int;

    void *buf = heap_caps_aligned_calloc(alignment, count, size, caps);

    if (buf == NULL) {
        return mp_const_none;
    } else {
        mp_obj_t data = mp_obj_new_list(count, NULL);

        for (int i = 0; i < count; i++) {
            mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, buf));
            view->typecode |= 0x80; // used to indicate writable buffer
            mp_obj_list_store(data, mp_obj_new_int(i), MP_OBJ_FROM_PTR(view));
            buf += size;
        }

        return data;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_heap_caps_aligned_calloc_obj, 4, mp_heap_caps_aligned_calloc);


STATIC mp_obj_t mp_heap_caps_calloc(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_count, ARG_size, ARG_caps };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_count,            MP_ARG_INT | MP_ARG_REQUIRED  },
        { MP_QSTR_size,         MP_ARG_INT | MP_ARG_REQUIRED  },
        { MP_QSTR_caps,         MP_ARG_INT | MP_ARG_REQUIRED  }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    size_t count = (size_t)args[ARG_count].u_int;
    size_t size = (size_t)args[ARG_size].u_int;
    uint32_t caps = (uint32_t)args[ARG_caps].u_int;

    void *buf = heap_caps_calloc(count, size, caps);

    if (buf == NULL) {
        return mp_const_none;
    } else {
        mp_obj_t data = mp_obj_new_list(count, NULL);

        for (int i = 0; i < count; i++) {
            mp_obj_array_t *view = MP_OBJ_TO_PTR(mp_obj_new_memoryview(BYTEARRAY_TYPECODE, size, buf));
            view->typecode |= 0x80; // used to indicate writable buffer
            mp_obj_list_store(data, mp_obj_new_int(i), MP_OBJ_FROM_PTR(view));
            buf += size;
        }

        return data;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(mp_heap_caps_calloc_obj, 3, mp_heap_caps_calloc);


STATIC const mp_map_elem_t mp_module_heap_caps_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),                            MP_OBJ_NEW_QSTR(MP_QSTR_heap_caps)                         },
    { MP_ROM_QSTR(MP_QSTR_malloc),                              (mp_obj_t)&mp_heap_caps_malloc_obj                         },
    { MP_ROM_QSTR(MP_QSTR_free),                                (mp_obj_t)&mp_heap_caps_free_obj                           },
    { MP_ROM_QSTR(MP_QSTR_realloc),                             (mp_obj_t)&mp_heap_caps_realloc_obj                        },
    { MP_ROM_QSTR(MP_QSTR_aligned_alloc),                       (mp_obj_t)&mp_heap_caps_aligned_alloc_obj                  },
    { MP_ROM_QSTR(MP_QSTR_aligned_calloc),                      (mp_obj_t)&mp_heap_caps_aligned_calloc_obj                 },
    { MP_ROM_QSTR(MP_QSTR_calloc),                              (mp_obj_t)&mp_heap_caps_calloc_obj                         },
    { MP_ROM_QSTR(MP_QSTR_CAP_EXEC),                            MP_ROM_INT(MALLOC_CAP_EXEC)                      },
    { MP_ROM_QSTR(MP_QSTR_CAP_32BIT),                           MP_ROM_INT(MALLOC_CAP_32BIT)                     },
    { MP_ROM_QSTR(MP_QSTR_CAP_8BIT),                            MP_ROM_INT(MALLOC_CAP_8BIT)                      },
    { MP_ROM_QSTR(MP_QSTR_CAP_DMA),                             MP_ROM_INT(MALLOC_CAP_DMA)                       },
    { MP_ROM_QSTR(MP_QSTR_CAP_PID2),                            MP_ROM_INT(MALLOC_CAP_PID2)                      },
    { MP_ROM_QSTR(MP_QSTR_CAP_PID3),                            MP_ROM_INT(MALLOC_CAP_PID3)                      },
    { MP_ROM_QSTR(MP_QSTR_CAP_PID4),                            MP_ROM_INT(MALLOC_CAP_PID4)                      },
    { MP_ROM_QSTR(MP_QSTR_CAP_PID5),                            MP_ROM_INT(MALLOC_CAP_PID5)                      },
    { MP_ROM_QSTR(MP_QSTR_CAP_PID6),                            MP_ROM_INT(MALLOC_CAP_PID6)                      },
    { MP_ROM_QSTR(MP_QSTR_CAP_PID7),                            MP_ROM_INT(MALLOC_CAP_PID7)                      },
    { MP_ROM_QSTR(MP_QSTR_CAP_SPIRAM),                          MP_ROM_INT(MALLOC_CAP_SPIRAM)                    },
    { MP_ROM_QSTR(MP_QSTR_CAP_INTERNAL),                        MP_ROM_INT(MALLOC_CAP_INTERNAL)                  },
    { MP_ROM_QSTR(MP_QSTR_CAP_DEFAULT),                         MP_ROM_INT(MALLOC_CAP_DEFAULT)                   },
    { MP_ROM_QSTR(MP_QSTR_CAP_IRAM_8BIT),                       MP_ROM_INT(MALLOC_CAP_IRAM_8BIT)                 },
    { MP_ROM_QSTR(MP_QSTR_CAP_RETENTION),                       MP_ROM_INT(MALLOC_CAP_RETENTION)                 },
    { MP_ROM_QSTR(MP_QSTR_CAP_RTCRAM),                          MP_ROM_INT(MALLOC_CAP_RTCRAM)                    },

};

STATIC MP_DEFINE_CONST_DICT(mp_module_heap_caps_globals, mp_module_heap_caps_globals_table);


const mp_obj_module_t mp_module_heap_caps = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_heap_caps_globals,
};

MP_REGISTER_MODULE(MP_QSTR_heap_caps, mp_module_heap_caps);
