

GENMPY_UNUSED static mp_obj_t /*#~0~#*/(/*#~1~#*/ *arr)
{
    mp_obj_t obj_arr[/*#~3~#*/];
    for (size_t i=0; i</*#~3~#*/; i++){{
        obj_arr[i] = /*#~2~#*/(arr[i]);
    }}
    return mp_obj_new_list(/*#~3~#*/, obj_arr); // TODO: return custom iterable object!
}