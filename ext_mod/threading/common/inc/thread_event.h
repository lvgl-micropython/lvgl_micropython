// micropython includes
#include "py/obj.h"
#include "py/runtime.h"


#ifndef __THREAD_EVENT_H__
    #define __THREAD_EVENT_H__

    typedef struct _thread_event_t thread_event_t;

    typedef struct _mp_obj_thread_event_t {
        mp_obj_base_t base;
        thread_event_t event;

        bool is_set;

    } mp_obj_thread_event_t;


    void threading_event_set(thread_event_t *event);
    bool threading_event_isset(thread_event_t *event);
    void threading_event_clear(thread_event_t *event);
    void threading_event_wait(thread_event_t *event, int32_t wait_ms);
    void threading_event_init(thread_event_t *event);
    void threading_event_delete(thread_event_t *event);

#endif













