
// Callback function /*#~0~#*/
// /*#~1~#*/

GENMPY_UNUSED static /*#~4~#*/ /*#~0~#*/_callback(/*#~2~#*/)
{
    mp_obj_t mp_args[/*#~3~#*/];
    /*#~7~#*/
    mp_obj_t callbacks = get_callback_dict_from_user_data(/*#~8~#*/);
    _nesting++;
    /*#~6~#*/mp_call_function_n_kw(mp_obj_dict_get(callbacks, MP_OBJ_NEW_QSTR(MP_QSTR_/*#~0~#*/)) , /*#~3~#*/, 0, mp_args);
    _nesting--;
    return/*#~5~#*/;
}