

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "../inc/common.h"
#include "threading.h"


#ifndef __MULTIPROCESS_H__
    #define __MULTIPROCESS_H__

    typedef struct _mp_obj_process_t {
        mp_obj_base_t base;
        TaskHandle_t id;
        TaskHandle_t **threads;
        uint8_t num_threads;
    } mp_obj_process_t;

   extern mp_obj_process_t *mp_processes[2];

#endif
