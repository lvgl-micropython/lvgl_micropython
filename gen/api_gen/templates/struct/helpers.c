

static void set_struct_attribute(mp_obj_t self, qstr field, mp_obj_t value)
{
    mp_obj_t dest[] = { MP_OBJ_SENTINEL, value };
    MP_OBJ_TYPE_GET_SLOT(self->base.type, attr)(self, field, dest);

    if (dest[0]) RAISE_AttributeError("Cannot set field %s on struct %s!", qstr_str(field), qstr_str(self->base.type->name));
}




static mp_lv_struct_t *mp_to_lv_struct(mp_obj_t mp_obj)
{
    if (mp_obj == NULL || mp_obj == mp_const_none) return NULL;

    mp_obj_t native_obj = get_native_obj(mp_obj);

    mp_make_new_fun_t make_new = MP_OBJ_TYPE_GET_SLOT_OR_NULL(mp_obj_get_type(native_obj), make_new);
    if (!MP_OBJ_IS_OBJ(native_obj) || make_new != &make_new_lv_struct) {
        RAISE_TypeError("Expected Struct object!");
    }

    mp_lv_struct_t *mp_lv_struct = MP_OBJ_TO_PTR(native_obj);

    return mp_lv_struct;
}


static size_t get_lv_struct_size(const mp_obj_type_t *type)
{
    mp_obj_dict_t *self = MP_OBJ_TO_PTR(MP_OBJ_TYPE_GET_SLOT(type, locals_dict));
    mp_map_elem_t *elem = mp_map_lookup(&self->map,
                                        MP_OBJ_NEW_QSTR(MP_QSTR___SIZE__),
                                        MP_MAP_LOOKUP);

    if (elem == NULL) return 0;
    else return (size_t)mp_obj_get_int(elem->value);
}

static mp_obj_t make_new_lv_struct(const mp_obj_type_t *type, size_t n_args,
                                   size_t n_kw, const mp_obj_t *args)
{
    mp_make_new_fun_t make_new = MP_OBJ_TYPE_GET_SLOT_OR_NULL(type, make_new);
    if (!MP_OBJ_IS_TYPE(type, &mp_type_type) || make_new != &make_new_lv_struct) {
        RAISE_TypeError("Argument is not a struct type!");
    }

    size_t size = get_lv_struct_size(type);
    mp_arg_check_num(n_args, n_kw, 0, 1, false);

    mp_lv_struct_t *self = m_new_obj(mp_lv_struct_t);

    mp_lv_struct_t *other = NULL;
    size_t count = 1;
    if (n_args > 0 && !mp_obj_is_int(args[0]) {
        other = mp_to_lv_struct(cast(args[0], type));
        count = mp_obj_get_int(args[0])
    }

    void *data = NULL;

    if (size != 0) {
        if (other == NULL || other->data == NULL) {
            data = m_malloc(size * count);
            data = memset(data, 0, size * count;
        } else {
            data = m_malloc(size * count);
            memcpy(data, other->data, size * count);
        }
    }

    *self = (mp_lv_struct_t){ .base = { type }, .data = data };

    return MP_OBJ_FROM_PTR(self);
}


static mp_obj_t lv_struct_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in)
{
    mp_lv_struct_t *lhs = MP_OBJ_TO_PTR(lhs_in);
    mp_lv_struct_t *rhs = MP_OBJ_TO_PTR(rhs_in);

    switch (op)
    {
        case MP_BINARY_OP_EQUAL:
            return mp_obj_new_bool(lhs->data == rhs->data);
        case MP_BINARY_OP_NOT_EQUAL:
            return mp_obj_new_bool(lhs->data != rhs->data);
        default:
            return MP_OBJ_NULL;
    }
}


static mp_obj_t lv_struct_subscr(mp_obj_t self_in, mp_obj_t index, mp_obj_t value)
{
    mp_lv_struct_t *self = mp_to_lv_struct(self_in);
    if ((!self) || (!self->data)) return NULL;

    if (!mp_obj_is_int(index)) RAISE_IndexError("Subscript index must be an integer!");

    const mp_obj_type_t *type = mp_obj_get_type(self_in);

    size_t element_size = get_lv_struct_size(type);
    if (element_size == 0) return mp_const_none;

    size_t element_index = mp_obj_get_int(index);
    void *element_addr = (byte*)self->data + element_size*element_index;

    if (value == MP_OBJ_NULL) {
        memset(element_addr, 0, element_size);
        return self_in;
    }

    mp_lv_struct_t *element_at_index = m_new_obj(mp_lv_struct_t);
    *element_at_index = (mp_lv_struct_t){
        .base = { type },
        .data = element_addr
    };

    if (value != MP_OBJ_SENTINEL) {
        mp_lv_struct_t *other = mp_to_lv_struct(cast(value, type));
        if (!other || !other->data) return NULL;

        memcpy(element_at_index->data, other->data, element_size);
    }

    return MP_OBJ_FROM_PTR(element_at_index);
}


// Reference an existing lv struct (or part of it)
static mp_obj_t lv_to_mp_struct(const mp_obj_type_t *type, void *lv_struct)
{
    if (lv_struct == NULL) return mp_const_none;

    mp_lv_struct_t *self = m_new_obj(mp_lv_struct_t);
    *self = (mp_lv_struct_t){
        .base = { type },
        .data = lv_struct
    };

    return MP_OBJ_FROM_PTR(self);
}


// Convert dict to struct
static mp_obj_t dict_to_struct(mp_obj_t dict, const mp_obj_type_t *type)
{
    mp_obj_t mp_struct = make_new_lv_struct(type, 0, 0, NULL);
    mp_obj_t native_dict = cast(dict, &mp_type_dict);
    mp_map_t *map = mp_obj_dict_get_map(native_dict);

    if (map == NULL) return mp_const_none;

    for (uint i = 0; i < map->alloc; i++) {
        mp_obj_t key = map->table[i].key;
        mp_obj_t value = map->table[i].value;

        if (key != MP_OBJ_NULL) {
            mp_obj_t dest[] = {MP_OBJ_SENTINEL, value};
            MP_OBJ_TYPE_GET_SLOT(type, attr)(mp_struct, mp_obj_str_get_qstr(key), dest);

            if (dest[0]) {
                RAISE_AttributeError("Cannot set field %s on struct %s!",
                                     qstr_str(mp_obj_str_get_qstr(key)),
                                     qstr_str(type->name));
            }
        }
    }

    return mp_struct;
}