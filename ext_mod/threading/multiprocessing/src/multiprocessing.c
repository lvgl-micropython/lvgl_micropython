#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "../multiprocess.h"
#include "../inc/common.h"




extern mp_obj_process_t *mp_processes[2];


mp_processes[0] = (*mp_obj_process_t)heap_caps_malloc(sizeof(mp_obj_process_t))

mp_processes[0]->id = mp_main_task_handle;



// ***************** Lock



// ***************** RLock




