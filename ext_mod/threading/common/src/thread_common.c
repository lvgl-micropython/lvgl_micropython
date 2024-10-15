// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/gc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "thread_common.h"
#include "threading_thread.h"

/*
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/stackctrl.h"
#include "py/mpthread.h"
#include "mpthreadport.h"

#include "stdio.h"
*/

int lock_acquire(threading_mutex_t *mutex, int wait)
{
    return pdTRUE == xSemaphoreTake(mutex->handle, wait < 0 ? portMAX_DELAY : pdMS_TO_TICKS(wait));
}


void lock_release(threading_mutex_t *mutex)
{
    xSemaphoreGive(mutex->handle);
}


void lock_init(threading_mutex_t *mutex)
{
    // Need a binary semaphore so a lock can be acquired on one Python thread
    // and then released on another.
    mutex->handle = xSemaphoreCreateBinaryStatic(&mutex->buffer);
    xSemaphoreGive(mutex->handle);
}


// configUSE_RECURSIVE_MUTEXES must be set to 1 in FreeRTOSConfig.h for this macro to be available.

int rlock_acquire(threading_mutex_t *mutex, int wait)
{
    return pdTRUE == xSemaphoreTakeRecursive(mutex->handle, wait < 0 ? portMAX_DELAY : pdMS_TO_TICKS(wait));
}


void rlock_release(threading_mutex_t *mutex)
{
    xSemaphoreGiveRecursive(mutex->handle);
}


void rlock_init(threading_mutex_t *mutex)
{
    // Need a binary semaphore so a lock can be acquired on one Python thread
    // and then released on another.
    mutex->handle = xSemaphoreCreateRecursiveMutexStatic(&mutex->buffer);
    xSemaphoreGiveRecursive(mutex->handle);
}


// the mutex controls access to the linked list
static threading_thread_entry_cb_t ext_threading_thread_entry = NULL;
threading_mutex_t t_mutex;
mp_obj_thread_thread_t _main_thread;
mp_obj_thread_thread_t *t_thread = NULL; // root pointer, handled by threading_gc_others
size_t thread_stack_size = 0;


void threading_init(void *stack, uint32_t stack_len) {
    mp_thread_set_state(&mp_state_ctx.thread);
    // create the first entry in the linked list of all threads
    _main_thread.id = xTaskGetCurrentTaskHandle();
    _main_thread.core_id = (uint8_t)xTaskGetCoreID(xTaskGetCurrentTaskHandle());
    _main_thread.ident = mp_obj_new_int_from_uint((mp_uint_t)_main_thread.id);
    _main_thread.ready = 1;
    _main_thread.is_alive = true;
    _main_thread.arg = NULL;
    _main_thread.stack = stack;
    _main_thread.stack_len = stack_len;
    _main_thread.next = NULL;
    lock_init(&t_mutex);

    // memory barrier to ensure above data is committed
    __sync_synchronize();

    // FREERTOS_TASK_DELETE_HOOK needs the thread ready after t_mutex is ready
    t_thread = &_main_thread;
}


void threading_deinit(void) {
    for (;;) {
        // Find a task to delete
        TaskHandle_t id = NULL;
        lock_acquire(&t_mutex, 1);
        for (mp_obj_thread_thread_t *th = t_thread; th != NULL; th = th->next) {
            // Don't delete the current task
            if (th->id != xTaskGetCurrentTaskHandle()) {
                id = th->id;
                break;
            }
        }
        lock_release(&t_mutex);

        if (id == NULL) {
            // No tasks left to delete
            break;
        } else {
            // Call FreeRTOS to delete the task (it will call FREERTOS_TASK_DELETE_HOOK)
            vTaskDelete(id);
        }
    }
}



void threading_gc_others(void) {
    lock_acquire(&t_mutex, 1);
    for (mp_obj_thread_thread_t *th = t_thread; th != NULL; th = th->next) {
        gc_collect_root((void **)&th, 1);
        gc_collect_root(&th->arg, 1); // probably not needed
        if (th->id == xTaskGetCurrentTaskHandle()) {
            continue;
        }
        if (!th->ready) {
            continue;
        }
        gc_collect_root(th->stack, th->stack_len);
    }
    lock_release(&t_mutex);
}


static void threading_freertos_entry(void *arg) {
    if (ext_threading_thread_entry) {
        mp_obj_thread_thread_t * self = (mp_obj_thread_thread_t *)arg;
        ext_threading_thread_entry(self);
    }
    vTaskDelete(NULL);
    for (;;) {;
    }
}


mp_uint_t thread_create_ex(threading_thread_entry_cb_t entry, mp_obj_thread_thread_t *self, int priority, char *name) {
    // store thread entry function into a global variable so we can access it
    ext_threading_thread_entry = entry;

    if (self->call_args->stack_size == 0) {
        self->call_args->stack_size = THREADING_DEFAULT_STACK_SIZE; // default stack size
    } else if (self->call_args->stack_size < THREADING_MIN_STACK_SIZE) {
        self->call_args->stack_size = THREADING_MIN_STACK_SIZE; // minimum stack size
    }

    // Allocate linked-list node (must be outside t_mutex lock)

    lock_acquire(&t_mutex, 1);

    // create thread
    BaseType_t result = xTaskCreatePinnedToCore(threading_freertos_entry, name, self->call_args->stack_size / sizeof(StackType_t), self, priority, &self->id, self->core_id);

    if (result != pdPASS) {
        lock_release(&t_mutex);
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("can't create thread"));
    }

    // add thread to linked list of all threads
    self->ready = 0;
    self->stack = pxTaskGetStackStart(self->id);
    self->stack_len = self->call_args->stack_size / sizeof(uintptr_t);
    self->next = t_thread;
    t_thread = self;

    // adjust the stack_size to provide room to recover from hitting the limit
    self->call_args->stack_size -= 1024;

    lock_release(&t_mutex);

    return (mp_uint_t)self->id;
}


mp_uint_t thread_create(threading_thread_entry_cb_t entry, mp_obj_thread_thread_t *self) {
    return thread_create_ex(entry, self,THREADING_PRIORITY, "mp_thread");
}


void THREADING_FREERTOS_TASK_DELETE_HOOK(void *tcb) {
    if (t_thread == NULL) {
        // threading not yet initialised
        return;
    }
    mp_obj_thread_thread_t *prev = NULL;
    lock_acquire(&t_mutex, 1);
    for (mp_obj_thread_thread_t *th = t_thread; th != NULL; prev = th, th = th->next) {
        if ((void *)th->id == tcb) {
            if (prev != NULL) {
                prev->next = th->next;
            } else {
                t_thread = th->next;
            }
            break;
        }
    }

    lock_release(&t_mutex);
}
