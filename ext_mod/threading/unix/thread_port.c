#include <pthread.h>
#include <semaphore.h>



int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);



int    sem_close(sem_t *);
int    sem_destroy(sem_t *);
int    sem_getvalue(sem_t *restrict, int *restrict);
int    sem_init(sem_t *, int, unsigned);
sem_t *sem_open(const char *, int, ...);
int    sem_post(sem_t *);
int    sem_timedwait(sem_t *restrict, const struct timespec *restrict);
int    sem_trywait(sem_t *);
int    sem_unlink(const char *);
int    sem_wait(sem_t *);


pthread_attr_t
pthread_barrier_t
pthread_barrierattr_t
pthread_cond_t
pthread_condattr_t
pthread_key_t
pthread_mutex_t

pthread_mutexattr_t

pthread_once_t

pthread_spinlock_t
pthread_t



int   pthread_atfork(void (*)(void), void (*)(void), void(*)(void));

int   pthread_attr_destroy(pthread_attr_t *);
int   pthread_attr_getdetachstate(const pthread_attr_t *, int *);
int   pthread_attr_getguardsize(const pthread_attr_t *restrict, size_t *restrict);
int   pthread_attr_getinheritsched(const pthread_attr_t *restrict, int *restrict);
int   pthread_attr_getschedparam(const pthread_attr_t *restrict, struct sched_param *restrict);
int   pthread_attr_getschedpolicy(const pthread_attr_t *restrict, int *restrict);
int   pthread_attr_getscope(const pthread_attr_t *restrict, int *restrict);
int   pthread_attr_getstack(const pthread_attr_t *restrict, void **restrict, size_t *restrict);
int   pthread_attr_getstacksize(const pthread_attr_t *restrict, size_t *restrict);
int   pthread_attr_init(pthread_attr_t *);
int   pthread_attr_setdetachstate(pthread_attr_t *, int);
int   pthread_attr_setguardsize(pthread_attr_t *, size_t);
int   pthread_attr_setinheritsched(pthread_attr_t *, int);
int   pthread_attr_setschedparam(pthread_attr_t *restrict, const struct sched_param *restrict);
int   pthread_attr_setschedpolicy(pthread_attr_t *, int);
int   pthread_attr_setscope(pthread_attr_t *, int);
int   pthread_attr_setstack(pthread_attr_t *, void *, size_t);
int   pthread_attr_setstacksize(pthread_attr_t *, size_t);

int   pthread_barrier_destroy(pthread_barrier_t *);
int   pthread_barrier_init(pthread_barrier_t *restrict, const pthread_barrierattr_t *restrict, unsigned);
int   pthread_barrier_wait(pthread_barrier_t *);
int   pthread_barrierattr_destroy(pthread_barrierattr_t *);
int   pthread_barrierattr_getpshared(const pthread_barrierattr_t *restrict, int *restrict);
int   pthread_barrierattr_init(pthread_barrierattr_t *);
int   pthread_barrierattr_setpshared(pthread_barrierattr_t *, int);

int   pthread_cancel(pthread_t);

int   pthread_cond_broadcast(pthread_cond_t *);
int   pthread_cond_destroy(pthread_cond_t *);
int   pthread_cond_init(pthread_cond_t *restrict, const pthread_condattr_t *restrict);
int   pthread_cond_signal(pthread_cond_t *);
int   pthread_cond_timedwait(pthread_cond_t *restrict, pthread_mutex_t *restrict, const struct timespec *restrict);
int   pthread_cond_wait(pthread_cond_t *restrict, pthread_mutex_t *restrict);
int   pthread_condattr_destroy(pthread_condattr_t *);
int   pthread_condattr_getclock(const pthread_condattr_t *restrict, clockid_t *restrict);
int   pthread_condattr_getpshared(const pthread_condattr_t *restrict, int *restrict);
int   pthread_condattr_init(pthread_condattr_t *);
int   pthread_condattr_setclock(pthread_condattr_t *, clockid_t);
int   pthread_condattr_setpshared(pthread_condattr_t *, int);

int   pthread_create(pthread_t *restrict, const pthread_attr_t *restrict, void *(*)(void*), void *restrict);
int   pthread_detach(pthread_t);
int   pthread_equal(pthread_t, pthread_t);

void  pthread_exit(void *);

int   pthread_getconcurrency(void);
int   pthread_getcpuclockid(pthread_t, clockid_t *);
int   pthread_getschedparam(pthread_t, int *restrict, struct sched_param *restrict);
void *pthread_getspecific(pthread_key_t);

int   pthread_join(pthread_t, void **);

int   pthread_key_create(pthread_key_t *, void (*)(void*));
int   pthread_key_delete(pthread_key_t);




int   pthread_mutex_consistent(pthread_mutex_t *);






int   pthread_mutex_getprioceiling(const pthread_mutex_t *restrict, int *restrict);

int   pthread_mutex_setprioceiling(pthread_mutex_t *restrict, int, int *restrict);
int   pthread_mutexattr_destroy(pthread_mutexattr_t *);
int   pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *restrict, int *restrict);
int   pthread_mutexattr_getprotocol(const pthread_mutexattr_t *restrict, int *restrict);
int   pthread_mutexattr_getpshared(const pthread_mutexattr_t *restrict, int *restrict);
int   pthread_mutexattr_getrobust(const pthread_mutexattr_t *restrict, int *restrict);
int   pthread_mutexattr_gettype(const pthread_mutexattr_t *restrict, int *restrict);


int   pthread_once(pthread_once_t *, void (*)(void));

pthread_t pthread_self(void);

int   pthread_setcancelstate(int, int *);
int   pthread_setcanceltype(int, int *);
int   pthread_setconcurrency(int);
int   pthread_setschedparam(pthread_t, int, const struct sched_param *);
int   pthread_setschedprio(pthread_t, int);
int   pthread_setspecific(pthread_key_t, const void *);

int   pthread_spin_destroy(pthread_spinlock_t *);
int   pthread_spin_init(pthread_spinlock_t *, int);
int   pthread_spin_lock(pthread_spinlock_t *);
int   pthread_spin_trylock(pthread_spinlock_t *);
int   pthread_spin_unlock(pthread_spinlock_t *);

void  pthread_testcancel(void);



int    sem_close(sem_t *);




sem_t *sem_open(const char *, int, ...);


int    sem_trywait(sem_t *);
int    sem_unlink(const char *);






#include <sched.h> // sched_getcpu
#include <unistd.h> // sysconf


uint8_t mp_get_cpu_count(void)
{
    return (uint8_t)sysconf(_SC_NPROCESSORS_ONLN);
}


uint8_t mp_get_process_core(thread_t *thread)
{
    cpu_set_t cpuset;

    // Get the current thread's affinity mask
    pthread_getaffinity_np(thread->handle, sizeof(cpuset), &cpuset)

    // Print the CPUs the thread is allowed to run on
    for (int i = 0; i < CPU_SETSIZE; i++) {
        if (CPU_ISSET(i, &cpuset)) return (uint8_t)i;
    }
}


uint8_t mp_get_current_process_core(void)
{
    return (uint8_t)sched_getcpu();
}


uint32_t mp_get_current_thread_id(void)
{
    return (uint32_t)pthread_self()
}


void threading_event_set(thread_event_t *event)
{
    if (event->is_set == 0) {
        event->is_set = 1;
        uint16_t count = event->ref_count;

        for (uint16_t i=0;i<count;i++) {
            sem_post(&event->handle);
        }
    } else {
        THREAD_UNUSED(event);
    }
}


bool threading_event_isset(thread_event_t *event)
{
    return (bool)event->is_set;
}


void threading_event_clear(thread_event_t *event)
{
    event->is_set = 0;
}

void threading_event_wait(thread_event_t *event, int32_t wait_ms)
{
    if (event->is_set == 0) {
        event->ref_count++;

        if (wait_ms < 0) {
            sem_wait(&event->handle);
        } else {
            time_t tv_sec = (time_t)(wait_ms / 1000);
            wait_ms = wait_ms - ((int32_t)tv_sec * 1000);

            long tv_nsec = (long)(wait_ms * 1000000);

            struct timespec tv = {
                .tv_sec=tv_sec,
                .tv_nsec=tv_nsec
            };
            sem_timedwait(&event->handle, &tv);
        }

        event->ref_count--;

    } else {
        THREAD_UNUSED(event);
        THREAD_UNUSED(wait_ms);
    }
}


void threading_event_init(thread_event_t *event)
{
    event->is_set = 0;
    event->ref_count = 0;
    sem_init(&event->handle, 0, 0);
}


void threading_event_delete(thread_event_t *event)
{
    int value = 0;

    sem_getvalue(&event->handle, &value);

    when the value is > 0
    sem_post = increases the value
    while (value != 0) {
        sem_post(&event->handle);
        sem_getvalue(&event->handle, &value);
    }

    sem_destroy(&event->handle);
}


int threading_lock_acquire(thread_lock_t *lock, int32_t wait_ms)
{
    lock->ref_count++;
    if (wait_ms < 0) {
        pthread_mutex_lock(&lock->handle);
    } else {
        time_t tv_sec = (time_t)(wait_ms / 1000);
        wait_ms = wait_ms - ((int32_t)tv_sec * 1000);

        long tv_nsec = (long)(wait_ms * 1000000);

        struct timespec tv = {
            .tv_sec=tv_sec,
            .tv_nsec=tv_nsec
        };

        pthread_mutex_timedlock(&lock->handle, &tv);
    }
    lock->ref_count--;
}

void threading_lock_delete(thread_lock_t *lock)
{
    uint16_t count = lock->ref_count;
    for (uint16_t i=0;i<count;i++) {
        pthread_mutex_unlock(&lock->handle);
    }
    pthread_mutex_destroy(&lock->handle);
}

void threading_lock_release(thread_lock_t *lock)
{
    pthread_mutex_unlock(&lock->handle);
}


void threading_lock_init(thread_lock_t *lock)
{
    pthread_attr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    pthread_mutex_init(&lock->handle, &attr);
}


int threading_rlock_acquire(thread_rlock_t *rlock, uint16_t wait_ms)
{
   return threading_lock_acquire((thread_lock_t *)rlock, wait_ms);
}


void threading_rlock_release(thread_rlock_t *rlock)
{
    threading_lock_release((thread_lock_t *)rlock);
}


void threading_rlock_delete(thread_rlock_t *rlock)
{
    threading_lock_delete((thread_lock_t *)rlock);
}


void threading_rlock_init(thread_rlock_t *rlock)
{
    pthread_attr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&rlock->handle, &attr);
}


uint16_t threading_semphamore_get_count(thread_semphamore_t *sem)
{
    int count;
    sem_getvalue(&sem->handle, &count);
    return (uint16_t)count;
}


bool threading_semphamore_acquire(thread_semphamore_t *sem, int32_t wait_ms)
{
    sem->ref_count++;
    if (wait_ms < 0) {
        sem_wait(&event->handle);
    } else {
        time_t tv_sec = (time_t)(wait_ms / 1000);
        wait_ms = wait_ms - ((int32_t)tv_sec * 1000);

        long tv_nsec = (long)(wait_ms * 1000000);

        struct timespec tv = {
            .tv_sec=tv_sec,
            .tv_nsec=tv_nsec
        };
        sem_timedwait(&event->handle, &tv);
    }
    sem->ref_count--;
}


void threading_semphamore_release(thread_semphamore_t *sem)
{
    sem_post(&sem->handle);
}


void threading_semphamore_init(thread_semphamore_t *sem, uint16_t start_value)
{
    sem->ref_count = 0;
    sem_init(&sem->handle, 0, (unsigned)start_value);
}


void threading_semphamore_delete(thread_semphamore_t *sem)
{
    uint16_t count = sem->ref_count;
    for (uint16_t i=0;i<count;i++) {
        sem_post(&sem->handle);
    }

    sem_destroy(&sem->handle);
}


static threading_thread_entry_cb_t ext_threading_thread_entry = NULL;


thread_rlock_t t_mutex;
mp_obj_thread_t _main_thread;
mp_obj_thread_t *t_thread = NULL; // root pointer, handled by threading_gc_others
size_t thread_stack_size = 0;


void *thread_entry_cb(mp_obj_thread_t *self)
{
    // signal that we are set up and running
    self->ready = 1;
    self->is_alive = true;

    mp_state_thread_t ts;
    mp_thread_init_state(&ts, self->call_args->stack_size, self->call_args->dict_locals, self->call_args->dict_globals);

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
    THREAD_UNUSED(stack);
    THRAED_UNUSED(stack_len);

    // create the first entry in the linked list of all threads
    _main_thread.thread.handle = pthread_self();
    _main_thread.core_id = mp_get_current_process_core();
    _main_thread.ident = mp_obj_new_int_from_uint((mp_uint_t)_main_thread.thread.handle);
    _main_thread.ready = 1;
    _main_thread.is_alive = true;
    _main_thread.arg = NULL;
    _main_thread.next = NULL;
    threading_rlock_init(&t_mutex);

    t_thread = &_main_thread;
}


void threading_deinit(void)
{
    threading_rlock_acquire(&t_mutex, 0)
    while (t_thread->next != NULL) {
        mp_obj_thread_t *th = t_thread;
        t_thread = t_thread->next;
        pthread_cancel(&th->thread.handle);
    }
    threading_rlock_release(&t_mutex);
}


void threading_gc_others(void)
{
    threading_rlock_acquire(&t_mutex, 0);

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
    threading_rlock_release(&t_mutex);
}


static void pthread_entry(void *arg)
{
    if (ext_threading_thread_entry) {

    #if defined(__APPLE__)
        if (mp_thread_is_realtime_enabled) {
            mp_thread_set_realtime();
        }
    #endif

        mp_obj_thread_t *self = (mp_obj_thread_t *)arg;
        ext_threading_thread_entry(self);
    }
}


mp_uint_t thread_create_ex(mp_obj_thread_t *self)
{
    // store thread entry function into a global variable so we can access it
    ext_threading_thread_entry = thread_entry_cb;

    // default stack size is 8k machine-words
    if (self->call_args->stack_size == 0) {
        self->call_args->stack_size = 8192 * sizeof(void *);
    }

    // minimum stack size is set by pthreads
    if (self->call_args->stack_size < PTHREAD_STACK_MIN) {
        self->call_args->stack_size = PTHREAD_STACK_MIN;
    }

    // ensure there is enough stack to include a stack-overflow margin
    if (self->call_args->stack_size < 2 * THREAD_STACK_OVERFLOW_MARGIN) {
        self->call_args->stack_size = 2 * THREAD_STACK_OVERFLOW_MARGIN;
    }

    // set thread attributes
    pthread_attr_t attr;
    int ret = pthread_attr_init(&attr);
    if (ret != 0) {
        goto er;
    }
    ret = pthread_attr_setstacksize(&attr, self->call_args->stack_size);
    if (ret != 0) {
        goto er;
    }

    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (ret != 0) {
        goto er;
    }

    threading_rlock_acquire(&t_mutex, 0);

    // create thread
    pthread_t id;
    ret = pthread_create(&self->thread.handle, &attr, entry, arg);
    if (ret != 0) {
        threading_rlock_release(&t_mutex);
        goto er;
    }

    // adjust stack_size to provide room to recover from hitting the limit
    self->call_args->stack_size -= THREAD_STACK_OVERFLOW_MARGIN;

    // add thread to linked list of all threads
    self->ready = 0;
    self->next = t_thread;
    t_thread = self;

    threading_rlock_release(&t_mutex);

    MP_STATIC_ASSERT(sizeof(mp_uint_t) >= sizeof(pthread_t));
    return (mp_uint_t)self->thread.handle;

er:
    mp_raise_OSError(ret);
}


mp_uint_t threading_create_thread(mp_obj_thread_t *self)
{
    return thread_create_ex(self);
}

void threading_delete_thread(mp_obj_thread_t *self)
{



}














