// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#ifndef __THREAD_COMMON_H__
    #define __THREAD_COMMON_H__

    #include "esp_task.h"

    #define THREAD_UNUSED(x) ((void)x)
    #define THREADING_MIN_STACK_SIZE                        (4 * 1024)
    #define THREADING_DEFAULT_STACK_SIZE                    (THREADING_MIN_STACK_SIZE + 1024)
    #define THREADING_PRIORITY                              (ESP_TASK_PRIO_MIN + 1)

    #include "threading_thread.h"

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

    typedef void *(*threading_thread_entry_cb_t)(mp_obj_thread_thread_t *self);

    mp_uint_t thread_create(threading_thread_entry_cb_t entry, mp_obj_thread_thread_t *self);

    void threading_init(void *stack, uint32_t stack_len);
    void threading_deinit(void);
    void threading_gc_others(void);

    extern size_t thread_stack_size;
    extern mp_obj_thread_thread_t *t_thread;
    extern threading_mutex_t t_mutex;
    extern mp_obj_thread_thread_t _main_thread;

    void THREADING_FREERTOS_TASK_DELETE_HOOK(void *tcb);


#endif