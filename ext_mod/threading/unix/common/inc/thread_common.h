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


    // Some platforms don't have SIGRTMIN but if we do have it, use it to avoid
    // potential conflict with other uses of the more commonly used SIGUSR1.
    #ifdef SIGRTMIN
        #define THREADING_GC_SIGNAL (SIGRTMIN + 5)
    #else
        #define THREADING_GC_SIGNAL (SIGUSR1)
    #endif

    #define THREAD_UNUSED(x) ((void)x)

    #define THREAD_STACK_OVERFLOW_MARGIN (8192)





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




    // new
    void threading_init(void *stack, uint32_t stack_len);
    void threading_deinit(void);
    void threading_gc_others(void);


    void thread_begin_atomic_section(void);
    void thread_end_atomic_section(void);

    extern mp_obj_thread_thread_t *t_thread;
    extern threading_mutex_t t_mutex;
    extern mp_obj_thread_thread_t _main_thread;
    extern size_t thread_stack_size;


    extern pthread_key_t thread_tls_key;

    #if defined(__APPLE__)
        extern char threading_signal_done_name[25];
        extern sem_t *threading_signal_done_p;
    #else
        extern sem_t threading_signal_done;
    #endif


    // *************

#endif