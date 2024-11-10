
#include "multiprocessing.h"
#include "threading.h"
#include "thread_lock.h"
#include "thread_port.h"


mp_obj_t *processes;

void multiprocessing_init(void)
{
    processes = (mp_obj_t *)malloc(sizeof(mp_obj_t) * mp_get_cpu_count());

    mp_obj_thread_t * main_thread = (mp_obj_thread_t *)MP_OBJ_TO_PTR(mp_get_main_thread());
    uint8_t curr_core_id = mp_get_process_core(&main_thread->thread);
    processes[curr_core_id] = MP_OBJ_FROM_PTR(main_thread);
}


static mp_obj_t multiprocessing_active_children(void)
{
    mp_obj_t list = mp_obj_new_list(0, NULL);

    uint8_t core_id = mp_get_current_process_core();
    uint8_t task_core_id;

    threading_lock_acquire((thread_lock_t *)(&t_mutex), 1);

    for (mp_obj_thread_t *th = t_thread; th != NULL; th = th->next) {
        if (!th->is_alive) {
            continue;
        }
        task_core_id = mp_get_process_core(&th->thread);

        if (task_core_id == core_id) {
            mp_obj_list_append(list, MP_OBJ_FROM_PTR(th));
        }
    }

    threading_lock_release((thread_lock_t *)(&t_mutex));
    return list;
}

static MP_DEFINE_CONST_FUN_OBJ_0(multiprocessing_active_children_obj, multiprocessing_active_children);


static mp_obj_t multiprocessing_cpu_count(void)
{
    return mp_obj_new_int_from_uint(mp_get_cpu_count());
}

static MP_DEFINE_CONST_FUN_OBJ_0(multiprocessing_cpu_count_obj, multiprocessing_cpu_count);


static mp_obj_t multiprocessing_current_process(void)
{
    uint8_t core_id = mp_get_current_process_core();
    return processes[core_id];
}

static MP_DEFINE_CONST_FUN_OBJ_0(multiprocessing_current_process_obj, multiprocessing_current_process);


static mp_obj_t multiprocessing_parent_process(void)
{
    // mp_obj_t main_thread = mp_get_main_thread();
    uint8_t core_id = mp_get_current_process_core();

    return processes[core_id];
}

static MP_DEFINE_CONST_FUN_OBJ_0(multiprocessing_parent_process_obj, multiprocessing_parent_process);


static const mp_rom_map_elem_t mp_module_multiprocessing_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_multiprocessing) },
    { MP_ROM_QSTR(MP_QSTR_RLock), (mp_obj_t)&mp_type_multiprocessing_rlock_t },
    { MP_ROM_QSTR(MP_QSTR_Lock), (mp_obj_t)&mp_type_multiprocessing_lock_t },
    { MP_ROM_QSTR(MP_QSTR_Event), (mp_obj_t)&mp_type_multiprocessing_event_t },
    { MP_ROM_QSTR(MP_QSTR_Process), (mp_obj_t)&mp_type_multiprocessing_process_t },
    { MP_ROM_QSTR(MP_QSTR_Semaphore), (mp_obj_t)&mp_type_multiprocessing_semaphore_t },
    { MP_ROM_QSTR(MP_QSTR_cpu_count), MP_ROM_PTR(&multiprocessing_cpu_count_obj) },
    { MP_ROM_QSTR(MP_QSTR_active_children), MP_ROM_PTR(&multiprocessing_active_children_obj) },
    { MP_ROM_QSTR(MP_QSTR_current_process), MP_ROM_PTR(&multiprocessing_current_process_obj) },
    { MP_ROM_QSTR(MP_QSTR_parent_process), MP_ROM_PTR(&multiprocessing_parent_process_obj) }
};

static MP_DEFINE_CONST_DICT(mp_module_multiprocessing_globals, mp_module_multiprocessing_globals_table);


const mp_obj_module_t mp_module_multiprocessing = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_multiprocessing_globals,
};

MP_REGISTER_MODULE(MP_QSTR_multiprocessing, mp_module_multiprocessing);