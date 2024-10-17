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

#include <pthread.h>


void thread_begin_atomic_section(void) {
    pthread_mutex_lock(&thread_mutex);
}

void thread_end_atomic_section(void) {
    pthread_mutex_unlock(&thread_mutex);
}

/* Create a new thread, starting with execution of START-ROUTINE
   getting passed ARG.  Creation attributed come from ATTR.  The new
   handle is stored in *NEWTHREAD.  */
extern int pthread_create (pthread_t *__restrict __newthread, const pthread_attr_t *__restrict __attr, void *(*__start_routine) (void *), void *__restrict __arg)

/* Terminate calling thread.
   The registered cleanup handlers are called via exception handling
   so we cannot mark this function with __THROW.*/
extern void pthread_exit (void *__retval)

/* Obtain the identifier of the current thread.  */
extern pthread_t pthread_self (void)

/* Indicate that the thread TH is never to be joined with PTHREAD_JOIN.
   The resources of TH will therefore be freed immediately when it
   terminates, instead of waiting for another thread to perform PTHREAD_JOIN
   on it.  */
extern int pthread_detach (pthread_t __th)

/* Compare two thread identifiers.  */
extern int pthread_equal (pthread_t __thread1, pthread_t __thread2)

/* Limit specified thread TH to run only on the processors represented in CPUSET.  */
extern int pthread_setaffinity_np (pthread_t __th, size_t __cpusetsize, const cpu_set_t *__cpuset)

/* Get bit set in CPUSET representing the processors TH can run on.  */
extern int pthread_getaffinity_np (pthread_t __th, size_t __cpusetsize, cpu_set_t *__cpuset)

/* Cancel THREAD immediately or at the next possibility.  */
extern int pthread_cancel (pthread_t __th);





/* Initialize a mutex.  */
extern int pthread_mutex_init (pthread_mutex_t *__mutex, const pthread_mutexattr_t *__mutexattr)

/* Destroy a mutex.  */
extern int pthread_mutex_destroy (pthread_mutex_t *__mutex)

/* Try locking a mutex.  */
extern int pthread_mutex_trylock (pthread_mutex_t *__mutex)

/* Lock a mutex.  */
extern int pthread_mutex_lock (pthread_mutex_t *__mutex)

/* Wait until lock becomes available, or specified time passes. */
extern int pthread_mutex_timedlock (pthread_mutex_t *__restrict __mutex, const struct timespec *__restrict __abstime)

/* Unlock a mutex.  */
extern int pthread_mutex_unlock (pthread_mutex_t *__mutex)




/* Initialize mutex attribute object ATTR with default attributes
   (kind is PTHREAD_MUTEX_TIMED_NP).  */
extern int pthread_mutexattr_init (pthread_mutexattr_t *__attr)

/* Destroy mutex attribute object ATTR.  */
extern int pthread_mutexattr_destroy (pthread_mutexattr_t *__attr)

/* Get the process-shared flag of the mutex attribute ATTR.  */
extern int pthread_mutexattr_getpshared (const pthread_mutexattr_t * __restrict __attr, int *__restrict __pshared)

/* Set the process-shared flag of the mutex attribute ATTR.  */
extern int pthread_mutexattr_setpshared (pthread_mutexattr_t *__attr, int __pshared)

/* Return in *KIND the mutex kind attribute in *ATTR.  */
extern int pthread_mutexattr_gettype (const pthread_mutexattr_t *__restrict __attr, int *__restrict __kind)

/* Set the mutex kind attribute in *ATTR to KIND (either PTHREAD_MUTEX_NORMAL,
   PTHREAD_MUTEX_RECURSIVE, PTHREAD_MUTEX_ERRORCHECK, or
   PTHREAD_MUTEX_DEFAULT).  */
extern int pthread_mutexattr_settype (pthread_mutexattr_t *__attr, int __kind)



#include <semaphore.h>

/*closes the named semaphore referred to by sem,
       allowing any resources that the system has allocated to the
       calling process for this semaphore to be freed.*/
int sem_close(sem_t *sem);

/*destroys the unnamed semaphore at the address
       pointed to by sem.

       Only a semaphore that has been initialized by sem_init(3) should
       be destroyed using sem_destroy().

       Destroying a semaphore that other processes or threads are
       currently blocked on (in sem_wait(3)) produces undefined
       behavior.

       Using a semaphore that has been destroyed produces undefined
       results, until the semaphore has been reinitialized using
       sem_init(3).*/
int sem_destroy(sem_t *sem);

/* places the current value of the semaphore pointed
       to sem into the integer pointed to by sval.

       If one or more processes or threads are blocked waiting to lock
       the semaphore with sem_wait(3), POSIX.1 permits two possibilities
       for the value returned in sval: either 0 is returned; or a
       negative number whose absolute value is the count of the number
       of processes and threads currently blocked in sem_wait(3).  Linux
       adopts the former behavior.*/
int sem_getvalue(sem_t *restrict sem, int *restrict sval);

/*initializes the unnamed semaphore at the address
       pointed to by sem.  The value argument specifies the initial
       value for the semaphore.

       The pshared argument indicates whether this semaphore is to be
       shared between the threads of a process, or between processes.

       If pshared has the value 0, then the semaphore is shared between
       the threads of a process, and should be located at some address
       that is visible to all threads (e.g., a global variable, or a
       variable allocated dynamically on the heap).

       If pshared is nonzero, then the semaphore is shared between
       processes, and should be located in a region of shared memory
       (see shm_open(3), mmap(2), and shmget(2)).  (Since a child
       created by fork(2) inherits its parent's memory mappings, it can
       also access the semaphore.)  Any process that can access the
       shared memory region can operate on the semaphore using
       sem_post(3), sem_wait(3), and so on.

       Initializing a semaphore that has already been initialized
       results in undefined behavior.*/
int sem_init(sem_t *sem, int pshared, unsigned int value);


#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */

/*creates a new POSIX semaphore or opens an existing
       semaphore.  The semaphore is identified by name.  For details of
       the construction of name, see sem_overview(7).

       The oflag argument specifies flags that control the operation of
       the call.  (Definitions of the flags values can be obtained by
       including <fcntl.h>.)  If O_CREAT is specified in oflag, then the
       semaphore is created if it does not already exist.  The owner
       (user ID) of the semaphore is set to the effective user ID of the
       calling process.  The group ownership (group ID) is set to the
       effective group ID of the calling process.  If both O_CREAT and
       O_EXCL are specified in oflag, then an error is returned if a
       semaphore with the given name already exists.

       If O_CREAT is specified in oflag, then two additional arguments
       must be supplied.  The mode argument specifies the permissions to
       be placed on the new semaphore, as for open(2).  (Symbolic
       definitions for the permissions bits can be obtained by including
       <sys/stat.h>.)  The permissions settings are masked against the
       process umask.  Both read and write permission should be granted
       to each class of user that will access the semaphore.  The value
       argument specifies the initial value for the new semaphore.  If
       O_CREAT is specified, and a semaphore with the given name already
       exists, then mode and value are ignored.*/
sem_t *sem_open(const char *name, int oflag);
sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);



int sem_post(sem_t *sem);
/*removes the named semaphore referred to by name.
       The semaphore name is removed immediately.  The semaphore is
       destroyed once all other processes that have the semaphore open
       close it.*/
int sem_unlink(const char *name);

/*decrements (locks) the semaphore pointed to by sem.
       If the semaphore's value is greater than zero, then the decrement
       proceeds, and the function returns, immediately.  If the
       semaphore currently has the value zero, then the call blocks
       until either it becomes possible to perform the decrement (i.e.,
       the semaphore value rises above zero), or a signal handler
       interrupts the call.*/
int sem_wait(sem_t *sem);

/*is the same as sem_wait(), except that if the
       decrement cannot be immediately performed, then call returns an
       error (errno set to EAGAIN) instead of blocking.*/
int sem_trywait(sem_t *sem);

/* is the same as sem_wait(), except that
       abs_timeout specifies a limit on the amount of time that the call
       should block if the decrement cannot be immediately performed.
       The abs_timeout argument points to a timespec(3) structure that
       specifies an absolute timeout in seconds and nanoseconds since
       the Epoch, 1970-01-01 00:00:00 +0000 (UTC).

       If the timeout has already expired by the time of the call, and
       the semaphore could not be locked immediately, then
       sem_timedwait() fails with a timeout error (errno set to
       ETIMEDOUT).

       If the operation can be performed immediately, then
       sem_timedwait() never fails with a timeout error, regardless of
       the value of abs_timeout.  Furthermore, the validity of
       abs_timeout is not checked in this case.*/
int sem_timedwait(sem_t *restrict sem, const struct timespec *restrict abs_timeout);


  PTHREAD_MUTEX_TIMED_NP,
  PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_ADAPTIVE_NP,
  PTHREAD_MUTEX_NORMAL = PTHREAD_MUTEX_TIMED_NP,
  PTHREAD_MUTEX_RECURSIVE = PTHREAD_MUTEX_RECURSIVE_NP,
  PTHREAD_MUTEX_ERRORCHECK = PTHREAD_MUTEX_ERRORCHECK_NP,
  PTHREAD_MUTEX_DEFAULT = PTHREAD_MUTEX_NORMAL,
  PTHREAD_MUTEX_FAST_NP = PTHREAD_MUTEX_TIMED_NP

#include <time.h>

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
    int res;

    if (wait <= 0) {
        res = pthread_mutex_lock (&mutex->handle)
    } else {
        time_t sec = (time_t)wait / 1000;
        time_t ns = ((time_t)wait - (sec * 1000)) * 1000000

        struct timespec ts = {
            .tv_sec = sec,
            .tv_nsec = ns
        };

        res = pthread_mutex_timedlock(&mutex->handle, &ts)
    }

    return res == 0 ? 1 : 0
}


void lock_release(threading_mutex_t *mutex)
{
    pthread_mutex_unlock(&mutex->handle)
}


void lock_init(threading_mutex_t *mutex)
{
    pthread_mutexattr_init(&mutex->attr);
    pthread_mutexattr_setpshared (&mutex->attr, PTHREAD_PROCESS_SHARED );
    pthread_mutexattr_settype(&mutex->attr, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init (&mutex->handle, &mutex->attr);
}


int rlock_acquire(threading_mutex_t *mutex, int wait)
{
    return lock_acquire(mutex, wait);
}


void rlock_release(threading_mutex_t *mutex)
{
    lock_release(mutex);
}


void rlock_init(threading_mutex_t *mutex)
{
    pthread_mutexattr_init(&mutex->attr);
    pthread_mutexattr_setpshared (&mutex->attr, PTHREAD_PROCESS_SHARED );
    pthread_mutexattr_settype(&mutex->attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init (&mutex->handle, &mutex->attr);
}


// the mutex controls access to the linked list
static threading_thread_entry_cb_t ext_threading_thread_entry = NULL;
threading_mutex_t t_mutex;
mp_obj_thread_thread_t _main_thread;
mp_obj_thread_thread_t *t_thread = NULL; // root pointer, handled by threading_gc_others
size_t thread_stack_size = 0;


void threading_init(void *stack, uint32_t stack_len) {
    pthread_key_create(&tls_key, NULL);
    pthread_setspecific(tls_key, &mp_state_ctx.thread);

    // Needs to be a recursive mutex to emulate the behavior of
    // BEGIN_ATOMIC_SECTION on bare metal.
    pthread_mutexattr_t thread_mutex_attr;
    pthread_mutexattr_init(&thread_mutex_attr);
    pthread_mutexattr_settype(&thread_mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&thread_mutex, &thread_mutex_attr);



#if defined(__APPLE__)
    thread_signal_done_p = sem_open(thread_signal_done_name, O_CREAT | O_EXCL, 0666, 0);
#else
    sem_init(&thread_signal_done, 0, 0);
#endif

    // enable signal handler for garbage collection
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = mp_thread_gc;
    sigemptyset(&sa.sa_mask);
    sigaction(MP_THREAD_GC_SIGNAL, &sa, NULL);

    // create first entry in linked list of all threads
    thread = malloc(sizeof(mp_thread_t));
    thread->id = pthread_self();
    thread->ready = 1;
    thread->arg = NULL;
    thread->next = NULL;

    _main_thread.id = (uint64_t)pthread_self();
    _main_thread.core_id = (uint8_t)xTaskGetCoreID(xTaskGetCurrentTaskHandle());
    _main_thread.ident = mp_obj_new_int_from_uint((mp_uint_t)_main_thread.id);
    _main_thread.ready = 1;
    _main_thread.is_alive = true;
    _main_thread.call_args = NULL;
    _main_thread.next = NULL;


37860221


4622.00

IOTV 3000 * 100000 = 300,000,000



M18A1 119 ea


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

int pthread_setaffinity_np(pthread_t thread, size_t cpusetsize,
                                  const cpu_set_t *cpuset);

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
