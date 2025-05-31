// Copyright (c) 2024 - 2025 Kevin G. Schlosser

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

// esp includes
#include "esp_lcd_panel_io.h"

#ifndef __BUS_TRANS_DONE_H__
    #define __BUS_TRANS_DONE_H__

    void cb_isr(mp_obj_t cb);
    bool bus_trans_done_cb(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx);

#endif