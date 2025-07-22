

typedef mp_obj_t (*mp_fun_ptr_t)(size_t n, const mp_obj_t *, void *ptr);


typedef struct mp_c_obj_fun_t {
    mp_obj_base_t base;
    mp_uint_t n_args;
    mp_fun_ptr_t mp_fun;
    void *c_fun;
} mp_c_obj_fun_t;



static mp_int_t mp_c_func_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags);
static mp_obj_t mp_c_fun_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args);


GENMPY_UNUSED static MP_DEFINE_CONST_OBJ_TYPE(
    mp_c_fun_type,
    MP_QSTR_function,
    MP_TYPE_FLAG_BUILTIN_FUN,
    call, mp_c_fun_call,
    buffer, mp_c_func_get_buffer
);


static mp_obj_t mp_c_fun_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_c_obj_fun_t *self = MP_OBJ_TO_PTR(self_in);
    mp_arg_check_num(n_args, n_kw, self->n_args, self->n_args, false);
    return self->mp_fun(n_args, args, self->c_fun);
}


static mp_int_t mp_c_func_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;

    mp_c_obj_fun_t *self = MP_OBJ_TO_PTR(self_in);

    bufinfo->buf = &self->c_fun;
    bufinfo->len = sizeof(self->c_fun);
    bufinfo->typecode = BYTEARRAY_TYPECODE;
    return 0;
}


#define MP_DEFINE_CONST_C_FUN_OBJ(obj_name, n_args, mp_fun, c_fun) \
const mp_c_obj_fun_t obj_name = { { &mp_c_fun_type }, n_args, mp_fun, c_fun }









