

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/atomic.h"
#include "freertos/event_groups.h"
#include "freertos/idf_additions.h"
#include "freertos/portmacro.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/stream_buffer.h"
#include "freertos/message_buffer.h"
#include "freertos/task.h"
#include "freertos/timers.h"


#include "mpy_atomic.h"
#include "mpy_event_groups.h"
#include "mpy_idf_additions.h"
#include "mpy_portmacro.h"
#include "mpy_projdefs.h"
#include "mpy_queue.h"
#include "mpy_semphr.h"
#include "mpy_stream_buffer.h"
#include "mpy_message_buffer.h"
#include "mpy_task.h"
#include "mpy_task_notify.h"
#include "mpy_timers.h"


static const mp_rom_map_elem_t freertos_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),           MP_OBJ_NEW_QSTR(MP_QSTR_FreeRTOS)        },

    //atomic
    { MP_ROM_QSTR(MP_QSTR_ATOMIC_ENTER_CRITICAL), MP_ROM_PTR(&mp_ATOMIC_ENTER_CRITICAL_obj) },
    { MP_ROM_QSTR(MP_QSTR_ATOMIC_EXIT_CRITICAL),  MP_ROM_PTR(&mp_ATOMIC_EXIT_CRITICAL_obj)  },

    // task notifications
    { MP_ROM_QSTR(MP_QSTR_xTaskNotifyGive),                    MP_ROM_PTR(&mp_xTaskNotifyGive_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_xTaskNotifyGiveIndexed),             MP_ROM_PTR(&mp_xTaskNotifyGiveIndexed_obj)            },
    { MP_ROM_QSTR(MP_QSTR_vTaskNotifyGiveFromISR),             MP_ROM_PTR(&mp_vTaskNotifyGiveFromISR_obj)            },
    { MP_ROM_QSTR(MP_QSTR_vTaskNotifyGiveIndexedFromISR),      MP_ROM_PTR(&mp_vTaskNotifyGiveIndexedFromISR_obj)     },
    { MP_ROM_QSTR(MP_QSTR_ulTaskNotifyTake),                   MP_ROM_PTR(&mp_ulTaskNotifyTake_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_ulTaskNotifyTakeIndexed),            MP_ROM_PTR(&mp_ulTaskNotifyTakeIndexed_obj)           },
    { MP_ROM_QSTR(MP_QSTR_xTaskNotify),                        MP_ROM_PTR(&mp_xTaskNotify_obj)                       },
    { MP_ROM_QSTR(MP_QSTR_xTaskNotifyWaitIndexed),             MP_ROM_PTR(&mp_xTaskNotifyWaitIndexed_obj)            },
    { MP_ROM_QSTR(MP_QSTR_xTaskNotifyWait),                    MP_ROM_PTR(&mp_xTaskNotifyWait_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_xTaskNotifyIndexedFromISR),          MP_ROM_PTR(&mp_xTaskNotifyIndexedFromISR_obj)         },
    { MP_ROM_QSTR(MP_QSTR_xTaskNotifyFromISR),                 MP_ROM_PTR(&mp_xTaskNotifyFromISR_obj)                },
    { MP_ROM_QSTR(MP_QSTR_xTaskNotifyAndQueryIndexedFromISR),  MP_ROM_PTR(&mp_xTaskNotifyAndQueryIndexedFromISR_obj) },
    { MP_ROM_QSTR(MP_QSTR_xTaskNotifyAndQueryFromISR),         MP_ROM_PTR(&mp_xTaskNotifyAndQueryFromISR_obj)        },
    { MP_ROM_QSTR(MP_QSTR_xTaskNotifyAndQueryIndexed),         MP_ROM_PTR(&mp_xTaskNotifyAndQueryIndexed_obj)        },
    { MP_ROM_QSTR(MP_QSTR_xTaskNotifyIndexed),                 MP_ROM_PTR(&mp_xTaskNotifyIndexed_obj)                },
    { MP_ROM_QSTR(MP_QSTR_xTaskNotifyAndQuery),                MP_ROM_PTR(&mp_xTaskNotifyAndQuery_obj)               },
    { MP_ROM_QSTR(MP_QSTR_xTaskNotifyStateClear),              MP_ROM_PTR(&mp_xTaskNotifyStateClear_obj)             },
    { MP_ROM_QSTR(MP_QSTR_xTaskNotifyStateClearIndexed),       MP_ROM_PTR(&mp_xTaskNotifyStateClearIndexed_obj)      },
    { MP_ROM_QSTR(MP_QSTR_ulTaskNotifyValueClear),             MP_ROM_PTR(&mp_ulTaskNotifyValueClear_obj)            },
    { MP_ROM_QSTR(MP_QSTR_ulTaskNotifyValueClearIndexed),      MP_ROM_PTR(&mp_ulTaskNotifyValueClearIndexed_obj)     },

    // timers
    { MP_ROM_QSTR(MP_QSTR_xTimerCreateStatic),          MP_ROM_PTR(&mp_xTimerCreateStatic_obj)        },
    { MP_ROM_QSTR(MP_QSTR_pvTimerGetTimerID),           MP_ROM_PTR(&mp_pvTimerGetTimerID_obj)         },
    { MP_ROM_QSTR(MP_QSTR_vTimerSetTimerID),            MP_ROM_PTR(&mp_vTimerSetTimerID_obj)          },
    { MP_ROM_QSTR(MP_QSTR_xTimerIsTimerActive),         MP_ROM_PTR(&mp_xTimerIsTimerActive_obj)       },
    { MP_ROM_QSTR(MP_QSTR_xTimerStart),                 MP_ROM_PTR(&mp_xTimerStart_obj)               },
    { MP_ROM_QSTR(MP_QSTR_xTimerStop),                  MP_ROM_PTR(&mp_xTimerStop_obj)                },
    { MP_ROM_QSTR(MP_QSTR_xTimerChangePeriod),          MP_ROM_PTR(&mp_xTimerChangePeriod_obj)        },
    { MP_ROM_QSTR(MP_QSTR_xTimerDelete),                MP_ROM_PTR(&mp_xTimerDelete_obj)              },
    { MP_ROM_QSTR(MP_QSTR_xTimerReset),                 MP_ROM_PTR(&mp_xTimerReset_obj)               },
    { MP_ROM_QSTR(MP_QSTR_xTimerStartFromISR),          MP_ROM_PTR(&mp_xTimerStartFromISR_obj)        },
    { MP_ROM_QSTR(MP_QSTR_xTimerStopFromISR),           MP_ROM_PTR(&mp_xTimerStopFromISR_obj)         },
    { MP_ROM_QSTR(MP_QSTR_xTimerChangePeriodFromISR),   MP_ROM_PTR(&mp_xTimerChangePeriodFromISR_obj) },
    { MP_ROM_QSTR(MP_QSTR_xTimerResetFromISR),          MP_ROM_PTR(&mp_xTimerResetFromISR_obj)        },
    { MP_ROM_QSTR(MP_QSTR_pcTimerGetName),              MP_ROM_PTR(&mp_pcTimerGetName_obj)            },
    { MP_ROM_QSTR(MP_QSTR_vTimerSetReloadMode),         MP_ROM_PTR(&mp_vTimerSetReloadMode_obj)       },
    { MP_ROM_QSTR(MP_QSTR_xTimerGetReloadMode),         MP_ROM_PTR(&mp_xTimerGetReloadMode_obj)       },
    { MP_ROM_QSTR(MP_QSTR_uxTimerGetReloadMode),        MP_ROM_PTR(&mp_uxTimerGetReloadMode_obj)      },
    { MP_ROM_QSTR(MP_QSTR_xTimerGetPeriod),             MP_ROM_PTR(&mp_xTimerGetPeriod_obj)           },
    { MP_ROM_QSTR(MP_QSTR_xTimerGetExpiryTime),         MP_ROM_PTR(&mp_xTimerGetExpiryTime_obj)       },
    { MP_ROM_QSTR(MP_QSTR_xTimerCreateTimerTask),       MP_ROM_PTR(&mp_xTimerCreateTimerTask_obj)         },

#if ( configUSE_TRACE_FACILITY == 1 )
    { MP_ROM_QSTR(MP_QSTR_vTimerSetTimerNumber),        MP_ROM_PTR(&mp_vTimerSetTimerNumber_obj)      },
    { MP_ROM_QSTR(MP_QSTR_uxTimerGetTimerNumber),       MP_ROM_PTR(&mp_uxTimerGetTimerNumber_obj)     },
#endif

    //task
    { MP_ROM_QSTR(MP_QSTR_taskVALID_CORE_ID),                   MP_ROM_PTR(&mp_taskVALID_CORE_ID_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_taskYIELD),                           MP_ROM_PTR(&mp_taskYIELD_obj)                           },
    { MP_ROM_QSTR(MP_QSTR_taskENTER_CRITICAL),                  MP_ROM_PTR(&mp_taskENTER_CRITICAL_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_taskENTER_CRITICAL_FROM_ISR),         MP_ROM_PTR(&mp_taskENTER_CRITICAL_FROM_ISR_obj)         },
    { MP_ROM_QSTR(MP_QSTR_taskENTER_CRITICAL_ISR),              MP_ROM_PTR(&mp_taskENTER_CRITICAL_ISR_obj)              },
    { MP_ROM_QSTR(MP_QSTR_taskEXIT_CRITICAL),                   MP_ROM_PTR(&mp_taskEXIT_CRITICAL_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_taskEXIT_CRITICAL_FROM_ISR),          MP_ROM_PTR(&mp_taskEXIT_CRITICAL_FROM_ISR_obj)          },
    { MP_ROM_QSTR(MP_QSTR_taskEXIT_CRITICAL_ISR),               MP_ROM_PTR(&mp_taskEXIT_CRITICAL_ISR_obj)               },
    { MP_ROM_QSTR(MP_QSTR_taskDISABLE_INTERRUPTS),              MP_ROM_PTR(&mp_taskDISABLE_INTERRUPTS_obj)              },
    { MP_ROM_QSTR(MP_QSTR_taskENABLE_INTERRUPTS),               MP_ROM_PTR(&mp_taskENABLE_INTERRUPTS_obj)               },
    { MP_ROM_QSTR(MP_QSTR_xTaskCreateStatic),                   MP_ROM_PTR(&mp_xTaskCreateStatic_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_vTaskDelete),                         MP_ROM_PTR(&mp_vTaskDelete_obj)                         },
    { MP_ROM_QSTR(MP_QSTR_vTaskDelay),                          MP_ROM_PTR(&mp_vTaskDelay_obj)                          },
    { MP_ROM_QSTR(MP_QSTR_xTaskDelayUntil),                     MP_ROM_PTR(&mp_xTaskDelayUntil_obj)                     },
    { MP_ROM_QSTR(MP_QSTR_vTaskDelayUntil),                     MP_ROM_PTR(&mp_vTaskDelayUntil_obj)                     },
    { MP_ROM_QSTR(MP_QSTR_xTaskAbortDelay),                     MP_ROM_PTR(&mp_xTaskAbortDelay_obj)                     },
    { MP_ROM_QSTR(MP_QSTR_uxTaskPriorityGet),                   MP_ROM_PTR(&mp_uxTaskPriorityGet_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_uxTaskPriorityGetFromISR),            MP_ROM_PTR(&mp_uxTaskPriorityGetFromISR_obj)            },
    { MP_ROM_QSTR(MP_QSTR_eTaskGetState),                       MP_ROM_PTR(&mp_eTaskGetState_obj)                       },
    { MP_ROM_QSTR(MP_QSTR_vTaskPrioritySet),                    MP_ROM_PTR(&mp_vTaskPrioritySet_obj)                    },
    { MP_ROM_QSTR(MP_QSTR_vTaskSuspend),                        MP_ROM_PTR(&mp_vTaskSuspend_obj)                        },
    { MP_ROM_QSTR(MP_QSTR_vTaskResume),                         MP_ROM_PTR(&mp_vTaskResume_obj)                         },
    { MP_ROM_QSTR(MP_QSTR_xTaskResumeFromISR),                  MP_ROM_PTR(&mp_xTaskResumeFromISR_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_vTaskStartScheduler),                 MP_ROM_PTR(&mp_vTaskStartScheduler_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_vTaskEndScheduler),                   MP_ROM_PTR(&mp_vTaskEndScheduler_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_vTaskSuspendAll),                     MP_ROM_PTR(&mp_vTaskSuspendAll_obj)                     },
    { MP_ROM_QSTR(MP_QSTR_xTaskResumeAll),                      MP_ROM_PTR(&mp_xTaskResumeAll_obj)                      },
    { MP_ROM_QSTR(MP_QSTR_xTaskGetTickCount),                   MP_ROM_PTR(&mp_xTaskGetTickCount_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_xTaskGetTickCountFromISR),            MP_ROM_PTR(&mp_xTaskGetTickCountFromISR_obj)            },
    { MP_ROM_QSTR(MP_QSTR_uxTaskGetNumberOfTasks),              MP_ROM_PTR(&mp_uxTaskGetNumberOfTasks_obj)              },
    { MP_ROM_QSTR(MP_QSTR_pcTaskGetName),                       MP_ROM_PTR(&mp_pcTaskGetName_obj)                       },
    { MP_ROM_QSTR(MP_QSTR_uxTaskGetStackHighWaterMark),         MP_ROM_PTR(&mp_uxTaskGetStackHighWaterMark_obj)         },
    { MP_ROM_QSTR(MP_QSTR_vTaskSetThreadLocalStoragePointer),   MP_ROM_PTR(&mp_vTaskSetThreadLocalStoragePointer_obj)   },
    { MP_ROM_QSTR(MP_QSTR_pvTaskGetThreadLocalStoragePointer),  MP_ROM_PTR(&mp_pvTaskGetThreadLocalStoragePointer_obj)  },
    { MP_ROM_QSTR(MP_QSTR_vApplicationStackOverflowHook),       MP_ROM_PTR(&mp_vApplicationStackOverflowHook_obj)       },
    { MP_ROM_QSTR(MP_QSTR_xTaskCatchUpTicks),                   MP_ROM_PTR(&mp_xTaskCatchUpTicks_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_xTaskIncrementTick),                  MP_ROM_PTR(&mp_xTaskIncrementTick_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_vTaskSwitchContext),                  MP_ROM_PTR(&mp_vTaskSwitchContext_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_vTaskMissedYield),                    MP_ROM_PTR(&mp_vTaskMissedYield_obj)                    },
    { MP_ROM_QSTR(MP_QSTR_xTaskGetSchedulerState),              MP_ROM_PTR(&mp_xTaskGetSchedulerState_obj)              },
    { MP_ROM_QSTR(MP_QSTR_xTaskPriorityInherit),                MP_ROM_PTR(&mp_xTaskPriorityInherit_obj)                },
    { MP_ROM_QSTR(MP_QSTR_xTaskPriorityDisinherit),             MP_ROM_PTR(&mp_xTaskPriorityDisinherit_obj)             },
    { MP_ROM_QSTR(MP_QSTR_vTaskPriorityDisinheritAfterTimeout), MP_ROM_PTR(&mp_vTaskPriorityDisinheritAfterTimeout_obj) },

    // event groups
    { MP_ROM_QSTR(MP_QSTR_xEventGroupCreateStatic),     MP_ROM_PTR(&mp_xEventGroupCreateStatic_obj)     },
    { MP_ROM_QSTR(MP_QSTR_xEventGroupWaitBits),         MP_ROM_PTR(&mp_xEventGroupWaitBits_obj)         },
    { MP_ROM_QSTR(MP_QSTR_xEventGroupClearBits),        MP_ROM_PTR(&mp_xEventGroupClearBits_obj)        },
    { MP_ROM_QSTR(MP_QSTR_xEventGroupClearBitsFromISR), MP_ROM_PTR(&mp_xEventGroupClearBitsFromISR_obj) },
    { MP_ROM_QSTR(MP_QSTR_xEventGroupSetBits),          MP_ROM_PTR(&mp_xEventGroupSetBits_obj)          },
    { MP_ROM_QSTR(MP_QSTR_xEventGroupSetBitsFromISR),   MP_ROM_PTR(&mp_xEventGroupSetBitsFromISR_obj)   },
    { MP_ROM_QSTR(MP_QSTR_xEventGroupSync),             MP_ROM_PTR(&mp_xEventGroupSync_obj)             },
    { MP_ROM_QSTR(MP_QSTR_xEventGroupGetBits),          MP_ROM_PTR(&mp_xEventGroupGetBits_obj)          },
    { MP_ROM_QSTR(MP_QSTR_xEventGroupGetBitsFromISR),   MP_ROM_PTR(&mp_xEventGroupGetBitsFromISR_obj)   },
    { MP_ROM_QSTR(MP_QSTR_vEventGroupDelete),           MP_ROM_PTR(&mp_vEventGroupDelete_obj)           },


    // queues
    { MP_ROM_QSTR(MP_QSTR_xQueueCreateStatic),            MP_ROM_PTR(&mp_xQueueCreateStatic_obj)            },
    { MP_ROM_QSTR(MP_QSTR_xQueueSendToFront),             MP_ROM_PTR(&mp_xQueueSendToFront_obj)             },
    { MP_ROM_QSTR(MP_QSTR_xQueueSendToBack),              MP_ROM_PTR(&mp_xQueueSendToBack_obj)              },
    { MP_ROM_QSTR(MP_QSTR_xQueueSend),                    MP_ROM_PTR(&mp_xQueueSend_obj)                    },
    { MP_ROM_QSTR(MP_QSTR_xQueueOverwrite),               MP_ROM_PTR(&mp_xQueueOverwrite_obj)               },
    { MP_ROM_QSTR(MP_QSTR_xQueuePeek),                    MP_ROM_PTR(&mp_xQueuePeek_obj)                    },
    { MP_ROM_QSTR(MP_QSTR_xQueuePeekFromISR),             MP_ROM_PTR(&mp_xQueuePeekFromISR_obj)             },
    { MP_ROM_QSTR(MP_QSTR_xQueueReceive),                 MP_ROM_PTR(&mp_xQueueReceive_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_uxQueueMessagesWaiting),        MP_ROM_PTR(&mp_uxQueueMessagesWaiting_obj)        },
    { MP_ROM_QSTR(MP_QSTR_uxQueueSpacesAvailable),        MP_ROM_PTR(&mp_uxQueueSpacesAvailable_obj)        },
    { MP_ROM_QSTR(MP_QSTR_vQueueDelete),                  MP_ROM_PTR(&mp_vQueueDelete_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_xQueueSendToFrontFromISR),      MP_ROM_PTR(&mp_xQueueSendToFrontFromISR_obj)      },
    { MP_ROM_QSTR(MP_QSTR_xQueueSendToBackFromISR),       MP_ROM_PTR(&mp_xQueueSendToBackFromISR_obj)       },
    { MP_ROM_QSTR(MP_QSTR_xQueueOverwriteFromISR),        MP_ROM_PTR(&mp_xQueueOverwriteFromISR_obj)        },
    { MP_ROM_QSTR(MP_QSTR_xQueueSendFromISR),             MP_ROM_PTR(&mp_xQueueSendFromISR_obj)             },
    { MP_ROM_QSTR(MP_QSTR_xQueueGiveFromISR),             MP_ROM_PTR(&mp_xQueueGiveFromISR_obj)             },
    { MP_ROM_QSTR(MP_QSTR_xQueueReceiveFromISR),          MP_ROM_PTR(&mp_xQueueReceiveFromISR_obj)          },
    { MP_ROM_QSTR(MP_QSTR_xQueueIsQueueEmptyFromISR),     MP_ROM_PTR(&mp_xQueueIsQueueEmptyFromISR_obj)     },
    { MP_ROM_QSTR(MP_QSTR_xQueueIsQueueFullFromISR),      MP_ROM_PTR(&mp_xQueueIsQueueFullFromISR_obj)      },
    { MP_ROM_QSTR(MP_QSTR_uxQueueMessagesWaitingFromISR), MP_ROM_PTR(&mp_uxQueueMessagesWaitingFromISR_obj) },
    { MP_ROM_QSTR(MP_QSTR_xQueueCreateMutexStatic), MP_ROM_PTR(&mp_xQueueCreateMutexStatic_obj) },
    { MP_ROM_QSTR(MP_QSTR_xQueueCreateMutexRecursiveStatic), MP_ROM_PTR(&mp_xQueueCreateMutexRecursiveStatic_obj) },
    { MP_ROM_QSTR(MP_QSTR_xQueueCreateCountingSemaphoreStatic), MP_ROM_PTR(&mp_xQueueCreateCountingSemaphoreStatic_obj) },
    { MP_ROM_QSTR(MP_QSTR_xQueueSemaphoreTake),           MP_ROM_PTR(&mp_xQueueSemaphoreTake_obj)           },
    { MP_ROM_QSTR(MP_QSTR_xQueueGetMutexHolder),          MP_ROM_PTR(&mp_xQueueGetMutexHolder_obj)          },
    { MP_ROM_QSTR(MP_QSTR_xQueueGetMutexHolderFromISR),   MP_ROM_PTR(&mp_xQueueGetMutexHolderFromISR_obj)   },
    { MP_ROM_QSTR(MP_QSTR_xQueueReset),                   MP_ROM_PTR(&mp_xQueueReset_obj)                   },
#if ( configQUEUE_REGISTRY_SIZE > 0 )
    { MP_ROM_QSTR(MP_QSTR_vQueueAddToRegistry),           MP_ROM_PTR(&mp_vQueueAddToRegistry_obj)           },
    { MP_ROM_QSTR(MP_QSTR_vQueueUnregisterQueue),         MP_ROM_PTR(&mp_vQueueUnregisterQueue_obj)         },
    { MP_ROM_QSTR(MP_QSTR_pcQueueGetName),                MP_ROM_PTR(&mp_pcQueueGetName_obj)                },
#endif
    { MP_ROM_QSTR(MP_QSTR_xQueueCreateSet),               MP_ROM_PTR(&mp_xQueueCreateSet_obj)               },
    { MP_ROM_QSTR(MP_QSTR_xQueueAddToSet),                MP_ROM_PTR(&mp_xQueueAddToSet_obj)                },
    { MP_ROM_QSTR(MP_QSTR_xQueueRemoveFromSet),           MP_ROM_PTR(&mp_xQueueRemoveFromSet_obj)           },
    { MP_ROM_QSTR(MP_QSTR_xQueueSelectFromSet),           MP_ROM_PTR(&mp_xQueueSelectFromSet_obj)           },
    { MP_ROM_QSTR(MP_QSTR_xQueueSelectFromSetFromISR),    MP_ROM_PTR(&mp_xQueueSelectFromSetFromISR_obj)    },

    // semphr
    { MP_ROM_QSTR(MP_QSTR_vSemaphoreDelete),                     MP_ROM_PTR(&mp_vQueueDelete_obj)                         },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreGetMutexHolder),             MP_ROM_PTR(&mp_xQueueGetMutexHolder_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreGetMutexHolderFromISR),      MP_ROM_PTR(&mp_xQueueGetMutexHolderFromISR_obj)          },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreCreateBinaryStatic),         MP_ROM_PTR(&mp_xSemaphoreCreateBinaryStatic_obj)         },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreTake),                       MP_ROM_PTR(&mp_xSemaphoreTake_obj)                       },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreTakeRecursive),              MP_ROM_PTR(&mp_xSemaphoreTakeRecursive_obj)              },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreGive),                       MP_ROM_PTR(&mp_xSemaphoreGive_obj)                       },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreGiveRecursive),              MP_ROM_PTR(&mp_xSemaphoreGiveRecursive_obj)              },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreGiveFromISR),                MP_ROM_PTR(&mp_xSemaphoreGiveFromISR_obj)                },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreTakeFromISR),                MP_ROM_PTR(&mp_xSemaphoreTakeFromISR_obj)                },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreCreateMutexStatic),          MP_ROM_PTR(&mp_xSemaphoreCreateMutexStatic_obj)          },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreCreateRecursiveMutexStatic), MP_ROM_PTR(&mp_xSemaphoreCreateRecursiveMutexStatic_obj) },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreCreateCountingStatic),       MP_ROM_PTR(&mp_xSemaphoreCreateCountingStatic_obj)       },
    { MP_ROM_QSTR(MP_QSTR_uxSemaphoreGetCount),                  MP_ROM_PTR(&mp_uxSemaphoreGetCount_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_uxSemaphoreGetCountFromISR),           MP_ROM_PTR(&mp_uxSemaphoreGetCountFromISR_obj)           },

    //stream_buffer
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferCreateStatic),            MP_ROM_PTR(&mp_xStreamBufferCreateStatic_obj)            },
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferSend),                    MP_ROM_PTR(&mp_xStreamBufferSend_obj)                    },
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferSendFromISR),             MP_ROM_PTR(&mp_xStreamBufferSendFromISR_obj)             },
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferReceive),                 MP_ROM_PTR(&mp_xStreamBufferReceive_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferReceiveFromISR),          MP_ROM_PTR(&mp_xStreamBufferReceiveFromISR_obj)          },
    { MP_ROM_QSTR(MP_QSTR_vStreamBufferDelete),                  MP_ROM_PTR(&mp_vStreamBufferDelete_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferIsFull),                  MP_ROM_PTR(&mp_xStreamBufferIsFull_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferIsEmpty),                 MP_ROM_PTR(&mp_xStreamBufferIsEmpty_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferReset),                   MP_ROM_PTR(&mp_xStreamBufferReset_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferSpacesAvailable),         MP_ROM_PTR(&mp_xStreamBufferSpacesAvailable_obj)         },
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferBytesAvailable),          MP_ROM_PTR(&mp_xStreamBufferBytesAvailable_obj)          },
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferSetTriggerLevel),         MP_ROM_PTR(&mp_xStreamBufferSetTriggerLevel_obj)         },
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferSendCompletedFromISR),    MP_ROM_PTR(&mp_xStreamBufferSendCompletedFromISR_obj)    },
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferReceiveCompletedFromISR), MP_ROM_PTR(&mp_xStreamBufferReceiveCompletedFromISR_obj) },

    //message_buffer
    { MP_ROM_QSTR(MP_QSTR_xMessageBufferCreateStatic),            MP_ROM_PTR(&mp_xMessageBufferCreateStatic_obj)            },
    { MP_ROM_QSTR(MP_QSTR_xMessageBufferSend),                    MP_ROM_PTR(&mp_xMessageBufferSend_obj)                    },
    { MP_ROM_QSTR(MP_QSTR_xMessageBufferSendFromISR),             MP_ROM_PTR(&mp_xMessageBufferSendFromISR_obj)             },
    { MP_ROM_QSTR(MP_QSTR_xMessageBufferReceive),                 MP_ROM_PTR(&mp_xMessageBufferReceive_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_xMessageBufferReceiveFromISR),          MP_ROM_PTR(&mp_xMessageBufferReceiveFromISR_obj)          },
    { MP_ROM_QSTR(MP_QSTR_vMessageBufferDelete),                  MP_ROM_PTR(&mp_vMessageBufferDelete_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_xMessageBufferIsFull),                  MP_ROM_PTR(&mp_xMessageBufferIsFull_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_xMessageBufferIsEmpty),                 MP_ROM_PTR(&mp_xMessageBufferIsEmpty_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_xMessageBufferReset),                   MP_ROM_PTR(&mp_xMessageBufferReset_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_xMessageBufferSpacesAvailable),         MP_ROM_PTR(&mp_xMessageBufferSpacesAvailable_obj)         },
    { MP_ROM_QSTR(MP_QSTR_xMessageBufferNextLengthBytes),         MP_ROM_PTR(&mp_xMessageBufferNextLengthBytes_obj)         },
    { MP_ROM_QSTR(MP_QSTR_xMessageBufferSendCompletedFromISR),    MP_ROM_PTR(&mp_xMessageBufferSendCompletedFromISR_obj)    },
    { MP_ROM_QSTR(MP_QSTR_xMessageBufferReceiveCompletedFromISR), MP_ROM_PTR(&mp_xMessageBufferReceiveCompletedFromISR_obj) },
    
    //portmacro
    { MP_ROM_QSTR(MP_QSTR_portNOP),                             MP_ROM_PTR(&mp_portNOP_obj)                             },
    { MP_ROM_QSTR(MP_QSTR_xPortInIsrContext),                   MP_ROM_PTR(&mp_xPortInIsrContext_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_vPortAssertIfInISR),                  MP_ROM_PTR(&mp_vPortAssertIfInISR_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_xPortInterruptedFromISRContext),      MP_ROM_PTR(&mp_xPortInterruptedFromISRContext_obj)      },
    { MP_ROM_QSTR(MP_QSTR_xPortSetInterruptMaskFromISR),        MP_ROM_PTR(&mp_xPortSetInterruptMaskFromISR_obj)        },
    { MP_ROM_QSTR(MP_QSTR_vPortClearInterruptMaskFromISR),      MP_ROM_PTR(&mp_vPortClearInterruptMaskFromISR_obj)      },
    { MP_ROM_QSTR(MP_QSTR_portMUX_INITIALIZE),                  MP_ROM_PTR(&mp_portMUX_INITIALIZE_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_xPortEnterCriticalTimeout),           MP_ROM_PTR(&mp_xPortEnterCriticalTimeout_obj)           },
    { MP_ROM_QSTR(MP_QSTR_vPortEnterCritical),                  MP_ROM_PTR(&mp_vPortEnterCritical_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_vPortExitCritical),                   MP_ROM_PTR(&mp_vPortExitCritical_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_xPortEnterCriticalTimeoutCompliance), MP_ROM_PTR(&mp_xPortEnterCriticalTimeoutCompliance_obj) },
    { MP_ROM_QSTR(MP_QSTR_vPortEnterCriticalCompliance),        MP_ROM_PTR(&mp_vPortEnterCriticalCompliance_obj)        },
    { MP_ROM_QSTR(MP_QSTR_vPortExitCriticalCompliance),         MP_ROM_PTR(&mp_vPortExitCriticalCompliance_obj)         },
    { MP_ROM_QSTR(MP_QSTR_xPortEnterCriticalTimeoutSafe),       MP_ROM_PTR(&mp_xPortEnterCriticalTimeoutSafe_obj)       },
    { MP_ROM_QSTR(MP_QSTR_vPortEnterCriticalSafe),              MP_ROM_PTR(&mp_vPortEnterCriticalSafe_obj)              },
    { MP_ROM_QSTR(MP_QSTR_vPortExitCriticalSafe),               MP_ROM_PTR(&mp_vPortExitCriticalSafe_obj)               },
    { MP_ROM_QSTR(MP_QSTR_vPortYield),                          MP_ROM_PTR(&mp_vPortYield_obj)                          },
    { MP_ROM_QSTR(MP_QSTR_vPortYieldOtherCore),                 MP_ROM_PTR(&mp_vPortYieldOtherCore_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_xPortGetTickRateHz),                  MP_ROM_PTR(&mp_xPortGetTickRateHz_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_xPortGetCoreID),                      MP_ROM_PTR(&mp_xPortGetCoreID_obj)                      },
    { MP_ROM_QSTR(MP_QSTR_portGET_CORE_ID),                     MP_ROM_PTR(&mp_portGET_CORE_ID_obj)                     },
    { MP_ROM_QSTR(MP_QSTR_portDISABLE_INTERRUPTS),              MP_ROM_PTR(&mp_portDISABLE_INTERRUPTS_obj)              },
    { MP_ROM_QSTR(MP_QSTR_portENABLE_INTERRUPTS),               MP_ROM_PTR(&mp_portENABLE_INTERRUPTS_obj)               },
    { MP_ROM_QSTR(MP_QSTR_portSET_INTERRUPT_MASK_FROM_ISR),     MP_ROM_PTR(&mp_portSET_INTERRUPT_MASK_FROM_ISR_obj)     },
    { MP_ROM_QSTR(MP_QSTR_portCLEAR_INTERRUPT_MASK_FROM_ISR),   MP_ROM_PTR(&mp_portCLEAR_INTERRUPT_MASK_FROM_ISR_obj)   },
    { MP_ROM_QSTR(MP_QSTR_portASSERT_IF_IN_ISR),                MP_ROM_PTR(&mp_portASSERT_IF_IN_ISR_obj)                },
    { MP_ROM_QSTR(MP_QSTR_portCHECK_IF_IN_ISR),                 MP_ROM_PTR(&mp_portCHECK_IF_IN_ISR_obj)                 },
    { MP_ROM_QSTR(MP_QSTR_portTRY_ENTER_CRITICAL),              MP_ROM_PTR(&mp_portTRY_ENTER_CRITICAL_obj)              },
    { MP_ROM_QSTR(MP_QSTR_portENTER_CRITICAL),                  MP_ROM_PTR(&mp_portENTER_CRITICAL_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_portEXIT_CRITICAL),                   MP_ROM_PTR(&mp_portEXIT_CRITICAL_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_portTRY_ENTER_CRITICAL_ISR),          MP_ROM_PTR(&mp_portTRY_ENTER_CRITICAL_ISR_obj)          },
    { MP_ROM_QSTR(MP_QSTR_portENTER_CRITICAL_ISR),              MP_ROM_PTR(&mp_portENTER_CRITICAL_ISR_obj)              },
    { MP_ROM_QSTR(MP_QSTR_portEXIT_CRITICAL_ISR),               MP_ROM_PTR(&mp_portEXIT_CRITICAL_ISR_obj)               },
    { MP_ROM_QSTR(MP_QSTR_portTRY_ENTER_CRITICAL_SAFE),         MP_ROM_PTR(&mp_portTRY_ENTER_CRITICAL_SAFE_obj)         },
    { MP_ROM_QSTR(MP_QSTR_portENTER_CRITICAL_SAFE),             MP_ROM_PTR(&mp_portENTER_CRITICAL_SAFE_obj)             },
    { MP_ROM_QSTR(MP_QSTR_portEXIT_CRITICAL_SAFE),              MP_ROM_PTR(&mp_portEXIT_CRITICAL_SAFE_obj)              },
    { MP_ROM_QSTR(MP_QSTR_portYIELD),                           MP_ROM_PTR(&mp_portYIELD_obj)                           },
    { MP_ROM_QSTR(MP_QSTR__frxt_setup_switch),                  MP_ROM_PTR(&mp__frxt_setup_switch_obj)                  },
    { MP_ROM_QSTR(MP_QSTR_portYIELD_FROM_ISR_NO_ARG),           MP_ROM_PTR(&mp_portYIELD_FROM_ISR_NO_ARG_obj)           },
    { MP_ROM_QSTR(MP_QSTR_portYIELD_FROM_ISR_ARG),              MP_ROM_PTR(&mp_portYIELD_FROM_ISR_ARG_obj)              },
    { MP_ROM_QSTR(MP_QSTR_portYIELD_WITHIN_API),                MP_ROM_PTR(&mp_portYIELD_WITHIN_API_obj)                },
    { MP_ROM_QSTR(MP_QSTR_portYIELD_CORE),                      MP_ROM_PTR(&mp_portYIELD_CORE_obj)                      },
    { MP_ROM_QSTR(MP_QSTR_portGET_RUN_TIME_COUNTER_VALUE),      MP_ROM_PTR(&mp_portGET_RUN_TIME_COUNTER_VALUE_obj)      },

#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
    { MP_ROM_QSTR(MP_QSTR_portRECORD_READY_PRIORITY),           MP_ROM_PTR(&mp_portRECORD_READY_PRIORITY_obj)           },
    { MP_ROM_QSTR(MP_QSTR_portRESET_READY_PRIORITY),            MP_ROM_PTR(&mp_portRESET_READY_PRIORITY_obj)            },
    { MP_ROM_QSTR(MP_QSTR_portGET_HIGHEST_PRIORITY),            MP_ROM_PTR(&mp_portGET_HIGHEST_PRIORITY_obj)            },
#endif

    { MP_ROM_QSTR(MP_QSTR_os_task_switch_is_pended),            MP_ROM_PTR(&mp_os_task_switch_is_pended_obj)            },

    // projdefs
    { MP_ROM_QSTR(MP_QSTR_pdMS_TO_TICKS), MP_ROM_PTR(&mp_pdMS_TO_TICKS_obj) },
    { MP_ROM_QSTR(MP_QSTR_pdTICKS_TO_MS), MP_ROM_PTR(&mp_pdTICKS_TO_MS_obj) },

    // idf_aditions
    { MP_ROM_QSTR(MP_QSTR_xTaskCreateStaticPinnedToCore),          MP_ROM_PTR(&mp_xTaskCreateStaticPinnedToCore_obj)          },
    { MP_ROM_QSTR(MP_QSTR_xTaskGetCoreID),                         MP_ROM_PTR(&mp_xTaskGetCoreID_obj)                         },

#if !CONFIG_FREERTOS_SMP
#if INCLUDE_xTaskGetIdleTaskHandle == 1
    { MP_ROM_QSTR(MP_QSTR_xTaskGetIdleTaskHandleForCore),          MP_ROM_PTR(&mp_xTaskGetIdleTaskHandleForCore_obj)          },
#endif
#if INCLUDE_xTaskGetIdleTaskHandle == 1 || configUSE_MUTEXES == 1
    { MP_ROM_QSTR(MP_QSTR_xTaskGetCurrentTaskHandleForCore),       MP_ROM_PTR(&mp_xTaskGetCurrentTaskHandleForCore_obj)       },
#endif
#endif

    { MP_ROM_QSTR(MP_QSTR_xTaskCreatePinnedToCoreWithCaps),        MP_ROM_PTR(&mp_xTaskCreatePinnedToCoreWithCaps_obj)        },
    { MP_ROM_QSTR(MP_QSTR_xTaskCreateWithCaps),                    MP_ROM_PTR(&mp_xTaskCreateWithCaps_obj)                    },
    { MP_ROM_QSTR(MP_QSTR_vTaskDeleteWithCaps),                    MP_ROM_PTR(&mp_vTaskDeleteWithCaps_obj)                    },
    { MP_ROM_QSTR(MP_QSTR_xQueueCreateWithCaps),                   MP_ROM_PTR(&mp_xQueueCreateWithCaps_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_vQueueDeleteWithCaps),                   MP_ROM_PTR(&mp_vQueueDeleteWithCaps_obj)                   },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreCreateBinaryWithCaps),         MP_ROM_PTR(&mp_xSemaphoreCreateBinaryWithCaps_obj)         },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreCreateCountingWithCaps),       MP_ROM_PTR(&mp_xSemaphoreCreateCountingWithCaps_obj)       },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreCreateMutexWithCaps),          MP_ROM_PTR(&mp_xSemaphoreCreateMutexWithCaps_obj)          },
    { MP_ROM_QSTR(MP_QSTR_xSemaphoreCreateRecursiveMutexWithCaps), MP_ROM_PTR(&mp_xSemaphoreCreateRecursiveMutexWithCaps_obj) },
    { MP_ROM_QSTR(MP_QSTR_vSemaphoreDeleteWithCaps),               MP_ROM_PTR(&mp_vSemaphoreDeleteWithCaps_obj)               },
    { MP_ROM_QSTR(MP_QSTR_xStreamBufferCreateWithCaps),            MP_ROM_PTR(&mp_xStreamBufferCreateWithCaps_obj)            },
    { MP_ROM_QSTR(MP_QSTR_vStreamBufferDeleteWithCaps),            MP_ROM_PTR(&mp_vStreamBufferDeleteWithCaps_obj)            },
    { MP_ROM_QSTR(MP_QSTR_xMessageBufferCreateWithCaps),           MP_ROM_PTR(&mp_xMessageBufferCreateWithCaps_obj)           },
    { MP_ROM_QSTR(MP_QSTR_vMessageBufferDeleteWithCaps),           MP_ROM_PTR(&mp_vMessageBufferDeleteWithCaps_obj)           },
    { MP_ROM_QSTR(MP_QSTR_xEventGroupCreateWithCaps),              MP_ROM_PTR(&mp_xEventGroupCreateWithCaps_obj)              },
    { MP_ROM_QSTR(MP_QSTR_vEventGroupDeleteWithCaps),              MP_ROM_PTR(&mp_vEventGroupDeleteWithCaps_obj)              },

    // constants
    { MP_ROM_QSTR(MP_QSTR_tskIDLE_PRIORITY),                       MP_ROM_INT(tskIDLE_PRIORITY)                      },
    { MP_ROM_QSTR(MP_QSTR_tskNO_AFFINITY),                         MP_ROM_INT(tskNO_AFFINITY)                        },
    { MP_ROM_QSTR(MP_QSTR_taskSCHEDULER_SUSPENDED),                MP_ROM_INT(taskSCHEDULER_SUSPENDED)               },
    { MP_ROM_QSTR(MP_QSTR_taskSCHEDULER_NOT_STARTED),              MP_ROM_INT(taskSCHEDULER_NOT_STARTED)             },
    { MP_ROM_QSTR(MP_QSTR_taskSCHEDULER_RUNNING),                  MP_ROM_INT(taskSCHEDULER_RUNNING)                 },
    { MP_ROM_QSTR(MP_QSTR_pdFALSE),                                MP_ROM_INT(pdFALSE)                               },
    { MP_ROM_QSTR(MP_QSTR_pdTRUE),                                 MP_ROM_INT(pdTRUE)                                },
    { MP_ROM_QSTR(MP_QSTR_pdPASS),                                 MP_ROM_INT(pdPASS)                                },
    { MP_ROM_QSTR(MP_QSTR_pdFAIL),                                 MP_ROM_INT(pdFAIL)                                },
    { MP_ROM_QSTR(MP_QSTR_errQUEUE_EMPTY),                         MP_ROM_INT(errQUEUE_EMPTY)                        },
    { MP_ROM_QSTR(MP_QSTR_errQUEUE_FULL),                          MP_ROM_INT(errQUEUE_FULL)                         },
    { MP_ROM_QSTR(MP_QSTR_errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY),  MP_ROM_INT(errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY) },
    { MP_ROM_QSTR(MP_QSTR_errQUEUE_BLOCKED),                       MP_ROM_INT(errQUEUE_BLOCKED)                      },
    { MP_ROM_QSTR(MP_QSTR_errQUEUE_YIELD),                         MP_ROM_INT(errQUEUE_YIELD)                        },

    // portmacro constants
    { MP_ROM_QSTR(MP_QSTR_portMAX_DELAY),               MP_ROM_INT(portMAX_DELAY)               },
    { MP_ROM_QSTR(MP_QSTR_portCRITICAL_NESTING_IN_TCB), MP_ROM_INT(portCRITICAL_NESTING_IN_TCB) },
    { MP_ROM_QSTR(MP_QSTR_portSTACK_GROWTH),            MP_ROM_INT(portSTACK_GROWTH)            },
    { MP_ROM_QSTR(MP_QSTR_portTICK_PERIOD_MS),          MP_ROM_INT(portTICK_PERIOD_MS)          },
    { MP_ROM_QSTR(MP_QSTR_portBYTE_ALIGNMENT),          MP_ROM_INT(portBYTE_ALIGNMENT)          },
    { MP_ROM_QSTR(MP_QSTR_portTICK_TYPE_IS_ATOMIC),     MP_ROM_INT(portTICK_TYPE_IS_ATOMIC)     },

    // atomic constants
    { MP_ROM_QSTR(MP_QSTR_ATOMIC_COMPARE_AND_SWAP_SUCCESS),        MP_ROM_INT(ATOMIC_COMPARE_AND_SWAP_SUCCESS)       },
    { MP_ROM_QSTR(MP_QSTR_ATOMIC_COMPARE_AND_SWAP_FAILURE),        MP_ROM_INT(ATOMIC_COMPARE_AND_SWAP_FAILURE)       },

    // eTaskState
    { MP_ROM_QSTR(MP_QSTR_eRunning),                  MP_ROM_INT(eRunning)                  },
    { MP_ROM_QSTR(MP_QSTR_eReady),                    MP_ROM_INT(eReady)                    },
    { MP_ROM_QSTR(MP_QSTR_eBlocked),                  MP_ROM_INT(eBlocked)                  },
    { MP_ROM_QSTR(MP_QSTR_eSuspended),                MP_ROM_INT(eSuspended)                },
    { MP_ROM_QSTR(MP_QSTR_eDeleted),                  MP_ROM_INT(eDeleted)                  },
    { MP_ROM_QSTR(MP_QSTR_eInvalid),                  MP_ROM_INT(eInvalid)                  },

    // eNotifyAction
    { MP_ROM_QSTR(MP_QSTR_eNoAction),                 MP_ROM_INT(eNoAction)                 },
    { MP_ROM_QSTR(MP_QSTR_eSetBits),                  MP_ROM_INT(eSetBits)                  },
    { MP_ROM_QSTR(MP_QSTR_eIncrement),                MP_ROM_INT(eIncrement)                },
    { MP_ROM_QSTR(MP_QSTR_eSetValueWithOverwrite),    MP_ROM_INT(eSetValueWithOverwrite)    },
    { MP_ROM_QSTR(MP_QSTR_eSetValueWithoutOverwrite), MP_ROM_INT(eSetValueWithoutOverwrite) },

    // eSleepModeStatus
    { MP_ROM_QSTR(MP_QSTR_eAbortSleep),               MP_ROM_INT(eAbortSleep)                },
    { MP_ROM_QSTR(MP_QSTR_eStandardSleep),            MP_ROM_INT(eStandardSleep)             },
#if ( INCLUDE_vTaskSuspend == 1 )
    { MP_ROM_QSTR(MP_QSTR_eNoTasksWaitingTimeout),    MP_ROM_INT(eNoTasksWaitingTimeout)     },
#endif
} ;


static MP_DEFINE_CONST_DICT(freertos_globals, freertos_globals_table);


const mp_obj_module_t module_freertos = {
    .base    = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&freertos_globals,
};

MP_REGISTER_MODULE(MP_QSTR_FreeRTOS, module_freertos);


