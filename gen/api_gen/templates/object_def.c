
// /*#~0~#*/ /*#~1~#*/ object definitions
static const mp_rom_map_elem_t /*#~1~#*/_locals_dict_table[] = {
    /*#~2~#*/
};

static MP_DEFINE_CONST_DICT(/*#~1~#*/_locals_dict, /*#~1~#*/_locals_dict_table);

/*#~3~#*/

static MP_DEFINE_CONST_OBJ_TYPE(
    mp_lv_/*#~1~#*/_type_base,
    MP_QSTR_/*#~1~#*/,
    MP_TYPE_FLAG_NONE,
    /*#~4~#*/
    /*#~5~#*/
    attr, call_parent_methods,
    /*#~6~#*/
    /*#~7~#*/
    locals_dict, &/*#~1~#*/_locals_dict
);

GENMPY_UNUSED static const mp_lv_obj_type_t mp_lv_/*#~1~#*/_type = {
#ifdef LV_OBJ_T
    .lv_obj_class = /*#~8~#*/,
#endif
    .mp_obj_type = &mp_lv_/*#~1~#*/_type_base,
};
