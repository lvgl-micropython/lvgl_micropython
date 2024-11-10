// micropython includes
#include "py/obj.h"
#include "py/runtime.h"


#ifndef __THREAD_THREAD_H__
    #define __THREAD_THREAD_H__

    #include "thread_port.h"

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
        void *stack;            // pointer to the stack
        size_t stack_len;       // number of words in the stack
        struct _mp_obj_thread_t *next;

    } mp_obj_thread_t;

    mp_uint_t threading_create_thread(mp_obj_thread_t *self); // needs to be defined in port
    void threading_delete_thread(mp_obj_thread_t *self); // needs to be defined in port

    extern const mp_obj_type_t mp_type_threading_thread_t;
    extern const mp_obj_type_t mp_type_multiprocessing_process_t;

#endif