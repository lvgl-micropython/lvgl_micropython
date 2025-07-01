
#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "mpy_portmacro.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"


static mp_obj_t mp_portNOP(void)
{
//    portNOP();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_portNOP_obj, mp_portNOP);


static mp_obj_t mp_xPortInIsrContext(void)
{
    BaseType_t ret = xPortInIsrContext();
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xPortInIsrContext_obj, mp_xPortInIsrContext);


static mp_obj_t mp_vPortAssertIfInISR(void)
{
    vPortAssertIfInISR();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_vPortAssertIfInISR_obj, mp_vPortAssertIfInISR);


static mp_obj_t mp_xPortInterruptedFromISRContext(void)
{
    BaseType_t ret = xPortInterruptedFromISRContext();
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xPortInterruptedFromISRContext_obj, mp_xPortInterruptedFromISRContext);


static mp_obj_t mp_xPortSetInterruptMaskFromISR(void)
{
    UBaseType_t ret = xPortSetInterruptMaskFromISR();
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xPortSetInterruptMaskFromISR_obj, mp_xPortSetInterruptMaskFromISR);


static mp_obj_t mp_vPortClearInterruptMaskFromISR(mp_obj_t prev_level_in)
{
    UBaseType_t prev_level = (UBaseType_t)mp_obj_get_int_truncated(prev_level_in);
    vPortClearInterruptMaskFromISR(prev_level);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vPortClearInterruptMaskFromISR_obj, mp_vPortClearInterruptMaskFromISR);


static mp_obj_t mp_portMUX_INITIALIZE(void)
{
    mp_obj_freertos_spinlock_t *self = m_new_obj(mp_obj_freertos_spinlock_t);
    self->type = mp_freertos_spinlock_type;
    portMUX_INITIALIZE(self->spinlock.handle);

    return MP_OBJ_FROM_PTR(self);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_portMUX_INITIALIZE_obj, mp_portMUX_INITIALIZE);


static mp_obj_t mp_xPortEnterCriticalTimeout(mp_obj_t mux_in, mp_obj_t timeout_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);
    UBaseType_t timeout = (UBaseType_t)mp_obj_get_int_truncated(timeout_in);

    BaseType_t ret = xPortEnterCriticalTimeout((portMUX_TYPE *)mux->spinlock.handle, timeout);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xPortEnterCriticalTimeout_obj, mp_xPortEnterCriticalTimeout);


static mp_obj_t mp_vPortEnterCritical(mp_obj_t mux_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);

    vPortEnterCritical((portMUX_TYPE *)mux->spinlock.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vPortEnterCritical_obj, mp_vPortEnterCritical);


static mp_obj_t mp_vPortExitCritical(mp_obj_t mux_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);

    vPortExitCritical((portMUX_TYPE *)mux->spinlock.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vPortExitCritical_obj, mp_vPortExitCritical);


static mp_obj_t mp_xPortEnterCriticalTimeoutCompliance(mp_obj_t mux_in, mp_obj_t timeout_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);
    UBaseType_t timeout = (UBaseType_t)mp_obj_get_int_truncated(timeout_in);

    BaseType_t ret = xPortEnterCriticalTimeoutCompliance((portMUX_TYPE *)mux->spinlock.handle, timeout);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xPortEnterCriticalTimeoutCompliance_obj, mp_xPortEnterCriticalTimeoutCompliance);


static mp_obj_t mp_vPortEnterCriticalCompliance(mp_obj_t mux_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);

    vPortEnterCriticalCompliance((portMUX_TYPE *)mux->spinlock.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vPortEnterCriticalCompliance_obj, mp_vPortEnterCriticalCompliance);


static mp_obj_t mp_vPortExitCriticalCompliance(mp_obj_t mux_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);

    vPortExitCriticalCompliance((portMUX_TYPE *)mux->spinlock.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vPortExitCriticalCompliance_obj, mp_vPortExitCriticalCompliance);


static mp_obj_t mp_xPortEnterCriticalTimeoutSafe(mp_obj_t mux_in, mp_obj_t timeout_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);
    UBaseType_t timeout = (UBaseType_t)mp_obj_get_int_truncated(timeout_in);

    BaseType_t ret = xPortEnterCriticalTimeoutSafe((portMUX_TYPE *)mux->spinlock.handle, timeout);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_xPortEnterCriticalTimeoutSafe_obj, mp_xPortEnterCriticalTimeoutSafe);


static mp_obj_t mp_vPortEnterCriticalSafe(mp_obj_t mux_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);

    vPortEnterCriticalSafe((portMUX_TYPE *)mux->spinlock.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vPortEnterCriticalSafe_obj, mp_vPortEnterCriticalSafe);


static mp_obj_t mp_vPortExitCriticalSafe(mp_obj_t mux_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);

    vPortExitCriticalSafe((portMUX_TYPE *)mux->spinlock.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vPortExitCriticalSafe_obj, mp_vPortExitCriticalSafe);


static mp_obj_t mp_vPortYield(void)
{
    vPortYield();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_vPortYield_obj, mp_vPortYield);


static mp_obj_t mp_vPortYieldOtherCore(mp_obj_t coreid_in)
{
    UBaseType_t coreid = (UBaseType_t)mp_obj_get_int_truncated(coreid_in);

    vPortYieldOtherCore(coreid);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_vPortYieldOtherCore_obj, mp_vPortYieldOtherCore);


static mp_obj_t mp_xPortCanYield(void)
{
    if (xPortCanYield()) return mp_const_true;
    else return mp_const_false;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xPortCanYield_obj, mp_xPortCanYield);


static mp_obj_t mp_xPortGetTickRateHz(void)
{
    uint32_t ret = xPortGetTickRateHz();
    return mp_obj_new_int_from_uint(ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xPortGetTickRateHz_obj, mp_xPortGetTickRateHz);


// void vPortSetStackWatchpoint( void *pxStackStart );

static mp_obj_t mp_xPortGetCoreID(void)
{
    BaseType_t ret = xPortGetCoreID();
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_xPortGetCoreID_obj, mp_xPortGetCoreID);


// void vPortTCBPreDeleteHook( void *pxTCB );

static mp_obj_t mp_portGET_CORE_ID(void)
{
    BaseType_t ret = portGET_CORE_ID();
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_portGET_CORE_ID_obj, mp_portGET_CORE_ID);


static mp_obj_t mp_portDISABLE_INTERRUPTS(void)
{
    portDISABLE_INTERRUPTS();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_portDISABLE_INTERRUPTS_obj, mp_portDISABLE_INTERRUPTS);


static mp_obj_t mp_portENABLE_INTERRUPTS(void)
{
    portENABLE_INTERRUPTS();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_portENABLE_INTERRUPTS_obj, mp_portENABLE_INTERRUPTS);


static mp_obj_t mp_portSET_INTERRUPT_MASK_FROM_ISR(void)
{
    UBaseType_t ret = portSET_INTERRUPT_MASK_FROM_ISR();
    return mp_obj_new_int_from_uint((uint32_t)ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_portSET_INTERRUPT_MASK_FROM_ISR_obj, mp_portSET_INTERRUPT_MASK_FROM_ISR);


static mp_obj_t mp_portCLEAR_INTERRUPT_MASK_FROM_ISR(mp_obj_t prev_level_in)
{
    UBaseType_t prev_level = (UBaseType_t)mp_obj_get_int_truncated(prev_level_in);
    portCLEAR_INTERRUPT_MASK_FROM_ISR(prev_level);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_portCLEAR_INTERRUPT_MASK_FROM_ISR_obj, mp_portCLEAR_INTERRUPT_MASK_FROM_ISR);


static mp_obj_t mp_portASSERT_IF_IN_ISR(void)
{
    portASSERT_IF_IN_ISR();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_portASSERT_IF_IN_ISR_obj, mp_portASSERT_IF_IN_ISR);


#if ( configNUM_CORES > 1 )
    static mp_obj_t mp_portCHECK_IF_IN_ISR(void)
    {
        BaseType_t ret = portCHECK_IF_IN_ISR();
        return mp_obj_new_int((int)ret);
    }

    MP_DEFINE_CONST_FUN_OBJ_0(mp_portCHECK_IF_IN_ISR_obj, mp_portCHECK_IF_IN_ISR);

#endif



static mp_obj_t mp_portTRY_ENTER_CRITICAL(mp_obj_t mux_in, mp_obj_t timeout_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);
    UBaseType_t timeout = (UBaseType_t)mp_obj_get_int_truncated(timeout_in);

    BaseType_t ret = portTRY_ENTER_CRITICAL((portMUX_TYPE *)mux->spinlock.handle, timeout);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_portTRY_ENTER_CRITICAL_obj, mp_portTRY_ENTER_CRITICAL);


static mp_obj_t mp_portENTER_CRITICAL(mp_obj_t mux_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);

    portENTER_CRITICAL((portMUX_TYPE *)mux->spinlock.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_portENTER_CRITICAL_obj, mp_portENTER_CRITICAL);


static mp_obj_t mp_portEXIT_CRITICAL(mp_obj_t mux_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);

    portEXIT_CRITICAL((portMUX_TYPE *)mux->spinlock.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_portEXIT_CRITICAL_obj, mp_portEXIT_CRITICAL);


static mp_obj_t mp_portTRY_ENTER_CRITICAL_ISR(mp_obj_t mux_in, mp_obj_t timeout_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);
    UBaseType_t timeout = (UBaseType_t)mp_obj_get_int_truncated(timeout_in);

    BaseType_t ret = portTRY_ENTER_CRITICAL_ISR((portMUX_TYPE *)mux->spinlock.handle, timeout);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_portTRY_ENTER_CRITICAL_ISR_obj, mp_portTRY_ENTER_CRITICAL_ISR);


static mp_obj_t mp_portENTER_CRITICAL_ISR(mp_obj_t mux_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);

    portENTER_CRITICAL_ISR((portMUX_TYPE *)mux->spinlock.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_portENTER_CRITICAL_ISR_obj, mp_portENTER_CRITICAL_ISR);


static mp_obj_t mp_portEXIT_CRITICAL_ISR(mp_obj_t mux_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);

    portEXIT_CRITICAL_ISR((portMUX_TYPE *)mux->spinlock.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_portEXIT_CRITICAL_ISR_obj, mp_portEXIT_CRITICAL_ISR);



static mp_obj_t mp_portTRY_ENTER_CRITICAL_SAFE(mp_obj_t mux_in, mp_obj_t timeout_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);
    UBaseType_t timeout = (UBaseType_t)mp_obj_get_int_truncated(timeout_in);

    BaseType_t ret = portTRY_ENTER_CRITICAL_SAFE((portMUX_TYPE *)mux->spinlock.handle, timeout);
    return mp_obj_new_int((int)ret);
}

MP_DEFINE_CONST_FUN_OBJ_2(mp_portTRY_ENTER_CRITICAL_SAFE_obj, mp_portTRY_ENTER_CRITICAL_SAFE);



static mp_obj_t mp_portENTER_CRITICAL_SAFE(mp_obj_t mux_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);

    portENTER_CRITICAL_SAFE((portMUX_TYPE *)mux->spinlock.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_portENTER_CRITICAL_SAFE_obj, mp_portENTER_CRITICAL_SAFE);


static mp_obj_t mp_portEXIT_CRITICAL_SAFE(mp_obj_t mux_in)
{
    mp_obj_freertos_spinlock_t *mux = MP_OBJ_TO_PTR(mux_in);

    portEXIT_CRITICAL_SAFE((portMUX_TYPE *)mux->spinlock.handle);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_portEXIT_CRITICAL_SAFE_obj, mp_portEXIT_CRITICAL_SAFE);




// ---------------------- Yielding -------------------------

static mp_obj_t mp_portYIELD(void)
{
    portYIELD();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_portYIELD_obj, mp_portYIELD);


static mp_obj_t mp__frxt_setup_switch(void)
{
    _frxt_setup_switch();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp__frxt_setup_switch_obj, mp__frxt_setup_switch);


static mp_obj_t mp_portYIELD_FROM_ISR_NO_ARG(void)
{
    // portYIELD_FROM_ISR_NO_ARG();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_portYIELD_FROM_ISR_NO_ARG_obj, mp_portYIELD_FROM_ISR_NO_ARG);


static mp_obj_t mp_portYIELD_FROM_ISR_ARG(mp_obj_t xHigherPriorityTaskWoken_in)
{
    // BaseType_t xHigherPriorityTaskWoken = (BaseType_t)mp_obj_get_int(xHigherPriorityTaskWoken_in);

    // portYIELD_FROM_ISR_ARG(xHigherPriorityTaskWoken);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_portYIELD_FROM_ISR_ARG_obj, mp_portYIELD_FROM_ISR_ARG);


// void portYIELD_FROM_ISR(possible arg)

static mp_obj_t mp_portYIELD_WITHIN_API(void)
{
    portYIELD_WITHIN_API();
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_portYIELD_WITHIN_API_obj, mp_portYIELD_WITHIN_API);


#if ( configNUMBER_OF_CORES > 1 )
    static mp_obj_t mp_portYIELD_CORE(mp_obj_t xCoreID_in)
    {
        BaseType_t xCoreID = (BaseType_t)mp_obj_get_int(xCoreID_in);

        portYIELD_CORE(xCoreID);
        return mp_const_none;
    }

    MP_DEFINE_CONST_FUN_OBJ_1(mp_portYIELD_CORE_obj, mp_portYIELD_CORE);


#endif


static mp_obj_t mp_portGET_RUN_TIME_COUNTER_VALUE(void)
{
    uint32_t ret = portGET_RUN_TIME_COUNTER_VALUE();
    return mp_obj_new_int_from_uint(ret);
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_portGET_RUN_TIME_COUNTER_VALUE_obj, mp_portGET_RUN_TIME_COUNTER_VALUE);


// void portCLEAN_UP_TCB(void * pxTCB )


#if configUSE_PORT_OPTIMISED_TASK_SELECTION == 1
    /* Store/clear the ready priorities in a bit map. */
    static mp_obj_t mp_portRECORD_READY_PRIORITY(mp_obj_t uxPriority_in, mp_obj_t uxReadyPriorities_in)
    {
        UBaseType_t uxPriority = (UBaseType_t)mp_obj_get_int_truncated(uxPriority_in);
        UBaseType_t uxReadyPriorities = (UBaseType_t)mp_obj_get_int_truncated(uxReadyPriorities_in);
        UBaseType_t ret = portRECORD_READY_PRIORITY(uxPriority, uxReadyPriorities);
        return mp_obj_new_int_from_uint((uint32_t)ret);
    }

    MP_DEFINE_CONST_FUN_OBJ_2(mp_portRECORD_READY_PRIORITY_obj, mp_portRECORD_READY_PRIORITY);

    static mp_obj_t mp_portRESET_READY_PRIORITY(mp_obj_t uxPriority_in, mp_obj_t uxReadyPriorities_in)
    {
        UBaseType_t uxPriority = (UBaseType_t)mp_obj_get_int_truncated(uxPriority_in);
        UBaseType_t uxReadyPriorities = (UBaseType_t)mp_obj_get_int_truncated(uxReadyPriorities_in);
        UBaseType_t ret = portRESET_READY_PRIORITY(uxPriority, uxReadyPriorities);
        return mp_obj_new_int_from_uint((uint32_t)ret);
    }

    MP_DEFINE_CONST_FUN_OBJ_2(mp_portRESET_READY_PRIORITY_obj, mp_portRESET_READY_PRIORITY);

    static mp_obj_t mp_portGET_HIGHEST_PRIORITY(mp_obj_t uxTopPriority_in, mp_obj_t uxReadyPriorities_in)
    {
        UBaseType_t uxTopPriority = (UBaseType_t)mp_obj_get_int_truncated(uxTopPriority_in);
        UBaseType_t uxReadyPriorities = (UBaseType_t)mp_obj_get_int_truncated(uxReadyPriorities_in);
        UBaseType_t ret = portGET_HIGHEST_PRIORITY(uxTopPriority, uxReadyPriorities);
        return mp_obj_new_int_from_uint((uint32_t)ret);
    }

    MP_DEFINE_CONST_FUN_OBJ_2(mp_portGET_HIGHEST_PRIORITY_obj, mp_portGET_HIGHEST_PRIORITY);

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION */


// bool xPortCheckValidListMem(const void *ptr);

// bool xPortCheckValidTCBMem(const void *ptr);

// bool xPortcheckValidStackMem(const void *ptr);

// bool portVALID_LIST_MEM(const void *ptr)
// bool portVALID_TCB_MEM(const void *ptr)
// bool portVALID_STACK_MEM(const void *ptr)

static mp_obj_t mp_os_task_switch_is_pended(mp_obj_t _cpu__in)
{
    uint32_t _cpu_ = (uint32_t)mp_obj_get_int_truncated(_cpu__in);

    (void)_cpu_;

    if (os_task_switch_is_pended(_cpu_)) return mp_const_true;
    else return mp_const_false;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_os_task_switch_is_pended_obj, mp_os_task_switch_is_pended);
