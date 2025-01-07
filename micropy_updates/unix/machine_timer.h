// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#include "py/obj.h"
#include "py/runtime.h"

#ifndef __MACHINE_TIMER_H__
    #define __MACHINE_TIMER_H__

    #define TIMER_COUNT    20

    typedef struct _machine_timer_obj_t {
        mp_obj_base_t base;
        uint8_t id;
        bool active;

        uint8_t repeat;
        uint16_t period;
        uint16_t ms_ticks;
        mp_obj_t callback;

    } machine_timer_obj_t;

    void machine_timer_deinit_all(void);

#endif // __MACHINE_TIMER_H__