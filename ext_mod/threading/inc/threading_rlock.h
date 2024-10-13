#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "../inc/common.h"

#ifndef __THREADING_RLOCK_H__
    #define __THREADING_RLOCK_H__

    typedef struct _mp_obj_thread_rlock_t {
        mp_obj_base_t base;
        threading_mutex_t mutex;
        volatile bool locked;
        volatile int count;
    } mp_obj_thread_rlock_t;


    extern const mp_obj_type_t mp_type_threading_rlock_t;

#endif