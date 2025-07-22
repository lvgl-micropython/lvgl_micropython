static void mp_/*#~0~#*/_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    mp_lv_struct_t *self = MP_OBJ_TO_PTR(self_in);
    GENMPY_UNUSED /*#~0~#*/ *data = (/*#~0~#*/ *)self->data;

    if (dest[0] == MP_OBJ_NULL) {
        // load attribute
        switch(attr)
        {
            /*#~1~#*/
            default: call_parent_methods(self_in, attr, dest); // fallback to locals_dict lookup
        }
    } else {
        if (dest[1])
        {
            // store attribute
            switch(attr)
            {
                /*#~2~#*/
                default: return;
            }

            dest[0] = MP_OBJ_NULL; // indicate success
        }
    }
}

