
// Struct /*#~1~#*/  {struct_name}
static const mp_obj_type_t mp_/*#~sanitized_struct_name~#*/_type;


static inline void* mp_write_ptr_/*#~sanitized_struct_name~#*/(mp_obj_t self_in)
{
    mp_lv_struct_t *self = MP_OBJ_TO_PTR(cast(self_in, &mp_/*#~sanitized_struct_name~#*/_type));
    return (/*#~struct_tag~#*//*#~struct_name~#*/*)self->data;
}


#define mp_write_/*#~sanitized_struct_name~#*/(struct_obj) *((/*#~struct_tag~#*//*#~struct_name~#*/*)mp_write_ptr_/*#~sanitized_struct_name~#*/(struct_obj))


static inline mp_obj_t mp_read_ptr_/*#~sanitized_struct_name~#*/(void *field)
{
    return lv_to_mp_struct(&mp_/*#~sanitized_struct_name~#*/_type, field);
}


#define mp_read_/*#~sanitized_struct_name~#*/(field) mp_read_ptr_/*#~sanitized_struct_name~#*/(copy_buffer(&field, sizeof(/*#~struct_tag~#*//*#~struct_name~#*/)))
#define mp_read_byref_/*#~sanitized_struct_name~#*/(field) mp_read_ptr_/*#~sanitized_struct_name~#*/(&field)


static void mp_/*#~sanitized_struct_name~#*/_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    mp_lv_struct_t *self = MP_OBJ_TO_PTR(self_in);
    GENMPY_UNUSED /*#~struct_tag~#*//*#~struct_name~#*/ *data = (/*#~struct_tag~#*//*#~struct_name~#*/*)self->data;

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        switch(attr)
        {
            /*#~read_cases~#*/;
            default: call_parent_methods(self_in, attr, dest); // fallback to locals_dict lookup
        }
    } else {
        if (dest[1])
        {
            // store attribute
            switch(attr)
            {
                /*#~write_cases~#*/;
                default: return;
            }

            dest[0] = MP_OBJ_NULL; // indicate success
        }
    }
}

static const mp_obj_dict_t mp_/*#~sanitized_struct_name~#*/_locals_dict;

static MP_DEFINE_CONST_OBJ_TYPE(
    mp_/*#~sanitized_struct_name~#*/_type,
    MP_QSTR_/*#~sanitized_struct_name~#*/,
    MP_TYPE_FLAG_NONE,
    make_new, make_new_lv_struct,
    binary_op, lv_struct_binary_op,
    subscr, lv_struct_subscr,
    attr, mp_/*#~sanitized_struct_name~#*/_attr,
    locals_dict, &mp_/*#~sanitized_struct_name~#*/_locals_dict,
    buffer, mp_blob_get_buffer,
    parent, &mp_lv_base_struct_type
);