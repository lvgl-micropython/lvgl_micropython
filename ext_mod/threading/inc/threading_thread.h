#include "freertos/task.h"
#include "freertos/idf_additions.h"

// micropython includes
#include "py/obj.h"
#include "py/runtime.h"


#ifndef __THREADING_THREAD_H__
    #define __THREADING_THREAD_H__

    typedef struct _threading_thread_entry_args_t {
        mp_obj_dict_t *dict_locals;
        mp_obj_dict_t *dict_globals;
        size_t stack_size;
        mp_obj_t fun;
        size_t n_args;
        size_t n_kw;
        mp_obj_t args[];
        bool is_alive;
    } threading_thread_entry_args_t;

    typedef struct _mp_obj_threading_thread_t {
        mp_obj_base_t base;
        TaskHandle_t id;
        mp_obj_t ident;

        mp_obj_t name;

        threading_thread_entry_args_t *th_args;

        int ready;              // whether the thread is ready and running
        void *arg;              // thread Python args, a GC root pointer
        void *stack;            // pointer to the stack
        size_t stack_len;       // number of words in the stack
        struct _mp_obj_threading_thread_t *next;

    } mp_obj_threading_thread_t;

    extern const mp_obj_type_t mp_type_threading_thread_t;

    void *threading_thread_entry(threading_thread_entry_args_t *args)


#endif