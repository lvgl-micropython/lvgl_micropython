#include <stdint.h>
#include <stdlib.h>

#include "freertos_mod.h"

#include "mpy_atomic.h"

#include "py/obj.h"
#include "py/runtime.h"

#include "freertos/FreeRTOS.h"
#include "freertos/atomic.h"


#if defined( portSET_INTERRUPT_MASK_FROM_ISR )

    mp_obj_t _ATOMIC_ENTER_CRITICAL(void)
    {
        UBaseType_t uxCriticalSectionType = portSET_INTERRUPT_MASK_FROM_ISR();
        return mp_obj_new_int_from_uint((uint32_t)uxCriticalSectionType);
    }

    void _ATOMIC_EXIT_CRITICAL(mp_obj_t uxCriticalSectionType_in)
    {
        UBaseType_t uxCriticalSectionType = (UBaseType_t)mp_obj_get_int_truncated(uxCriticalSectionType_in);
        portCLEAR_INTERRUPT_MASK_FROM_ISR(uxCriticalSectionType);
    }

#else

    mp_obj_t _ATOMIC_ENTER_CRITICAL(void)
    {
        ATOMIC_ENTER_CRITICAL();
        return mp_const_none;
    }

    void _ATOMIC_EXIT_CRITICAL(mp_obj_t _)
    {
        ATOMIC_EXIT_CRITICAL();
    }

#endif


mp_obj_t mp_ATOMIC_ENTER_CRITICAL(void)
{
    return _ATOMIC_ENTER_CRITICAL();
}

MP_DEFINE_CONST_FUN_OBJ_0(mp_ATOMIC_ENTER_CRITICAL_obj, mp_ATOMIC_ENTER_CRITICAL);


mp_obj_t mp_ATOMIC_EXIT_CRITICAL(mp_obj_t uxCriticalSectionType)
{
    _ATOMIC_EXIT_CRITICAL(uxCriticalSectionType);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(mp_ATOMIC_EXIT_CRITICAL_obj, mp_ATOMIC_EXIT_CRITICAL);


/*
Performs an atomic compare-and-swap operation on the specified values.

Atomic compare-and-swap

Parameters
    [in,out]	pulDestination	Pointer to memory location from where value is to be loaded and checked.
    [in]	ulExchange	If condition meets, write this value to memory.
    [in]	ulComparand	Swap condition.

Returns
    Unsigned integer of value 1 or 0. 1 for swapped, 0 for not swapped.

Note
    This function only swaps *pulDestination with ulExchange, if previous *pulDestination value equals ulComparand.
*/
// uint32_t Atomic_CompareAndSwap_u32( uint32_t volatile * pulDestination, uint32_t ulExchange, uint32_t ulComparand )


/*
Atomically sets the address pointed to by *ppvDestination to the value of *pvExchange.

Atomic swap (pointers)

Parameters
    [in,out]	ppvDestination	Pointer to memory location from where a pointer value is to be loaded and written back to.
    [in]	pvExchange	Pointer value to be written to *ppvDestination.

Returns
    The initial value of *ppvDestination.
*/
// void * Atomic_SwapPointers_p32( void * volatile * ppvDestination, void * pvExchange )


/*
Performs an atomic compare-and-swap operation on the specified pointer values.

Atomic compare-and-swap (pointers)

Parameters
    [in,out]	ppvDestination	Pointer to memory location from where a pointer value is to be loaded and checked.
    [in]	pvExchange	If condition meets, write this value to memory.
    [in]	pvComparand	Swap condition.

Returns
    Unsigned integer of value 1 or 0. 1 for swapped, 0 for not swapped.

Note
    This function only swaps *ppvDestination with pvExchange, if previous *ppvDestination value equals pvComparand
*/
// uint32_t Atomic_CompareAndSwapPointers_p32( void * volatile * ppvDestination,  void * pvExchange,  void * pvComparand )



/*
Atomically adds count to the value of the specified pointer points to.

Atomic add

Parameters
    [in,out]	pulAddend	Pointer to memory location from where value is to be loaded and written back to.
    [in]	ulCount	Value to be added to *pulAddend.

Returns
    previous *pulAddend value
*/
// uint32_t Atomic_Add_u32( uint32_t volatile * pulAddend,  uint32_t ulCount )


/*
Atomically subtracts count from the value of the specified pointer pointers to.

Atomic subtract

Parameters
    [in,out]	pulAddend	Pointer to memory location from where value is to be loaded and written back to.
    [in]	ulCount	Value to be subtract from *pulAddend.

Returns
    previous *pulAddend value.
*/
// uint32_t Atomic_Subtract_u32( uint32_t volatile * pulAddend, uint32_t ulCount )


/*
Atomically increments the value of the specified pointer points to.

Atomic increment

Parameters
    [in,out]	pulAddend	Pointer to memory location from where value is to be loaded and written back to.

Returns
    *pulAddend value before increment.
*/
// uint32_t Atomic_Increment_u32( uint32_t volatile * pulAddend )


/*
Atomically decrements the value of the specified pointer points to.

Atomic decrement

Parameters
    [in,out]	pulAddend	Pointer to memory location from where value is to be loaded and written back to.

Returns
    *pulAddend value before decrement
*/
// uint32_t Atomic_Decrement_u32( uint32_t volatile * pulAddend )


/*
Performs an atomic OR operation on the specified values.

Atomic OR

Parameters
    [in,out]	pulDestination	Pointer to memory location from where value is to be loaded and written back to.
    [in]	ulValue	Value to be ORed with *pulDestination.

Returns
    The original value of *pulDestination
*/
// uint32_t Atomic_OR_u32( uint32_t volatile * pulDestination, uint32_t ulValue )


/*
Performs an atomic AND operation on the specified values.

Atomic AND

Parameters
    [in,out]	pulDestination	Pointer to memory location from where value is to be loaded and written back to.
    [in]	ulValue	Value to be ANDed with *pulDestination.

Returns
    The original value of *pulDestination
*/
// uint32_t Atomic_AND_u32( uint32_t volatile * pulDestination, uint32_t ulValue )


/*
Performs an atomic NAND operation on the specified values.

Atomic NAND

Parameters
    [in,out]	pulDestination	Pointer to memory location from where value is to be loaded and written back to.
    [in]	ulValue	Value to be NANDed with *pulDestination.

Returns
    The original value of *pulDestination
*/
// uint32_t Atomic_NAND_u32( uint32_t volatile * pulDestination, uint32_t ulValue )


/*
Performs an atomic XOR operation on the specified values.

Atomic XOR

Parameters
    [in,out]	pulDestination	Pointer to memory location from where value is to be loaded and written back to.
    [in]	ulValue	Value to be XORed with *pulDestination.

Returns
    The original value of *pulDestination
*/
// uint32_t Atomic_XOR_u32( uint32_t volatile * pulDestination, uint32_t ulValue )
