// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "py/mpconfig.h"

#include "threading_thread.h"

#ifndef __THREADING_H__
    #define __THREADING_H__

    mp_obj_t threading_main_thread(void);
    mp_obj_t threading_enumerate(void);

    extern size_t thread_stack_size;

#endif // __THREADING_H__
