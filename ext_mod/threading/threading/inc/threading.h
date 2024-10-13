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
#ifndef __THREADING_H__
    #define __THREADING_H__

    #define THREAD_MIN_STACK_SIZE                        (4 * 1024)
    #define THREAD_DEFAULT_STACK_SIZE                    (THREADING_MIN_STACK_SIZE + 1024)
    #define THREAD_PRIORITY                              (ESP_TASK_PRIO_MIN + 1)

    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 2, 0) && !CONFIG_FREERTOS_ENABLE_STATIC_TASK_CLEAN_UP
        #define FREERTOS_TASK_DELETE_HOOK                       vTaskPreDeletionHook
    #else
        #define FREERTOS_TASK_DELETE_HOOK                       vPortCleanUpTCB
    #endif

    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/semphr.h"
    #include "freertos/queue.h"

    #include "py/mpconfig.h"

    #include "threading_thread.h"

    typedef void *(*threading_thread_entry_cb_t)(mp_obj_threading_thread_t *);
    extern size_t thread_stack_size;

    mp_uint_t threading_thread_create(threading_thread_entry_cb_t entry, mp_obj_threading_thread_t *th);

    void threading_init(void *stack, uint32_t stack_len);
    void threading_gc_others(void);
    void threading_deinit(void);

#endif // __THREADING_H__
