// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#ifndef __THREADING_H__
    #define __THREADING_H__

    #define THREAD_UNUSED(x) ((void)x)

    #include "thread_thread.h"
    #include "thread_lock.h"
    #include "thread_rlock.h"
    #include "thread_semphamore.h"
    #include "thread_event.h"

    void threading_init(void *stack, uint32_t stack_len); // needs to be defined in port
    void threading_deinit(void); // needs to be defined in port
    void threading_gc_others(void); // needs to be defined in port

    typedef void *(*thread_entry_cb_t)(mp_obj_thread_t *self);

    mp_obj_t mp_get_main_thread(void); // needs to be defined in port
    mp_obj_t mp_enumerate_threads(void); // needs to be defined in port
    uint32_t mp_get_current_thread_id(void); // needs to be defined in port

    extern size_t thread_stack_size; // needs to be defined in port
    extern mp_obj_thread_t *t_thread; // needs to be defined in port
    extern thread_lock_t t_mutex; // needs to be defined in port
    extern mp_obj_thread_t _main_thread; // needs to be defined in port

#endif /*__THREADING_H__ */
