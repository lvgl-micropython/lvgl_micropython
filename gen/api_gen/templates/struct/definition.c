// Struct /*#~0~#*/
static const mp_obj_type_t mp_/*#~0~#*/_type;

static void* mp_write_ptr_/*#~0~#*/(mp_obj_t self_in)
{
    mp_lv_struct_t *self = MP_OBJ_TO_PTR(cast(self_in, &mp_/*#~0~#*/_type));
    return (/*#~0~#*/ *)self->data;
}

mp_write_ptr_
mp_read_ptr_
mp_read_byref_

#define mp_write_/*#~0~#*/(struct_obj) *((/*#~0~#*/ *)mp_write_ptr_/*#~0~#*/(struct_obj))

static mp_obj_t mp_read_ptr_/*#~0~#*/(void *field)
{
    return lv_to_mp_struct(&mp_/*#~0~#*/_type, field);
}

#define mp_read_/*#~0~#*/(field) mp_read_ptr_/*#~0~#*/(copy_buffer(&field, sizeof(/*#~0~#*/)))
#define mp_read_byref_/*#~0~#*/(field) mp_read_ptr_/*#~0~#*/(&field)

/*#~2~#*/

static const mp_obj_dict_t mp_/*#~0~#*/_locals_dict;

static MP_DEFINE_CONST_OBJ_TYPE(
    mp_/*#~0~#*/_type,
    MP_QSTR_/*#~1~#*/,
    MP_TYPE_FLAG_NONE,
    make_new, make_new_lv_struct,
    binary_op, lv_struct_binary_op,
    subscr, lv_struct_subscr,/*#~3~#*/
    locals_dict, &mp_/*#~0~#*/_locals_dict,
    buffer, mp_blob_get_buffer,
    parent, &mp_lv_base_struct_type
);

