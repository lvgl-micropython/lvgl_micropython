// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#ifndef __THREADING_H__
    #define __THREADING_H__

    #include "thread_thread.h"
    #include "thread_lock.h"
    #include "thread_rlock.h"
    #include "thread_semphamore.h"
    #include "thread_event.h"

    #define THREAD_UNUSED(x) ((void)x)

    void threading_init(void *stack, uint32_t stack_len)
    void threading_deinit(void)
    void threading_gc_others(void)

    typedef void *(*thread_entry_cb_t)(mp_obj_thread_t *self);

    mp_obj_t mp_get_main_thread(void);
    mp_obj_t mp_enumerate_threads(void);

    uint32_t mp_get_current_thread_id(void)

    extern size_t thread_stack_size;
    extern mp_obj_thread_t *t_thread;
    extern thread_lock_t t_mutex;
    extern mp_obj_thread_t _main_thread;

#endif /*__THREADING_H__ */
