



typedef struct mp_lv_struct_t
{
    mp_obj_base_t base;
    void *data;
} mp_lv_struct_t;



typedef struct mp_lv_array_t
{
    mp_lv_struct_t base;
    size_t element_size;
    bool is_signed;
} mp_lv_array_t;


static mp_obj_t lv_array_unary_op(mp_unary_op_t op, mp_obj_t o_in) {
    mp_obj_array_t *o = MP_OBJ_TO_PTR(o_in);
    switch (op) {
        case MP_UNARY_OP_BOOL:
            return mp_obj_new_bool(o->len != 0);
        case MP_UNARY_OP_LEN:
            return MP_OBJ_NEW_SMALL_INT(o->len);
        default:
            return MP_OBJ_NULL;      // op not supported
    }
}


static mp_obj_t lv_array_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in)
{
    mp_lv_struct_t *lhs = MP_OBJ_TO_PTR(lhs_in);
    mp_lv_struct_t *rhs = MP_OBJ_TO_PTR(rhs_in);
    switch (op)
    {
        case MP_BINARY_OP_EQUAL:
            return mp_obj_new_bool(lhs->data == rhs->data);
        case MP_BINARY_OP_NOT_EQUAL:
            return mp_obj_new_bool(lhs->data != rhs->data);
        case MP_BINARY_OP_CONTAINS:

        MP_BINARY_OP_INPLACE_MULTIPLY
        MP_BINARY_OP_MULTIPLY
        MP_BINARY_OP_ADD

        MP_BINARY_OP_INPLACE_ADD

        default:
            return MP_OBJ_NULL;
    }
}


static mp_int_t mp_array_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    mp_lv_struct_t *self = MP_OBJ_TO_PTR(self_in);

    bufinfo->buf = &self->data;
    bufinfo->len = sizeof(self->data);
    bufinfo->typecode = BYTEARRAY_TYPECODE;
    return 0;
}


static mp_obj_t lv_array_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    mp_lv_array_t *self = MP_OBJ_TO_PTR(self_in);

    if ((!self) || (!self->base.data))
        return NULL;
    if (!mp_obj_is_int(index)) {
        nlr_raise(
            mp_obj_new_exception_msg(
                &mp_type_SyntaxError, MP_ERROR_TEXT("Subscript index must be an integer!")));
    }

    size_t element_size = self->element_size;
    size_t element_index = mp_obj_get_int(index);
    void *element_addr = (byte*)self->base.data + element_size*element_index;
    bool is_signed = self->is_signed;
    union {
        long long val;
        unsigned long long uval;
    } element;
    memset(&element, 0, sizeof(element));

    if (value == MP_OBJ_NULL){
        memset(element_addr, 0, element_size);
    }
    else if (value == MP_OBJ_SENTINEL){
        memcpy(&element, element_addr, element_size);
        return is_signed? mp_obj_new_int_from_ll(element.val): mp_obj_new_int_from_ull(element.uval);
    } else {
        if (!mp_obj_is_int(value)) {
            nlr_raise(
                mp_obj_new_exception_msg_varg(
                    &mp_type_SyntaxError, MP_ERROR_TEXT("Value '%s' must be an integer!"), mp_obj_get_type_str(value)));
        }
        element.uval = mp_obj_get_ull(value);
        memcpy(element_addr, &element, element_size);
    }

    return self_in;
}



static MP_DEFINE_CONST_OBJ_TYPE(
    mp_lv_array_type,
    MP_QSTR_C_Array,
    MP_TYPE_FLAG_NONE,
    binary_op, lv_array_binary_op,
    subscr, lv_array_subscr,
    buffer, mp_array_get_buffer,
    locals_dict, &mp_base_struct_locals_dict
);

GENMPY_UNUSED static mp_obj_t mp_array_from_ptr(void *lv_arr, size_t element_size, bool is_signed)
{
    mp_lv_array_t *self = m_new_obj(mp_lv_array_t);
    *self = (mp_lv_array_t){
        { {&mp_lv_array_type}, lv_arr },
        element_size,
        is_signed
    };
    return MP_OBJ_FROM_PTR(self);
}

GENMPY_UNUSED static void *mp_array_to_ptr(mp_obj_t *mp_arr, size_t element_size, GENMPY_UNUSED bool is_signed)
{
    if (MP_OBJ_IS_STR_OR_BYTES(mp_arr) ||
        MP_OBJ_IS_TYPE(mp_arr, &mp_type_bytearray) ||
        MP_OBJ_IS_TYPE(mp_arr, &mp_type_memoryview)){
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
            nlr_raise(
                mp_obj_new_exception_msg_varg(
                    &mp_type_SyntaxError, MP_ERROR_TEXT("Value '%s' must be an integer!"), mp_obj_get_type_str(item)));
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
                                                                   \
                                                                   \
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