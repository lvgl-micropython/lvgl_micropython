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

#ifndef __MPTHREADPORT__H__
#define __MPTHREADPORT__H__

#include <pthread.h>
#include <stdbool.h>

// this structure forms a linked list, one node per active thread
typedef struct _mp_thread_t {
    pthread_t id;           // system id of thread
    int ready;              // whether the thread is ready and running
    void *arg;              // thread Python args, a GC root pointer
    struct _mp_thread_t *next;
} mp_thread_t;

extern pthread_key_t tls_key;
extern pthread_mutex_t thread_mutex;
extern mp_thread_t *thread;


typedef pthread_mutex_t mp_thread_mutex_t;

void mp_thread_init(void);
void mp_thread_deinit(void);
void mp_thread_gc_others(void);

// Unix version of "enable/disable IRQs".
// Functions as a port-global lock for any code that must be serialised.
void mp_thread_unix_begin_atomic_section(void);
void mp_thread_unix_end_atomic_section(void);

extern mp_thread_mutex_t thread_mutex;
extern mp_thread_t thread_entry0;
extern mp_thread_t *thread = NULL; // root pointer, handled by mp_thread_gc_others

// for `-X realtime` command line option
#if defined(__APPLE__)
extern bool mp_thread_is_realtime_enabled;
void mp_thread_set_realtime(void);
#endif

#endif /* __MPTHREADPORT__H__ */
