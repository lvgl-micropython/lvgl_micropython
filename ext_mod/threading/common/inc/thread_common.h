// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#ifndef __COMMON_H__
    #define __COMMON_H__

    #define THREAD_UNUSED(x) ((void)x)

    #include "../../threading/threading_thread.h"

    typedef struct _threading_mutex_t {
        SemaphoreHandle_t handle;
        StaticSemaphore_t buffer;
    } threading_mutex_t;

    int lock_acquire(threading_mutex_t *mutex, int wait);
    void lock_release(threading_mutex_t *mutex);
    void lock_init(threading_mutex_t *mutex);

    int rlock_acquire(threading_mutex_t *mutex, int wait);
    void rlock_release(threading_mutex_t *mutex);
    void rlock_init(threading_mutex_t *mutex);

    typedef void *(*threading_thread_entry_cb_t)(mp_obj_thread_thread_t *);

    mp_uint_t thread_create(threading_thread_entry_cb_t entry, mp_obj_thread_thread_t *th);

    void threading_init(void *stack, uint32_t stack_len);
    void threading_deinit(void);
    void threading_gc_others(void);

    estern size_t thread_stack_size;

    void THREADING_FREERTOS_TASK_DELETE_HOOK(void *tcb);


#endif