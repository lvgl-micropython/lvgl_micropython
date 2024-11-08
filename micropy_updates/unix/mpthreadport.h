/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George on behalf of Pycom Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */



    #include <pthread.h>
    #include <stdbool.h>

    // Some platforms don't have SIGRTMIN but if we do have it, use it to avoid
    // potential conflict with other uses of the more commonly used SIGUSR1.
#ifdef SIGRTMIN
    #define MP_THREAD_GC_SIGNAL (SIGRTMIN + 5)
#else
    #define MP_THREAD_GC_SIGNAL (SIGUSR1)
#endif

    // This value seems to be about right for both 32-bit and 64-bit builds.
    #define THREAD_STACK_OVERFLOW_MARGIN (8192)



    typedef pthread_mutex_t mp_thread_mutex_t;

    void mp_thread_init(void);
    void mp_thread_deinit(void);
    void mp_thread_gc_others(void);

    // Unix version of "enable/disable IRQs".
    // Functions as a port-global lock for any code that must be serialised.
    void mp_thread_unix_begin_atomic_section(void);
    void mp_thread_unix_end_atomic_section(void);

    // for `-X realtime` command line option
#if defined(__APPLE__)
    extern bool mp_thread_is_realtime_enabled;
    void mp_thread_set_realtime(void);
#endif


#include <stdbool.h>

#ifndef __MPTHREADPORT__H__
    #define __MPTHREADPORT__H__

    #include <pthread.h>
    #include <semaphore.h>

    // this is used to synchronise the signal handler of the thread
    // it's needed because we can't use any pthread calls in a signal handler
#if defined(__APPLE__)
    extern char thread_signal_done_name[25];
    extern sem_t *thread_signal_done_p;
#else
    extern sem_t thread_signal_done;
#endif

    typedef pthread_mutex_t mp_thread_mutex_t;

    void mp_thread_init(void);
    void mp_thread_deinit(void);
    void mp_thread_gc_others(void);

    // Unix version of "enable/disable IRQs".
    // Functions as a port-global lock for any code that must be serialised.
    void mp_thread_unix_begin_atomic_section(void);
    void mp_thread_unix_end_atomic_section(void);

// for `-X realtime` command line option
#if defined(__APPLE__)
    extern bool mp_thread_is_realtime_enabled;
    void mp_thread_set_realtime(void);
#endif
#endif /* __MPTHREADPORT__H__ */

