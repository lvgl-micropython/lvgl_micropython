// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "thread_common.h"

#ifndef __MULTIPROCESS_H__
    #define __MULTIPROCESS_H__

    void multiprocessing_init(void);
    extern mp_obj_t processes[2];

#endif
