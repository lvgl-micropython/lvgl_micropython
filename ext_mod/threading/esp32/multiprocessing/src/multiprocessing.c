#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"


#include "thread_common.h"
#include "thread_thread.h"

#include "threading.h"

#include "multiprocessing.h"
#include "multiprocessing_event.h"
#include "multiprocessing_lock.h"
#include "multiprocessing_process.h"
#include "multiprocessing_rlock.h"
#include "multiprocessing_semaphore.h"


mp_obj_t processes[2];


void multiprocessing_init(void)
{
    mp_obj_thread_thread_t * main_thread = (mp_obj_thread_thread_t *)MP_OBJ_TO_PTR(threading_main_thread());
    uint16_t curr_core_id = (uint16_t)xTaskGetCoreID(main_thread->id);
    processes[curr_core_id] = MP_OBJ_FROM_PTR(main_thread);
}


static mp_obj_t multiprocessing_active_children(void)
{
    mp_obj_t list = mp_obj_new_list(0, NULL);

    uint16_t core_id = (uint16_t)xTaskGetCoreID(xTaskGetCurrentTaskHandle());
    uint16_t task_core_id;

    lock_acquire(&t_mutex, 1);

    for (mp_obj_thread_thread_t *th = t_thread; th != NULL; th = th->next) {
        if (!th->is_alive) {
            continue;
        }
        task_core_id = (uint16_t)xTaskGetCoreID(th->id);

        if (task_core_id == core_id) {
            mp_obj_list_append(list, MP_OBJ_FROM_PTR(th));
        }
    }

    lock_release(&t_mutex);
    return list;
}

static MP_DEFINE_CONST_FUN_OBJ_0(multiprocessing_active_children_obj, multiprocessing_active_children);


static mp_obj_t multiprocessing_cpu_count(void)
{
    return mp_obj_new_int(2);
}

static MP_DEFINE_CONST_FUN_OBJ_0(multiprocessing_cpu_count_obj, multiprocessing_cpu_count);


static mp_obj_t multiprocessing_current_process(void)
{
    uint16_t core_id = (uint16_t)xTaskGetCoreID(xTaskGetCurrentTaskHandle());
    return processes[core_id];
}

static MP_DEFINE_CONST_FUN_OBJ_0(multiprocessing_current_process_obj, multiprocessing_current_process);


static mp_obj_t multiprocessing_parent_process(void)
{
    mp_obj_t main_thread = threading_main_thread();
    uint16_t core_id = (uint16_t)xTaskGetCoreID(xTaskGetCurrentTaskHandle());

    if (processes[core_id] == main_thread) {
        return mp_const_none;
    } else {
        return main_thread;
    }
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


