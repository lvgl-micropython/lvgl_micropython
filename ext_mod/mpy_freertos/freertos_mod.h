#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/atomic.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/stream_buffer.h"
#include "freertos/message_buffer.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#ifndef __FREERTOS_MOD_H__
    #define __FREERTOS_MOD_H__

    typedef enum {
        mp_freertos_event_group_type = 1,
        mp_freertos_queue_type = 2,
        mp_freertos_queue_set_type = 3,
        mp_freertos_semaphore_type = 4,
        mp_freertos_task_type = 5,
        mp_freertos_timer_type = 6,
        mp_freertos_spinlock_type = 7,
        mp_freertos_message_buffer_type = 8,
        mp_freertos_stream_buffer_type = 9
    } mp_freertos_types;

#endif