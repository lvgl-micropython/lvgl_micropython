// micropython includes
#include "py/obj.h"
#include "py/runtime.h"

#include "thread_common.h"

#ifndef __THREAD_THREAD_H__
    #define __THREAD_THREAD_H__

    typedef struct _thread_t thread_t;

    typedef struct _thread_entry_args_t {
        mp_obj_dict_t *dict_locals;
        mp_obj_dict_t *dict_globals;
        size_t stack_size;
        mp_obj_t fun;
        size_t n_args;
        size_t n_kw;
        mp_obj_t args[];
    } thread_entry_args_t;

    typedef struct _mp_obj_thread_t {
        mp_obj_base_t base;
        thread_t thread;
        mp_obj_t ident;

        mp_obj_t name;

        thread_entry_args_t *call_args;

        int ready;              // whether the thread is ready and running
        int is_alive;
        uint8_t core_id;
        void *arg;              // thread Python args, a GC root pointer
        void *stack;            // pointer to the stack
        size_t stack_len;       // number of words in the stack
        struct _mp_obj_thread_t *next;

    } mp_obj_thread_t;

    mp_uint_t threading_create_thread(mp_obj_thread_t *self);
    void threading_delete_thread(thread_t *thread);

#endif