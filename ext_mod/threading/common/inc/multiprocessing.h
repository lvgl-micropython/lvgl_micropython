
#ifndef __MULTIPROCESSING_H__
    #define __MULTIPROCESSING_H__

    #include "thread_thread.h"

    uint8_t mp_get_current_process_core(void); // needs to be defined in port
    uint8_t mp_get_process_core(thread_t *thread); // needs to be defined in port
    uint8_t mp_get_cpu_count(void); // needs to be defined in port
    void multiprocessing_init(void); // needs to be defined in port

    extern mp_obj_t *processes; // needs to be defined in port
    extern uint8_t process_count; // needs to be defined in port

#endif /* __MULTIPROCESSING_H__ */