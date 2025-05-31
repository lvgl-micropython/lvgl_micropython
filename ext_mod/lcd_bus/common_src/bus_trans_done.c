


bool bus_trans_done_cb(void *user_ctx)
{
    mp_lcd_bus_obj_t *self = (mp_lcd_bus_obj_t *)user_ctx;

    if (self->callback != mp_const_none && mp_obj_is_callable(self->callback)) {
        mp_call_function_n_kw(self->callback, 0, 0, NULL);
    }

    self->trans_done = true;
    return false;
}