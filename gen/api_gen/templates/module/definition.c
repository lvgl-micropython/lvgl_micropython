const mp_obj_module_t mp_module_lvgl = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_lvgl_globals
};


MP_REGISTER_MODULE(MP_QSTR_lvgl, mp_module_lvgl);
