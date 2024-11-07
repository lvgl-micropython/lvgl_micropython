
#ifndef __MULTIPROCESSING_H__
    #define __MULTIPROCESSING_H__

    uint8_t mp_get_current_process_core(void);
    uint8_t mp_get_process_core(thread_t *thread);
    uint8_t mp_get_cpu_count(void);

    extern mp_obj_t *processes;
    extern uint8_t process_count;

    void multiprocessing_init(void);
    extern mp_obj_t processes[2];

#endif /* __MULTIPROCESSING_H__ */