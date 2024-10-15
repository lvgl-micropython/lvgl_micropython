// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "thread_common.h"
#include "../../threading/threading_thread.h"


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
static threading_mutex_t thread_mutex;
static mp_obj_thread_thread_t thread_entry0;
static mp_obj_thread_thread_t *thread = NULL; // root pointer, handled by threading_gc_others
size_t thread_stack_size = 0;


void threading_init(void *stack, uint32_t stack_len) {
    mp_thread_set_state(&mp_state_ctx.thread);
    // create the first entry in the linked list of all threads
    thread_entry0.id = xTaskGetCurrentTaskHandle();

    thread_entry0.ident = mp_obj_new_int_from_uint((mp_uint_t)thread_entry0.id);
    thread_entry0.ready = 1;
    thread_entry0.running = true;
    thread_entry0.arg = NULL;
    thread_entry0.stack = stack;
    thread_entry0.stack_len = stack_len;
    thread_entry0.next = NULL;
    mutex_init(&thread_mutex);

    // memory barrier to ensure above data is committed
    __sync_synchronize();

    // FREERTOS_TASK_DELETE_HOOK needs the thread ready after thread_mutex is ready
    thread = &thread_entry0;
}


void threading_deinit(void) {
    for (;;) {
        // Find a task to delete
        TaskHandle_t id = NULL;
        mutex_lock(&thread_mutex, 1);
        for (mp_obj_thread_thread_t *th = thread; th != NULL; th = th->next) {
            // Don't delete the current task
            if (th->id != xTaskGetCurrentTaskHandle()) {
                id = th->id;
                break;
            }
        }
        mutex_unlock(&thread_mutex);

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
    mutex_lock(&thread_mutex, 1);
    for (mp_obj_thread_thread_t *th = thread; th != NULL; th = th->next) {
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
    mutex_unlock(&thread_mutex);
}


static void threading_freertos_entry(void *arg) {
    if (ext_threading_thread_entry) {
        mp_obj_thread_thread_t * th = (mp_obj_thread_thread_t *)arg;
        ext_threading_thread_entry(th);
    }
    vTaskDelete(NULL);
    for (;;) {;
    }
}


mp_uint_t thread_create_ex(threading_thread_entry_cb_t entry, mp_obj_thread_thread_t *th, int priority, char *name) {
    // store thread entry function into a global variable so we can access it
    ext_threading_thread_entry = entry;

    if (th->call_args->stack_size == 0) {
        th->call_args->stack_size = THREADING_DEFAULT_STACK_SIZE; // default stack size
    } else if (th->call_args->stack_size < THREADING_MIN_STACK_SIZE) {
        th->call_args->stack_size = THREADING_MIN_STACK_SIZE; // minimum stack size
    }

    // Allocate linked-list node (must be outside thread_mutex lock)

    mutex_lock(&thread_mutex, 1);

    BaseType_t core_id = xPortGetCoreID();
    // create thread
    BaseType_t result = xTaskCreatePinnedToCore(threading_freertos_entry, name, *stack_size / sizeof(StackType_t), th, priority, &th->id, core_id);

    if (result != pdPASS) {
        mutex_unlock(&thread_mutex);
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("can't create thread"));
    }

    // add thread to linked list of all threads
    th->ready = 0;
    th->arg = th->call_args;
    th->stack = pxTaskGetStackStart(th->id);
    th->stack_len = th->call_args->stack_size / sizeof(uintptr_t);
    th->next = thread;
    thread = th;

    // adjust the stack_size to provide room to recover from hitting the limit
    th->call_args->stack_size -= 1024;

    mutex_unlock(&thread_mutex);

    return (mp_uint_t)th->id;
}


mp_uint_t thread_create(threading_thread_entry_cb_t entry, mp_obj_thread_thread_t *th) {
    return thread_create_ex(entry, th, MP_THREAD_PRIORITY, "mp_thread");
}


void THREADING_FREERTOS_TASK_DELETE_HOOK(void *tcb) {
    if (thread == NULL) {
        // threading not yet initialised
        return;
    }
    mp_obj_thread_thread_t *prev = NULL;
    mutex_lock(&thread_mutex, 1);
    for (mp_obj_thread_thread_t *th = thread; th != NULL; prev = th, th = th->next) {
        if ((void *)th->id == tcb) {
            if (prev != NULL) {
                prev->next = th->next;
            } else {
                thread = th->next;
            }
            break;
        }
    }

    mutex_unlock(&thread_mutex);
}
