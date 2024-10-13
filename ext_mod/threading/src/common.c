

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "../inc/common.h"


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
