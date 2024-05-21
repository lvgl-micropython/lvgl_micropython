
#include <string.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "mphalport.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_heap_caps.h"

#include "../include/nvs.h"

#define NVS_TYPE_FLOAT  (0xFE)


STATIC mp_nvs_obj_t *_nvs_new(nvs_handle_t ns, const char *ns_name) {
    mp_nvs_obj_t *self = mp_obj_malloc(mp_nvs_obj_t, &mp_nvs_type);
    self->ns = ns;
    self->ns_name = ns_name;
    return self;
}


STATIC void mp_nvs_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, "<NVS namespace>");
}


STATIC mp_obj_t mp_nvs_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    const char *ns_name = mp_obj_str_get_str(all_args[0]);
    nvs_handle_t ns;
    check_esp_err(nvs_open(ns_name, NVS_READWRITE, &ns));
    return MP_OBJ_FROM_PTR(_nvs_new(ns, ns_name));
}


STATIC mp_obj_t mp_nvs_set(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    enum { ARG_self, ARG_type, ARG_key, ARG_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,  MP_ARG_OBJ | MP_ARG_REQUIRED, { .u_obj = mp_const_none } },
        { MP_QSTR_type,  MP_ARG_INT | MP_ARG_REQUIRED, { .u_int = 0             } },
        { MP_QSTR_key,   MP_ARG_OBJ | MP_ARG_REQUIRED, { .u_obj = mp_const_none } },
        { MP_QSTR_value, MP_ARG_OBJ | MP_ARG_REQUIRED, { .u_obj = mp_const_none } }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_nvs_obj_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);
    uint8_t type = (uint8_t) args[ARG_type].u_int;
    const char *key = mp_obj_str_get_str(args[ARG_key].u_obj);

    esp_err_t err;
    switch(type) {
        case NVS_TYPE_U8:
            uint8_t u8_value = (uint8_t)mp_obj_get_int(args[ARG_value].u_obj);
            err = nvs_set_u8(self->ns, key, u8_value);
            break;
        case NVS_TYPE_I8:
            int8_t i8_value = (int8_t)mp_obj_get_int(args[ARG_value].u_obj);
            err = nvs_set_i8(self->ns, key, i8_value);
            break;
        case NVS_TYPE_U16:
            uint16_t u16_value = (uint16_t)mp_obj_get_int(args[ARG_value].u_obj);
            err = nvs_set_u16(self->ns, key, u16_value);
            break;
        case NVS_TYPE_I16:
            int16_t i16_value = (int16_t)mp_obj_get_int(args[ARG_value].u_obj);
            err = nvs_set_i16(self->ns, key, i16_value);
            break;
        case NVS_TYPE_U32:
            uint32_t u32_value = (uint32_t)mp_obj_get_int(args[ARG_value].u_obj);
            err = nvs_set_u32(self->ns, key, u32_value);
            break;
        case NVS_TYPE_I32:
            int32_t i32_value = (int32_t)mp_obj_get_int(args[ARG_value].u_obj);
            err = nvs_set_i32(self->ns, key, i32_value);
            break;
        case NVS_TYPE_U64:
            uint64_t u64_value = (uint64_t)mp_obj_get_int(args[ARG_value].u_obj);
            err = nvs_set_u64(self->ns, key, u64_value);
            break;
        case NVS_TYPE_I64:
            int64_t i64_value = (int64_t)mp_obj_get_int(args[ARG_value].u_obj);
            err = nvs_set_i64(self->ns, key, i64_value);
            break;
        case NVS_TYPE_STR:
            const char *str_value = mp_obj_str_get_str(args[ARG_value].u_obj);
            err = nvs_set_str(self->ns, key, str_value);
            break;
        case NVS_TYPE_BLOB:
            mp_buffer_info_t blob_value;
            mp_get_buffer_raise(args[ARG_value].u_obj, &blob_value, MP_BUFFER_READ);
            err = nvs_set_blob(self->ns, key, blob_value.buf, blob_value.len);
            break;
        case NVS_TYPE_FLOAT:
            float float_value = (float)mp_obj_get_float(args[ARG_value].u_obj);
            union {
                float f;
                uint32_t i;
            } u;

            u.f = float_value;
            uint64_t temp_val = 0x8000000000000000 | (uint64_t)u.i;
            err = nvs_set_u64(self->ns, key, temp_val);
            break;

        default:
            err = ESP_ERR_NVS_TYPE_MISMATCH;
            break;
    }

    check_esp_err(err);

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_KW(mp_nvs_set_obj, 4, mp_nvs_set);


STATIC mp_obj_t mp_nvs_get(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {

    enum { ARG_self, ARG_type, ARG_key};
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_self,  MP_ARG_OBJ | MP_ARG_REQUIRED, { .u_obj = mp_const_none } },
        { MP_QSTR_type,  MP_ARG_INT | MP_ARG_REQUIRED, { .u_int = 0             } },
        { MP_QSTR_key,   MP_ARG_OBJ | MP_ARG_REQUIRED, { .u_obj = mp_const_none } },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    mp_nvs_obj_t *self = MP_OBJ_TO_PTR(args[ARG_self].u_obj);
    uint8_t type = (uint8_t)args[ARG_type].u_int;
    const char *key = mp_obj_str_get_str(args[ARG_key].u_obj);

    esp_err_t err;
    mp_obj_t value;

    switch(type) {
        case NVS_TYPE_U8:
            uint8_t u8_value;
            err = nvs_get_u8(self->ns, key, &u8_value);
            value = mp_obj_new_int_from_uint(u8_value);
            break;
        case NVS_TYPE_I8:
            int8_t i8_value;
            err = nvs_get_i8(self->ns, key, &i8_value);
            value = mp_obj_new_int(i8_value);
            break;
        case NVS_TYPE_U16:
            uint16_t u16_value;
            err = nvs_get_u16(self->ns, key, &u16_value);
            value = mp_obj_new_int_from_uint(u16_value);
            break;
        case NVS_TYPE_I16:
            int16_t i16_value;
            err = nvs_get_i16(self->ns, key, &i16_value);
            value = mp_obj_new_int(i16_value);
            break;
        case NVS_TYPE_U32:
            uint32_t u32_value;
            err = nvs_get_u32(self->ns, key, &u32_value);
            value = mp_obj_new_int_from_uint(u32_value);
            break;
        case NVS_TYPE_I32:
            int32_t i32_value;
            err = nvs_get_i32(self->ns, key, &i32_value);
            value = mp_obj_new_int(i32_value);
            break;
        case NVS_TYPE_U64:
            uint64_t u64_value;
            err = nvs_get_u64(self->ns, key, &u64_value);
            value = mp_obj_new_int_from_uint(u64_value);
            break;
        case NVS_TYPE_I64:
            int64_t i64_value;
            err = nvs_get_i64(self->ns, key, &i64_value);
            value = mp_obj_new_int(i64_value);
            break;
        case NVS_TYPE_STR:
            size_t str_len;
            nvs_get_str(self->ns, key, NULL, &str_len);
            char *str_value = (char *)heap_caps_malloc(str_len, 0);  // might be a memory leak, don't know if I need to free it after creating the str object
            err = nvs_get_str(self->ns, key, str_value, &str_len);
            value = mp_obj_new_str(str_value, str_len);
            break;
        case NVS_TYPE_BLOB:
            size_t blob_len;
            nvs_get_blob(self->ns, key, NULL, &blob_len);
            uint8_t *data = (uint8_t *)heap_caps_malloc(blob_len, 0);  // might be a memory leak, don't know if I need to free it after creating the bytes object
            err = nvs_get_blob(self->ns, key, data, &blob_len);
            value = mp_obj_new_bytes(data, blob_len);
            break;
        case NVS_TYPE_FLOAT:
            union {
                float f;
                uint32_t i;
            } u;

            uint64_t tmp_val;
            err = nvs_get_u64(self->ns, key, &tmp_val);
            u.i = (uint32_t)(tmp_val & 0xFFFFFFFF);
            value = mp_obj_new_float(u.f);
            break;
        default:
            value = mp_const_none;
            err = ESP_ERR_NVS_TYPE_MISMATCH;
            break;
    }

    check_esp_err(err);
    return value;
}


MP_DEFINE_CONST_FUN_OBJ_KW(mp_nvs_get_obj, 3, mp_nvs_get);


STATIC mp_obj_t mp_nvs_erase(mp_obj_t self_in, mp_obj_t key_in) {
    mp_nvs_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const char *key = mp_obj_str_get_str(key_in);
    check_esp_err(nvs_erase_key(self->ns, key));
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(mp_nvs_erase_obj, mp_nvs_erase);


STATIC mp_obj_t mp_nvs_commit(mp_obj_t self_in) {
    mp_nvs_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_esp_err(nvs_commit(self->ns));
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_nvs_commit_obj, mp_nvs_commit);



STATIC mp_obj_t mp_nvs_close(mp_obj_t self_in) {
    mp_nvs_obj_t *self = MP_OBJ_TO_PTR(self_in);
    nvs_close(self->ns);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_nvs_close_obj, mp_nvs_close);



STATIC mp_obj_t mp_nvs_get_keys(mp_obj_t self_in) {
    mp_nvs_obj_t *self = MP_OBJ_TO_PTR(self_in);

    size_t used_entries = 0;
    nvs_get_used_entry_count(self->ns, &used_entries);

    nvs_iterator_t it = NULL;
    esp_err_t err = nvs_entry_find(NVS_DEFAULT_PART_NAME, self->ns_name, NVS_TYPE_ANY, &it);

    mp_obj_t dict = mp_obj_new_dict(used_entries);

    while(err == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info); // Can omit error check if parameters are guaranteed to be non-NULL

        if (info.type == NVS_TYPE_I64) {
            uint64_t tmp_val;
            nvs_get_u64(self->ns, info.key, &tmp_val);
            if ((tmp_val >> 63) & 0x1) {
                info.type = NVS_TYPE_FLOAT;
                for (uint8_t i=32;i<63;i++) {
                    if ((tmp_val >> i) & 0x1) {
                        info.type = NVS_TYPE_I64;
                        break;
                    }
                }
            }
        }

        mp_obj_dict_store(
            dict,
            mp_obj_new_str(info.key, strlen(info.key)),
            mp_obj_new_int_from_uint(info.type)
        );

        err = nvs_entry_next(&it);
    }

    nvs_release_iterator(it);

    if (err != ESP_ERR_NVS_NOT_FOUND) {
        check_esp_err(err);
    }

    return dict;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_nvs_get_keys_obj, mp_nvs_get_keys);


STATIC mp_obj_t mp_nvs_reset(mp_obj_t self_in) {
    mp_nvs_obj_t *self = MP_OBJ_TO_PTR(self_in);
    check_esp_err(nvs_erase_all(self->ns));
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(mp_nvs_reset_obj, mp_nvs_reset);


STATIC const mp_rom_map_elem_t mp_nvs_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get),      MP_ROM_PTR(&mp_nvs_get_obj)      },
    { MP_ROM_QSTR(MP_QSTR_set),      MP_ROM_PTR(&mp_nvs_set_obj)      },
    { MP_ROM_QSTR(MP_QSTR_erase),    MP_ROM_PTR(&mp_nvs_erase_obj)    },
    { MP_ROM_QSTR(MP_QSTR_commit),   MP_ROM_PTR(&mp_nvs_commit_obj)   },
    { MP_ROM_QSTR(MP_QSTR_close),    MP_ROM_PTR(&mp_nvs_close_obj)    },
    { MP_ROM_QSTR(MP_QSTR___del__),  MP_ROM_PTR(&mp_nvs_close_obj)    },
    { MP_ROM_QSTR(MP_QSTR_get_keys), MP_ROM_PTR(&mp_nvs_get_keys_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset),    MP_ROM_PTR(&mp_nvs_reset_obj)    },
};

STATIC MP_DEFINE_CONST_DICT(mp_nvs_locals_dict, mp_nvs_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_nvs_type,
    MP_QSTR_NVS,
    MP_TYPE_FLAG_NONE,
    make_new, mp_nvs_make_new,
    print, mp_nvs_print,
    locals_dict, &mp_nvs_locals_dict
);


STATIC const mp_rom_map_elem_t mp_nvs_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),   MP_OBJ_NEW_QSTR(MP_QSTR_nvs) },
    { MP_ROM_QSTR(MP_QSTR_NVS),        (mp_obj_t)&mp_nvs_type       },
    { MP_ROM_QSTR(MP_QSTR_TYPE_U8),    MP_ROM_INT(NVS_TYPE_U8)      },
    { MP_ROM_QSTR(MP_QSTR_TYPE_I8),    MP_ROM_INT(NVS_TYPE_I8)      },
    { MP_ROM_QSTR(MP_QSTR_TYPE_U16),   MP_ROM_INT(NVS_TYPE_U16)     },
    { MP_ROM_QSTR(MP_QSTR_TYPE_I16),   MP_ROM_INT(NVS_TYPE_I16)     },
    { MP_ROM_QSTR(MP_QSTR_TYPE_U32),   MP_ROM_INT(NVS_TYPE_U32)     },
    { MP_ROM_QSTR(MP_QSTR_TYPE_I32),   MP_ROM_INT(NVS_TYPE_I32)     },
    { MP_ROM_QSTR(MP_QSTR_TYPE_U64),   MP_ROM_INT(NVS_TYPE_U64)     },
    { MP_ROM_QSTR(MP_QSTR_TYPE_I64),   MP_ROM_INT(NVS_TYPE_I64)     },
    { MP_ROM_QSTR(MP_QSTR_TYPE_STR),   MP_ROM_INT(NVS_TYPE_STR)     },
    { MP_ROM_QSTR(MP_QSTR_TYPE_BLOB),  MP_ROM_INT(NVS_TYPE_BLOB)    },
    { MP_ROM_QSTR(MP_QSTR_TYPE_FLOAT), MP_ROM_INT(NVS_TYPE_FLOAT)   },
};

STATIC MP_DEFINE_CONST_DICT(mp_nvs_module_globals, mp_nvs_module_globals_table);


const mp_obj_module_t mp_module_nvs = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_nvs_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_nvs, mp_module_nvs);
