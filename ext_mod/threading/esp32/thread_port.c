// micropython includes
#include "py/obj.h"
#include "py/runtime.h"
#include "py/gc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "thread_port.h"

#include "threading.h"
#include "multiprocessing.h"

#include "thread_thread.h"
#include "thread_event.h"
#include "thread_semaphore.h"
#include "thread_lock.h"
#include "thread_rlock.h"

#define THREADING_MIN_STACK_SIZE                        (4 * 1024)
#define THREADING_DEFAULT_STACK_SIZE                    (THREADING_MIN_STACK_SIZE + 1024)
#define THREADING_PRIORITY                              (ESP_TASK_PRIO_MIN + 1)


uint8_t mp_get_cpu_count(void)
{
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32P4  // dual core
    return 2;
#elif CONFIG_IDF_TARGET_ESP32C2 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C5 || CONFIG_IDF_TARGET_ESP32C6 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32H2
    return 1;
#else
    return 1;
#endif
}


uint8_t mp_get_process_core(thread_t *thread)
{
    return (uint8_t)xTaskGetCoreID(thread->handle);
}


uint8_t mp_get_current_process_core(void)
{
    return (uint8_t)xTaskGetCoreID(xTaskGetCurrentTaskHandle());
}


uint32_t mp_get_current_thread_id(void)
{
    return (uint32_t)xTaskGetCurrentTaskHandle();
}


void threading_event_set(thread_event_t *event)
{
    xEventGroupSetBits(event->handle, 0);
}


bool threading_event_isset(thread_event_t *event)
{
    return (bool)(xEventGroupGetBits(event->handle) == 1);
}


void threading_event_clear(thread_event_t *event)
{
    xEventGroupClearBits(event->handle, 0);
}


void threading_event_wait(thread_event_t *event, int32_t wait_ms)
{
    xEventGroupWaitBits(event->handle, 0, pdFALSE, pdFALSE, wait_ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS((uint16_t)wait_ms));
}


void threading_event_init(thread_event_t *event)
{
    event->handle = xEventGroupCreateStatic(event->buffer);
}


void threading_event_delete(thread_event_t *event)
{
    xEventGroupSetBits(event->handle, 0);
    vEventGroupDelete(event->handle);
}


int threading_lock_acquire(thread_lock_t *lock, int32_t wait_ms)
{
    return pdTRUE == xSemaphoreTake(lock->handle, wait_ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS((uint16_t)wait_ms));
}


void threading_lock_release(thread_lock_t *lock)
{
    xSemaphoreGive(lock->handle);
}


void threading_lock_init(thread_lock_t *lock)
{
    mutex->handle = xSemaphoreCreateBinaryStatic(&lock->buffer);
    xSemaphoreGive(lock->handle);
}


int threading_rlock_acquire(thread_rlock_t *rlock, uint16_t wait_ms)
{
    return pdTRUE == xSemaphoreTakeRecursive(rlock->handle, wait < 0 ? portMAX_DELAY : pdMS_TO_TICKS(wait_ms));
}


void threading_rlock_release(thread_rlock_t *rlock)
{
    xSemaphoreGiveRecursive(rlock->handle);
}


void threading_rlock_init(thread_rlock_t *rlock)
{
    mutex->handle = xSemaphoreCreateRecursiveMutexStatic(&rlock->buffer);
    xSemaphoreGiveRecursive(rlock->handle);
}


uint16_t threading_semaphore_get_count(thread_semaphore_t *sem)
{
    return (uint16_t)uxSemaphoreGetCount(sem->handle);
}


bool threading_semaphore_acquire(thread_semaphore_t *sem, int32_t wait_ms)
{
    return (bool)(pdTRUE == xSemaphoreTake(sem->handle, wait_ms < 0 ? portMAX_DELAY : pdMS_TO_TICKS((uint16_t)wait_ms)));
}


void threading_semaphore_release(thread_semaphore_t *sem)
{
    xSemaphoreGive(sem->handle);
}


void threading_semaphore_init(thread_semaphore_t *sem, uint16_t start_value)
{
    sem->handle = xSemaphoreCreateCountingStatic(start_value, start_value, &sem->buffer);
}


void threading_semaphore_delete(thread_semaphore_t *sem)
{
    vSemaphoreDelete(sem->handle);
}


static threading_thread_entry_cb_t ext_threading_thread_entry = NULL;


thread_lock_t t_mutex;
mp_obj_thread_t _main_thread;
mp_obj_thread_t *t_thread = NULL; // root pointer, handled by threading_gc_others
size_t thread_stack_size = 0;


void *thread_entry_cb(mp_obj_thread_t *self)
{
    // Execution begins here for a new thread.  We do not have the GIL.

    mp_state_thread_t ts;
    mp_thread_init_state(&ts, self->call_args->stack_size, self->call_args->dict_locals, self->call_args->dict_globals);

#if MICROPY_ENABLE_PYSTACK
    // TODO threading and pystack is not fully supported, for now just make a small stack
    mp_obj_t mini_pystack[128];
    mp_pystack_init(mini_pystack, &mini_pystack[128]);
#endif

    // signal that we are set up and running
    self->ready = 1;
    self->is_alive = true;

    // TODO set more thread-specific state here:
    //  cur_exception (root pointer)

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_call_function_n_kw(self->call_args->fun, self->call_args->n_args, self->call_args->n_kw, self->call_args->args);
        nlr_pop();
    } else {
        // uncaught exception
        // check for SystemExit
        mp_obj_base_t *exc = (mp_obj_base_t *)nlr.ret_val;
        if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(exc->type), MP_OBJ_FROM_PTR(&mp_type_SystemExit))) {
            // swallow exception silently
        } else {
            // print exception out
            mp_printf(MICROPY_ERROR_PRINTER, "Unhandled exception in thread started by ");
            mp_obj_print_helper(MICROPY_ERROR_PRINTER, self->call_args->fun, PRINT_REPR);
            mp_printf(MICROPY_ERROR_PRINTER, "\n");
            mp_obj_print_exception(MICROPY_ERROR_PRINTER, MP_OBJ_FROM_PTR(exc));
        }
    }

    // signal that we are finished

    self->is_alive = false;
    self->ready = 0;
    return NULL;
}



void threading_init(void *stack, uint32_t stack_len)
{
    mp_thread_set_state(&mp_state_ctx.thread);
    // create the first entry in the linked list of all threads
    _main_thread.thread.handle = xTaskGetCurrentTaskHandle();
    _main_thread.core_id = mp_get_current_process_core();
    _main_thread.ident = mp_obj_new_int_from_uint((mp_uint_t)_main_thread.thread.handle);
    _main_thread.ready = 1;
    _main_thread.is_alive = true;
    _main_thread.arg = NULL;
    _main_thread.stack = stack;
    _main_thread.stack_len = stack_len;
    _main_thread.next = NULL;
    threading_lock_init(&t_mutex);

    // memory barrier to ensure above data is committed
    __sync_synchronize();

    // FREERTOS_TASK_DELETE_HOOK needs the thread ready after t_mutex is ready
    t_thread = &_main_thread;
}


void threading_deinit(void)
{
    for (;;) {
        // Find a task to delete
        
        thread_t thread = { .handle=NULL };
        threading_lock_acquire(&t_mutex, 1);
        
        for (mp_obj_thread_t *th = t_thread; th != NULL; th = th->next) {
            // Don't delete the current task
            if (th->thread.handle != xTaskGetCurrentTaskHandle()) {
                thread.handle = th->thread.handle;
                break;
            }
        }
        threading_lock_release(&t_mutex);

        if (thread.handle == NULL) {
            // No tasks left to delete
            break;
        } else {
            // Call FreeRTOS to delete the task (it will call FREERTOS_TASK_DELETE_HOOK)
            
            threading_delete_thread(&thread);
        }
    }
}


void threading_gc_others(void)
{
    threading_lock_acquire(&t_mutex, 1);
    
    for (mp_obj_thread_t *th = t_thread; th != NULL; th = th->next) {
        gc_collect_root((void **)&th, 1);
        gc_collect_root(&th->arg, 1); // probably not needed
        if (th->thread.handle == xTaskGetCurrentTaskHandle()) {
            continue;
        }
        if (!th->ready) {
            continue;
        }
        gc_collect_root(th->stack, th->stack_len);
    }
    threading_lock_release(&t_mutex);
}


static void freertos_entry(void *arg)
{
    if (ext_threading_thread_entry) {
        mp_obj_thread_t *self = (mp_obj_thread_t *)arg;
        ext_threading_thread_entry(self);
    }

    vTaskDelete(NULL);
    for (;;) {;
    }
}


mp_uint_t thread_create_ex(mp_obj_thread_t *self, int priority, char *name)
{
    // store thread entry function into a global variable so we can access it
    ext_threading_thread_entry = thread_entry_cb;

    if (self->call_args->stack_size == 0) {
        self->call_args->stack_size = THREADING_DEFAULT_STACK_SIZE; // default stack size
    } else if (self->call_args->stack_size < THREADING_MIN_STACK_SIZE) {
        self->call_args->stack_size = THREADING_MIN_STACK_SIZE; // minimum stack size
    }

    // Allocate linked-list node (must be outside t_mutex lock)

    threading_lock_acquire(&t_mutex, 1);

    // create thread
    BaseType_t result = xTaskCreatePinnedToCore(freertos_entry, name, self->call_args->stack_size / sizeof(StackType_t), self, priority, &self->thread.handle, self->core_id);

    if (result != pdPASS) {
        threading_lock_release(&t_mutex);
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("can't create thread"));
    }

    // add thread to linked list of all threads
    self->ready = 0;
    self->stack = pxTaskGetStackStart(self->thread.handle);
    self->stack_len = self->call_args->stack_size / sizeof(uintptr_t);
    self->next = t_thread;
    t_thread = self;

    // adjust the stack_size to provide room to recover from hitting the limit
    self->call_args->stack_size -= 1024;

    threading_lock_release(&t_mutex);

    return (mp_uint_t)self->id;
}


mp_uint_t threading_create_thread(thread_t *self)
{
    return thread_create_ex(self, THREADING_PRIORITY, "mp_thread");
}

void threading_delete_thread(thread_t *thread) 
{
    vTaskDelete(thread->handle);
}
    

void THREADING_FREERTOS_TASK_DELETE_HOOK(void *tcb) {
    if (t_thread == NULL) {
        // threading not yet initialised
        return;
    }
    mp_obj_thread_t *prev = NULL;
    threading_lock_acquire(&t_mutex, 1);

    for (mp_obj_thread_t *th = t_thread; th != NULL; prev = th, th = th->next) {
        if ((void *)th->thread.handle == tcb) {
            if (prev != NULL) {
                prev->next = th->next;
            } else {
                t_thread = th->next;
            }
            break;
        }
    }

    threading_lock_release(&t_mutex);
}
