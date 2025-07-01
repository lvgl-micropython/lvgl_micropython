

from typing import NewType, Final, Callable, Any

BaseType_t = NewType('BaseType_t', int)

pdFALSE: Final[BaseType_t] = ...
pdTRUE: Final[BaseType_t] = ...
pdPASS: Final[BaseType_t] = ...
pdFAIL: Final[BaseType_t] = ...

errQUEUE_EMPTY: Final[BaseType_t] = ...
errQUEUE_FULL: Final[BaseType_t] = ...
errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY: Final[int] = ...
errQUEUE_BLOCKED: Final[int] = ...
errQUEUE_YIELD: Final[int] = ...

PortConstant = NewType('PortConstant', int)

portMAX_DELAY: Final[PortConstant] = ...

portCRITICAL_NESTING_IN_TCB: Final[PortConstant] = ...
portSTACK_GROWTH: Final[PortConstant] = ...
portTICK_PERIOD_MS: Final[PortConstant] = ...
portBYTE_ALIGNMENT: Final[PortConstant] = ...
portTICK_TYPE_IS_ATOMIC: Final[PortConstant] = ...

UBaseType_t = NewType('UBaseType_t', int)

tskIDLE_PRIORITY: Final[UBaseType_t] = ...
tskNO_AFFINITY: Final[UBaseType_t] = ...

taskSCHEDULER_SUSPENDED: Final[int] = ...
taskSCHEDULER_NOT_STARTED: Final[int] = ...
taskSCHEDULER_RUNNING: Final[int] = ...


TaskFunction_t = NewType('TaskFunction_t', Callable[[Any], None])

task_t = NewType('task_t', object)
semaphore_t = NewType('semaphore_t', object)


# atomic

def ATOMIC_ENTER_CRITICAL() -> int:
    ...


def ATOMIC_EXIT_CRITICAL(uxCriticalSectionType: int, /) -> None:
    ...


# task notifications


eTaskState = NewType('eTaskState', int)

eRunning: Final[eTaskState] = ...
eReady: Final[eTaskState] = ...
eBlocked: Final[eTaskState] = ...
eSuspended: Final[eTaskState] = ...
eDeleted: Final[eTaskState] = ...
eInvalid: Final[eTaskState] = ...

eNotifyAction = NewType('eNotifyAction', int)

eNoAction: Final[eNotifyAction] = ...
eSetBits: Final[eNotifyAction] = ...
eIncrement: Final[eNotifyAction] = ...
eSetValueWithOverwrite: Final[eNotifyAction] = ...
eSetValueWithoutOverwrite: Final[eNotifyAction] = ...

eSleepModeStatus = NewType('eSleepModeStatus', int)

eAbortSleep: Final[eSleepModeStatus] = ...
eStandardSleep: Final[eSleepModeStatus] = ...
eNoTasksWaitingTimeout: Final[eSleepModeStatus] = ...


def xTaskNotifyGive(xTaskToNotify: task_t, /) -> int:
    ...


def xTaskNotifyGiveIndexed(xTaskToNotify: task_t, uxIndexToNotify: int, /) -> int:
    ...


def vTaskNotifyGiveFromISR(xTaskToNotify: task_t, /) -> int:
    ...


def vTaskNotifyGiveIndexedFromISR(xTaskToNotify: task_t, uxIndexToNotify: int, /) -> int:
    ...


def ulTaskNotifyTake(xClearCountOnExit: BaseType_t, xTicksToWait: int, /) -> int:
    ...


def ulTaskNotifyTakeIndexed(uxIndexToWaitOn: int, xClearCountOnExit: BaseType_t, xTicksToWait: int, /) -> int:
    ...


def xTaskNotify(xTaskToNotify: task_t, ulValue: int, eAction: eNotifyAction, /) -> int:
    ...


def xTaskNotifyWaitIndexed(uxIndexToWaitOn: int, ulBitsToClearOnEntry: int, ulBitsToClearOnExit: int, xTicksToWait: int, /) -> tuple[int, int]:
    ...


def xTaskNotifyWait(ulBitsToClearOnEntry: int, ulBitsToClearOnExit: int, xTicksToWait: int, /) -> tuple[int, int]:
    ...


def xTaskNotifyIndexedFromISR(xTaskToNotify: task_t, uxIndexToNotify: int, ulValue: int, eAction: eNotifyAction, /) -> tuple[int, int]:
    ...


def xTaskNotifyFromISR(xTaskToNotify: task_t, ulValue: int, eAction: eNotifyAction, /) -> tuple[int, int]:
    ...


def xTaskNotifyAndQueryIndexedFromISR(xTaskToNotify: task_t, uxIndexToNotify: int, ulValue: int, eAction: eNotifyAction, /) -> tuple[int, int, int]:
    ...


def xTaskNotifyAndQueryFromISR(xTaskToNotify: task_t, ulValue: int, eAction: eNotifyAction, /) -> tuple[int, int, int]:
    ...


def xTaskNotifyAndQueryIndexed(xTaskToNotify: task_t, uxIndexToNotify: int, ulValue: int, eAction: eNotifyAction, /) -> tuple[int, int]:
    ...


def xTaskNotifyIndexed(xTaskToNotify: task_t, uxIndexToNotify: int, ulValue: int, eAction: eNotifyAction, /) -> int:
    ...


def xTaskNotifyAndQuery(xTaskToNotify: task_t, ulValue: int, eAction: eNotifyAction, /) -> tuple[int, int]:
    ...


def xTaskNotifyStateClear(xTask: task_t, /) -> int:
    ...


def xTaskNotifyStateClearIndexed(xTask: task_t, uxIndexToClear: int, /) -> int:
    ...


def ulTaskNotifyValueClear(xTask: task_t, ulBitsToClear: int, /) -> int:
    ...


def ulTaskNotifyValueClearIndexed(xTask: task_t, uxIndexToClear: int, ulBitsToClear: int, /) -> int:
    ...


# timers


timer_t = NewType('timer_t', object)

def xTimerCreateStatic(pcTimerName: str, xTimerPeriodInTicks: int, xAutoReload: BaseType_t, callback: Callable[[Any], None], /) -> timer_t:
    ...


def pvTimerGetTimerID(xTimer: timer_t, /) -> int:
    ...


def vTimerSetTimerID(xTimer: timer_t, pvTimerID: int, /) -> None:
    ...


def xTimerIsTimerActive(xTimer: timer_t, /) -> BaseType_t:
    ...


def xTimerStart(xTimer: timer_t, xTicksToWait: int, /) -> int:
    ...


def xTimerStop(xTimer: timer_t, xTicksToWait: int, /) -> int:
    ...


def xTimerChangePeriod(xTimer: timer_t, xNewPeriod: int, xTicksToWait: int, /) -> int:
    ...


def xTimerDelete(xTimer: timer_t, xTicksToWait: int, /) -> int:
    ...


def xTimerReset(xTimer: timer_t, xTicksToWait: int, /) -> int:
    ...


def xTimerStartFromISR(xTimer: timer_t, /) -> tuple[int, int]:
    ...


def xTimerStopFromISR(xTimer: timer_t, /) -> tuple[int, int]:
    ...


def xTimerChangePeriodFromISR(xTimer: timer_t, xNewPeriod: int, /) -> tuple[int, int]:
    ...


def xTimerResetFromISR(xTimer: timer_t, /) -> tuple[int, int]:
    ...


def pcTimerGetName(xTimer: timer_t, /) -> str:
    ...


def vTimerSetReloadMode(xTimer: timer_t, xAutoReload: BaseType_t, /) -> None:
    ...


def xTimerGetReloadMode(xTimer: timer_t, /) -> BaseType_t:
    ...


def uxTimerGetReloadMode(xTimer: timer_t, /) -> UBaseType_t:
    ...


def xTimerGetPeriod(xTimer: timer_t, /) -> UBaseType_t:
    ...


def xTimerGetExpiryTime(xTimer: timer_t, /) -> UBaseType_t:
    ...


def xTimerCreateTimerTask() -> int:
    ...



#if ( configUSE_TRACE_FACILITY == 1 )
def vTimerSetTimerNumber(xTimer: timer_t, uxTimerNumber: int, /) -> None:
    ...


def uxTimerGetTimerNumber(xTimer: timer_t, /) -> int:
    ...


#endif

# task

def taskVALID_CORE_ID(xCoreID: int, /) -> None:
    ...


def taskYIELD() -> None:
    ...


def taskENTER_CRITICAL(x: semaphore_t, /) -> None:
    ...


def taskENTER_CRITICAL_FROM_ISR() -> None:
    ...


def taskENTER_CRITICAL_ISR(x: semaphore_t, /) -> None:
    ...


def taskEXIT_CRITICAL(x: semaphore_t, /) -> None:
    ...


def taskEXIT_CRITICAL_FROM_ISR(prev_level: int, /) -> None:
    ...


def taskEXIT_CRITICAL_ISR(x: semaphore_t, /) -> None:
    ...


def taskDISABLE_INTERRUPTS() -> None:
    ...


def taskENABLE_INTERRUPTS() -> None:
    ...



def xTaskCreateStatic(pxTaskCode: TaskFunction_t, pcName: str,
                      uxStackDepth, pvParameters: Any, uxPriority: int,
                      puxStackBuffer: tuple | None, /) -> task_t:
    ...


def vTaskDelete(xTaskToDelete: task_t | None, /) -> None:
    ...


def vTaskDelay(xTicksToDelay: int, /) -> None:
    ...


def xTaskDelayUntil(xTimeIncrement: int, /) -> tuple[int, int]:
    ...


def vTaskDelayUntil(xTimeIncrement: int, /) -> int:
    ...


def xTaskAbortDelay(xTask: task_t, /) -> int:
    ...


def uxTaskPriorityGet(xTask: task_t, /) -> int:
    ...


def uxTaskPriorityGetFromISR(xTask: task_t, /) -> int:
    ...


def eTaskGetState(xTask: task_t, /) -> eTaskState:
    ...


def vTaskPrioritySet(xTask: task_t, uxNewPriority: int, /) -> None:
    ...


def vTaskSuspend(xTaskToResume: task_t, /) -> None:
    ...


def vTaskResume(xTaskToSuspend: task_t, /) -> None:
    ...


def xTaskResumeFromISR(xTaskToSuspend: task_t, /) -> int:
    ...


def vTaskStartScheduler() -> None:
    ...


def vTaskEndScheduler() -> None:
    ...


def vTaskSuspendAll() -> None:
    ...


def xTaskResumeAll() -> int:
    ...


def xTaskGetTickCount() -> int:
    ...


def xTaskGetTickCountFromISR() -> int:
    ...


def uxTaskGetNumberOfTasks() -> int:
    ...


def pcTaskGetName(xTaskToQuery: task_t, /) -> str:
    ...


def uxTaskGetStackHighWaterMark(xTask: task_t, /) -> int:
    ...


def vTaskSetThreadLocalStoragePointer(xTaskToSet: task_t, xIndex: int, pvValue: Any, /) -> None:
    ...


def pvTaskGetThreadLocalStoragePointer(xTaskToQuery: task_t, xIndex: int, /) -> Any:
    ...


def vApplicationStackOverflowHook(xTask: task_t, pcTaskName: str, /) -> None:
    ...


def xTaskCatchUpTicks(xTicksToCatchUp: int, /) -> int:
    ...


def xTaskIncrementTick() -> int:
    ...


def vTaskSwitchContext() -> None:
    ...


def vTaskMissedYield() -> None:
    ...


def xTaskGetSchedulerState() -> int:
    ...


def xTaskPriorityInherit(pxMutexHolder: task_t, /) -> int:
    ...


def xTaskPriorityDisinherit(pxMutexHolder: task_t, /) -> int:
    ...


def vTaskPriorityDisinheritAfterTimeout(pxMutexHolder: task_t, uxHighestPriorityWaitingTask: int, /) -> None:
    ...



# event groups

event_group_t = NewType('event_group_t', object)


def xEventGroupCreateStatic() -> event_group_t:
    ...


def xEventGroupWaitBits(xEventGroup: event_group_t, uxBitsToWaitFor: int,
                        xClearOnExit: BaseType_t, xWaitForAllBits: BaseType_t,
                        xTicksToWait: int, /) -> int:
    ...

def xEventGroupClearBits(xEventGroup: event_group_t, uxBitsToClear: int, /) -> int:
    ...


def xEventGroupClearBitsFromISR(xEventGroup: event_group_t, uxBitsToClear: int, /) -> int:
    ...


def xEventGroupSetBits(xEventGroup: event_group_t, uxBitsToSet: int, /) -> int:
    ...


def xEventGroupSetBitsFromISR(xEventGroup: event_group_t, uxBitsToSet: int, /) -> tuple[int, int]:
    ...


def xEventGroupSync(xEventGroup: event_group_t, uxBitsToSet: int, uxBitsToWaitFor: int, xTicksToWait: int, /) -> int:
    ...


def xEventGroupGetBits(xEventGroup: event_group_t, /) -> int:
    ...


def xEventGroupGetBitsFromISR(xEventGroup: event_group_t, /) -> int:
    ...


def vEventGroupDelete(xEventGroup: event_group_t, /) -> None:
    ...




# queues

queue_t = NewType('queue_t', object)


def xQueueCreateStatic(uxQueueLength: int, /) -> queue_t:
    ...


def xQueueSendToFront(xQueue: queue_t, pvItemToQueue: Any, xTicksToWait: int, /) -> int:
    ...


def xQueueSendToBack(xQueue: queue_t, pvItemToQueue: Any, xTicksToWait: int, /) -> int:
    ...


def xQueueSend(xQueue: queue_t, pvItemToQueue: Any, xTicksToWait: int, /) -> int:
    ...


def xQueueOverwrite(xQueue: queue_t, pvItemToQueue: Any, /) -> int:
    ...


def xQueuePeek(xQueue: queue_t, xTicksToWait: int, /) -> tuple[int, Any]:
    ...


def xQueuePeekFromISR(xQueue: queue_t, /) -> tuple[int, Any]:
    ...


def xQueueReceive(xQueue: queue_t, xTicksToWait: int, /) -> tuple[int, Any]:
    ...


def uxQueueMessagesWaiting(xQueue: queue_t, /) -> int:
    ...


def uxQueueSpacesAvailable(xQueue: queue_t, /) -> int:
    ...


def vQueueDelete(xQueue: queue_t, /) -> None:
    ...


def xQueueSendToFrontFromISR(xQueue: queue_t, pvItemToQueue: Any, /) -> tuple[int, int]:
    ...


def xQueueSendToBackFromISR(xQueue: queue_t, pvItemToQueue: Any, /) -> tuple[int, int]:
    ...


def xQueueOverwriteFromISR(xQueue: queue_t, pvItemToQueue: Any, /) -> tuple[int, int]:
    ...


def xQueueSendFromISR(xQueue: queue_t, pvItemToQueue: Any, /) -> tuple[int, int]:
    ...


def xQueueGiveFromISR(xQueue: queue_t, /) -> tuple[int, int]:
    ...


def xQueueReceiveFromISR(xQueue: queue_t, /) -> tuple[int, Any, int]:
    ...


def xQueueIsQueueEmptyFromISR(xQueue: queue_t, /) -> int:
    ...


def xQueueIsQueueFullFromISR(xQueue: queue_t, /) -> int:
    ...


def uxQueueMessagesWaitingFromISR(xQueue: queue_t, /) -> int:
    ...


def xQueueCreateMutexStatic() -> queue_t:
    ...


def xQueueCreateMutexRecursiveStatic() -> queue_t:
    ...


def xQueueCreateCountingSemaphoreStatic() -> queue_t:
    ...


def xQueueSemaphoreTake(xSemaphore: queue_t, xTicksToWait: int, /) -> int:
    ...


def xQueueGetMutexHolder(xSemaphore: queue_t, /) -> task_t:
    ...


def xQueueGetMutexHolderFromISR(xSemaphore: queue_t, /) -> task_t:
    ...


def xQueueReset(xQueue: queue_t, /) -> int:
    ...


#if ( configQUEUE_REGISTRY_SIZE > 0 )
def vQueueAddToRegistry(xQueue: queue_t, pcQueueName: str, /) -> None:
    ...


def vQueueUnregisterQueue(xQueue: queue_t, /) -> None:
    ...


def pcQueueGetName(xQueue: queue_t, /) -> str:
    ...


#endif
def xQueueCreateSet(uxEventQueueLength: int, /) -> queue_t:
    ...


def xQueueAddToSet(xQueueOrSemaphore: queue_t, xQueueSet: queue_t, /) -> int:
    ...


def xQueueRemoveFromSet(xQueueOrSemaphore: queue_t, xQueueSet: queue_t, /) -> int:
    ...


def xQueueSelectFromSet(xQueueSet: queue_t, xTicksToWait: int, /) -> queue_t:
    ...


def xQueueSelectFromSetFromISR(xQueueSet: queue_t, /) -> queue_t:
    ...



# semphr
def vSemaphoreDelete() -> None:
    ...


def xSemaphoreGetMutexHolder() -> None:
    ...


def xSemaphoreGetMutexHolderFromISR() -> None:
    ...


def xSemaphoreCreateBinaryStatic() -> semaphore_t:
    ...


def xSemaphoreTake(xSemaphore: semaphore_t, xBlockTime: int, /) -> int:
    ...


def xSemaphoreTakeRecursive(xSemaphore: semaphore_t, xBlockTime: int, /) -> int:
    ...


def xSemaphoreGive(xSemaphore: semaphore_t, /) -> int:
    ...


def xSemaphoreGiveRecursive(xSemaphore: semaphore_t, /) -> int:
    ...


def xSemaphoreGiveFromISR(xSemaphore: semaphore_t, /) -> tuple[int, int]:
    ...


def xSemaphoreTakeFromISR(xSemaphore: semaphore_t, /) -> tuple[int, int]:
    ...


def xSemaphoreCreateMutexStatic() -> semaphore_t:
    ...


def xSemaphoreCreateRecursiveMutexStatic() -> semaphore_t:
    ...


def xSemaphoreCreateCountingStatic(uxMaxCount: int, uxInitialCount: int, /) -> semaphore_t:
    ...


def uxSemaphoreGetCount(xSemaphore: semaphore_t, /) -> int:
    ...


def uxSemaphoreGetCountFromISR(xSemaphore: semaphore_t, /) -> int:
    ...



# stream_buffer
stream_buffer_t = NewType('stream_buffer_t', object)

def xStreamBufferCreateStatic(xBufferSizeBytes: int, xTriggerLevelBytes: int, /) -> stream_buffer_t:
    ...


def xStreamBufferSend(xStreamBuffer: stream_buffer_t, pvTxData: memoryview, xTicksToWait: int, /) -> int:
    ...


def xStreamBufferSendFromISR(xStreamBuffer: stream_buffer_t, pvTxData: memoryview, /) -> tuple[int, int]:
    ...


def xStreamBufferReceive(xStreamBuffer: stream_buffer_t, pvRxData: memoryview, xTicksToWait: int, /) -> int:
    ...


def xStreamBufferReceiveFromISR(xStreamBuffer: stream_buffer_t, pvRxData: memoryview, /) -> tuple[int, int]:
    ...


def vStreamBufferDelete(xStreamBuffer: stream_buffer_t, /) -> None:
    ...


def xStreamBufferIsFull(xStreamBuffer: stream_buffer_t, /) -> int:
    ...


def xStreamBufferIsEmpty(xStreamBuffer: stream_buffer_t, /) -> int:
    ...


def xStreamBufferReset(xStreamBuffer: stream_buffer_t, /) -> int:
    ...


def xStreamBufferSpacesAvailable(xStreamBuffer: stream_buffer_t, /) -> int:
    ...


def xStreamBufferBytesAvailable(xStreamBuffer: stream_buffer_t, /) -> int:
    ...


def xStreamBufferSetTriggerLevel(xStreamBuffer: stream_buffer_t, xTriggerLevel: int, /) -> int:
    ...


def xStreamBufferSendCompletedFromISR(xStreamBuffer: stream_buffer_t, /) -> tuple[int, int]:
    ...


def xStreamBufferReceiveCompletedFromISR(xStreamBuffer: stream_buffer_t, /) -> tuple[int, int]:
    ...



# message_buffer
message_buffer_t = NewType('message_buffer_t', object)

def xMessageBufferCreateStatic(xBufferSizeBytes: int, /) -> message_buffer_t:
    ...


def xMessageBufferSend(xMessageBuffer: message_buffer_t, pvTxData: memoryview, xTicksToWait: int, /) -> int:
    ...


def xMessageBufferSendFromISR(xMessageBuffer: message_buffer_t, pvTxData: memoryview, /) -> tuple[int, int]:
    ...


def xMessageBufferReceive(xMessageBuffer: message_buffer_t, pvRxData: memoryview, xTicksToWait: int, /) -> int:
    ...


def xMessageBufferReceiveFromISR(xMessageBuffer: message_buffer_t, pvRxData: memoryview, /) -> tuple[int, int]:
    ...


def vMessageBufferDelete(xMessageBuffer: message_buffer_t, /) -> None:
    ...


def xMessageBufferIsFull(xMessageBuffer: message_buffer_t, /) -> BaseType_t:
    ...


def xMessageBufferIsEmpty(xMessageBuffer: message_buffer_t, /) -> BaseType_t:
    ...


def xMessageBufferReset(xMessageBuffer: message_buffer_t, /) -> int:
    ...


def xMessageBufferSpacesAvailable(xMessageBuffer: message_buffer_t, /) -> int:
    ...


def xMessageBufferNextLengthBytes(xMessageBuffer: message_buffer_t, /) -> int:
    ...


def xMessageBufferSendCompletedFromISR(xMessageBuffer: message_buffer_t, /) -> tuple[int, int]:
    ...


def xMessageBufferReceiveCompletedFromISR(xMessageBuffer: message_buffer_t, /) -> tuple[int, int]:
    ...



# portmacro
def portNOP() -> None:
    ...


def xPortInIsrContext() -> BaseType_t:
    ...


def vPortAssertIfInISR() -> None:
    ...


def xPortInterruptedFromISRContext() -> None:
    ...


def xPortSetInterruptMaskFromISR() -> BaseType_t:
    ...


def vPortClearInterruptMaskFromISR() -> UBaseType_t:
    ...


spinlock_t = NewType('spinlock_t', object)


def portMUX_INITIALIZE() -> spinlock_t:
    ...


def xPortEnterCriticalTimeout(mux: spinlock_t, timeout: int, /) -> int:
    ...


def vPortEnterCritical(mux: spinlock_t, /) -> None:
    ...


def vPortExitCritical(mux: spinlock_t, /) -> None:
    ...


def xPortEnterCriticalTimeoutCompliance(mux: spinlock_t, timeout: int, /) -> BaseType_t:
    ...


def vPortEnterCriticalCompliance(mux: spinlock_t, /) -> None:
    ...


def vPortExitCriticalCompliance(mux: spinlock_t, /) -> None:
    ...


def xPortEnterCriticalTimeoutSafe(mux: spinlock_t, timeout: int, /) -> BaseType_t:
    ...


def vPortEnterCriticalSafe(mux: spinlock_t, /) -> None:
    ...


def vPortExitCriticalSafe(mux: spinlock_t, /) -> None:
    ...


def vPortYield() -> None:
    ...


def vPortYieldOtherCore(coreid: int) -> None:
    ...


def xPortCanYield() -> bool:
    ...


def xPortGetTickRateHz() -> int:
    ...


def xPortGetCoreID() -> int:
    ...


def portGET_CORE_ID() -> int:
    ...


def portDISABLE_INTERRUPTS() -> None:
    ...


def portENABLE_INTERRUPTS() -> None:
    ...


def portSET_INTERRUPT_MASK_FROM_ISR() -> int:
    ...


def portCLEAR_INTERRUPT_MASK_FROM_ISR(prev_level: int) -> None:
    ...


def portASSERT_IF_IN_ISR() -> None:
    ...


def portCHECK_IF_IN_ISR() -> int:
    ...


def portTRY_ENTER_CRITICAL(mux: spinlock_t, timeout: int, /) -> BaseType_t:
    ...


def portENTER_CRITICAL(mux: spinlock_t, /) -> None:
    ...


def portEXIT_CRITICAL(mux: spinlock_t, /) -> None:
    ...


def portTRY_ENTER_CRITICAL_ISR(mux: spinlock_t, timeout: int, /) -> BaseType_t:
    ...


def portENTER_CRITICAL_ISR(mux: spinlock_t, /) -> None:
    ...


def portEXIT_CRITICAL_ISR(mux: spinlock_t, /) -> None:
    ...


def portTRY_ENTER_CRITICAL_SAFE(mux: spinlock_t, timeout: int, /) -> BaseType_t:
    ...


def portENTER_CRITICAL_SAFE(mux: spinlock_t, /) -> None:
    ...


def portEXIT_CRITICAL_SAFE(mux: spinlock_t, /) -> None:
    ...


def portYIELD() -> None:
    ...


def _frxt_setup_switch() -> None:
    ...


def portYIELD_FROM_ISR_NO_ARG() -> None:
    ...

#
# def portYIELD_FROM_ISR_ARG() -> None:
#     ...


def portYIELD_WITHIN_API() -> None:
    ...


def portYIELD_CORE(xCoreID: int, /) -> None:
    ...


def portGET_RUN_TIME_COUNTER_VALUE() -> int:
    ...



#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
def portRECORD_READY_PRIORITY(uxPriority: int, uxReadyPriorities: int, /) -> int:
    ...


def portRESET_READY_PRIORITY(uxPriority: int, uxReadyPriorities: int, /) -> int:
    ...


def portGET_HIGHEST_PRIORITY(uxTopPriority: int, uxReadyPriorities: int, /) -> int:
    ...


#endif

def os_task_switch_is_pended(_cpu_: int) -> bool:
    ...



# projdefs
def pdMS_TO_TICKS(xTimeInMs: int, /) -> int:
    ...


def pdTICKS_TO_MS(xTimeInTicks: int, /) -> int:
    ...


# idf_aditions
def xTaskCreateStaticPinnedToCore(pxTaskCode: TaskFunction_t, pcName: str,
                      uxStackDepth: int, pvParameters: Any, uxPriority: int,
                      puxStackBuffer: tuple | None, xCoreID: int, /) -> task_t:
    ...


def xTaskGetCoreID(xTask: task_t, /) -> int:
    ...



#if !CONFIG_FREERTOS_SMP
#if INCLUDE_xTaskGetIdleTaskHandle == 1
def xTaskGetIdleTaskHandleForCore(xCoreID: int, /) -> task_t:
    ...


#endif
#if INCLUDE_xTaskGetIdleTaskHandle == 1 || configUSE_MUTEXES == 1
def xTaskGetCurrentTaskHandleForCore(xCoreID: int, /) -> task_t:
    ...


#endif
#endif

def xTaskCreatePinnedToCoreWithCaps(pxTaskCode: TaskFunction_t, pcName: str,
                                    uxStackDepth: int, pvParameters: Any, uxPriority: int,
                                    xCoreID: int, uxMemoryCaps: int, /) -> task_t:
    ...


def xTaskCreateWithCaps(pxTaskCode: TaskFunction_t, pcName: str,
                        uxStackDepth: int, pvParameters: Any, uxPriority: int,
                        uxMemoryCaps: int, /) -> task_t:
    ...


def vTaskDeleteWithCaps(xTaskToDelete: task_t, /) -> None:
    ...


def xQueueCreateWithCaps(uxQueueLength: int, uxMemoryCaps: int, /) -> queue_t:
    ...


def vQueueDeleteWithCaps(xQueue: queue_t, /) -> None:
    ...


def xSemaphoreCreateBinaryWithCaps(uxMemoryCaps: int, /) -> semaphore_t:
    ...


def xSemaphoreCreateCountingWithCaps(uxMaxCount: int, uxInitialCount: int, uxMemoryCaps: int, /) -> semaphore_t:
    ...


def xSemaphoreCreateMutexWithCaps(uxMemoryCaps: int, /) -> semaphore_t:
    ...


def xSemaphoreCreateRecursiveMutexWithCaps(uxMemoryCaps: int, /) -> semaphore_t:
    ...


def vSemaphoreDeleteWithCaps(xSemaphore: semaphore_t, /) -> None:
    ...


def xStreamBufferCreateWithCaps(xBufferSizeBytes: int, xTriggerLevelBytes: int, uxMemoryCaps: int, /) -> stream_buffer_t:
    ...


def vStreamBufferDeleteWithCaps(xStreamBuffer: stream_buffer_t, /) -> None:
    ...


def xMessageBufferCreateWithCaps(xBufferSizeBytes: int, uxMemoryCaps: int, /) -> message_buffer_t:
    ...


def vMessageBufferDeleteWithCaps(xMessageBuffer: message_buffer_t, /) -> None:
    ...


def xEventGroupCreateWithCaps(uxMemoryCaps: int, /) -> event_group_t:
    ...


def vEventGroupDeleteWithCaps(xEventGroup: event_group_t, /) -> None:
    ...


