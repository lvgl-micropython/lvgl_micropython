#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "../inc/common.h"

#ifndef __MULTIPROCESS_RLOCK_H__
    #define __MULTIPROCESS_RLOCK_H__

    typedef struct _mp_obj_process_rlock_t {
        mp_obj_base_t base;
        mp_mutex_t mutex;
        volatile bool locked;
        volatile int count;
    } mp_obj_process_rlock_t;

#endif