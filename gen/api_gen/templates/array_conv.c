
// Array convertors for /*#~0~#*/

GENMPY_UNUSED static /*#~1~#*//*#~2~#*/ */*#~3~#*/(mp_obj_t mp_arr)
{
    mp_obj_t mp_len = mp_obj_len_maybe(mp_arr);

    if (mp_len == MP_OBJ_NULL) return mp_to_ptr(mp_arr);

    mp_int_t len = mp_obj_get_int(mp_len);
    /*#~4~#*/
    /*#~1~#*//*#~2~#*/ *lv_arr = (/*#~1~#*//*#~2~#*/*)m_malloc(len * sizeof(/*#~1~#*//*#~2~#*/));
    mp_obj_t iter = mp_getiter(mp_arr, NULL);
    mp_obj_t item;
    size_t i = 0;

    while ((item = mp_iternext(iter)) != MP_OBJ_STOP_ITERATION) {
        lv_arr[i++] = /*#~5~#*/(item);
    }

    return (/*#~1~#*//*#~2~#*/ *)lv_arr;
}