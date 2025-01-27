#include "common/lcd_framebuf.h"

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/objarray.h"
#include "py/binary.h"


#define FB_UNUSED(x) ((void)x)

#ifdef ESP_IDF_VERSION
    #include "esp_heap_caps.h"

    #define MEM_FREE(obj)  heap_caps_free(obj)
    #define MEM_ALLOC(size, caps)  heap_caps_malloc(size, caps)

#else
    #include "py/misc.h"

    #define MEM_FREE(obj)  m_free(obj)
    #define MEM_ALLOC(size, caps) m_malloc(size); FB_UNUSED(caps)
#endif /* ESP_IDF_VERSION */




static mp_obj_t framebuf_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
     enum { ARG_size, ARG_caps };

    const mp_arg_t make_new_args[] = {
        { MP_QSTR_size, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_caps, MP_ARG_INT | MP_ARG_REQUIRED }
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(make_new_args)];
    mp_arg_parse_all_kw_array(
        n_args,
        n_kw,
        all_args,
        MP_ARRAY_SIZE(make_new_args),
        make_new_args,
        args
    );

    // create new object
    mp_lcd_framebuf_t *self = m_new_obj(mp_lcd_framebuf_t);
    self->base.type = &mp_lcd_framebuf_type;

    self->len = (size_t)args[ARG_size].u_int;
    self->caps = (uint32_t)args[ARG_caps].u_int;

    self->typecode = BYTEARRAY_TYPECODE | 0x80;
    self->items = MEM_ALLOC(self->len, self->caps);

    if (self->items == NULL) {
       mp_raise_msg_varg(
           &mp_type_MemoryError,
           MP_ERROR_TEXT("Not enough memory available (%lu)"),
           self->len
       );
       return mp_const_none;
    }

    return MP_OBJ_FROM_PTR(self);
}


static mp_obj_t framebuf_free(mp_obj_t self_in, mp_obj_t frame_buf)
{
    mp_lcd_framebuf_t *self = MP_OBJ_TO_PTR(self_in);

    if (self->items == NULL) {
        mp_raise_msg(&mp_type_MemoryError, MP_ERROR_TEXT("framebuffer has already been free'd"));
    } else {
        MEM_FREE(self->items);
        self->items = NULL;
        self->len = 0;
    }

    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(framebuf_free_obj, framebuf_free);


static void framebuf_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    if (dest[0] == MP_OBJ_NULL) {
        mp_map_t *locals_map = &MP_OBJ_TYPE_GET_SLOT(&mp_lcd_framebuf_type, locals_dict)->map;
        mp_map_elem_t *elem = mp_map_lookup(locals_map, MP_OBJ_NEW_QSTR(attr), MP_MAP_LOOKUP);
        if (elem != NULL) {
            mp_convert_member_lookup(self_in, &mp_lcd_framebuf_type, MP_OBJ_FROM_PTR(elem->value), dest);
        } else {
            locals_map = &MP_OBJ_TYPE_GET_SLOT(&mp_type_bytearray, locals_dict)->map;
            elem = mp_map_lookup(locals_map, MP_OBJ_NEW_QSTR(attr), MP_MAP_LOOKUP);
            if (elem != NULL) {
                mp_convert_member_lookup(self_in, &mp_type_bytearray, MP_OBJ_FROM_PTR(elem->value), dest);
            }
        }
    }
}


static mp_obj_t framebuf_iterator_new(mp_obj_t self_in, mp_obj_iter_buf_t *iter_buf)
{
    return ((mp_getiter_fun_t)MP_OBJ_TYPE_GET_SLOT(&mp_type_bytearray, iter))(self_in, iter_buf);
}


static mp_obj_t framebuf_unary_op(mp_unary_op_t op, mp_obj_t o_in)
{
    return ((mp_unary_op_fun_t)MP_OBJ_TYPE_GET_SLOT(&mp_type_bytearray, unary_op))(op, o_in);
}


static mp_obj_t framebuf_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in)
{
    return ((mp_binary_op_fun_t)MP_OBJ_TYPE_GET_SLOT(&mp_type_bytearray, binary_op))(op, lhs_in, rhs_in);
}


static mp_obj_t framebuf_subscr(mp_obj_t self_in, mp_obj_t index_in, mp_obj_t value)
{
    return ((mp_subscr_fun_t)MP_OBJ_TYPE_GET_SLOT(&mp_type_bytearray, subscr))(self_in, index_in, value);
}


static mp_int_t framebuf_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags)
{
    return ((mp_buffer_fun_t)MP_OBJ_TYPE_GET_SLOT(&mp_type_bytearray, buffer))(self_in, bufinfo, flags);
}


static const mp_rom_map_elem_t framebuf_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_free), MP_ROM_PTR(&framebuf_free_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&framebuf_free_obj) },
};


static MP_DEFINE_CONST_DICT(framebuf_locals_dict, framebuf_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    mp_lcd_framebuf_type,
    MP_QSTR_framebuffer,
    MP_TYPE_FLAG_EQ_CHECKS_OTHER_TYPE | MP_TYPE_FLAG_ITER_IS_GETITER,
    make_new, framebuf_make_new,
    iter, framebuf_iterator_new,
    unary_op, framebuf_unary_op,
    binary_op, framebuf_binary_op,
    subscr, framebuf_subscr,
    buffer, framebuf_get_buffer,
    attr, framebuf_attr,
    locals_dict, &framebuf_locals_dict,
    parent, &mp_type_bytearray
);
