/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation 
 * 
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 * 
 * SPDX-License-Identifier: MIT
 **************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */ 
/** ThreadX Component                                                     */ 
/**                                                                       */
/**   Thread                                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define TX_SOURCE_CODE


/* Include necessary system files.  */

#include "tx_api.h"
#include "tx_thread.h"

/* Define small routines used for the TX_DISABLE/TX_RESTORE macros.  */

UINT   _tx_thread_interrupt_disable(void)
{

UINT    previous_value;


    previous_value =  _tx_thread_interrupt_control(TX_INT_DISABLE);
    return(previous_value);
}


VOID   _tx_thread_interrupt_restore(UINT previous_posture)
{

    previous_posture =  _tx_thread_interrupt_control(previous_posture);
}


/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _tx_thread_interrupt_control                        Linux/GNU       */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function is responsible for changing the interrupt lockout     */ 
/*    posture of the system.                                              */ 
/*                                                                        */ 
/*  INPUT                                                                 */ 
/*                                                                        */ 
/*    new_posture                           New interrupt lockout posture */ 
/*                                                                        */ 
/*  OUTPUT                                                                */ 
/*                                                                        */ 
/*    old_posture                           Old interrupt lockout posture */ 
/*                                                                        */ 
/*  CALLS                                                                 */ 
/*                                                                        */ 
/*    tx_linux_mutex_lock                                                 */ 
/*    pthread_self                                                        */ 
/*    pthread_getschedparam                                               */ 
/*    tx_linux_mutex_recursive_unlock                                     */ 
/*    pthread_exit                                                        */ 
/*                                                                        */ 
/*  CALLED BY                                                             */ 
/*                                                                        */ 
/*    Application Code                                                    */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-30-2020     William E. Lamie         Initial Version 6.1           */
/*                                                                        */
/**************************************************************************/
UINT   _tx_thread_interrupt_control(UINT new_posture)
{

UINT        old_posture;
TX_THREAD   *thread_ptr; 
pthread_t   thread_id;
int         exit_code = 0;


    /* Lock Linux mutex. */
    tx_linux_mutex_lock(_tx_linux_mutex);

    /* Pickup the id of the current thread.  */
    thread_id = pthread_self();

    /* Pickup the current thread pointer.  */
    thread_ptr =      _tx_thread_current_ptr;

    /* Determine if this is a thread and it does not 
       match the current thread pointer.  */
    if ((_tx_linux_threadx_thread) && 
        ((!thread_ptr) || (!pthread_equal(thread_ptr -> tx_thread_linux_thread_id, thread_id)))) 
    { 

        /* This indicates the Linux thread was actually terminated by ThreadX is only 
           being allowed to run in order to cleanup its resources.  */
        /* Unlock linux mutex. */
        tx_linux_mutex_recursive_unlock(_tx_linux_mutex);
        pthread_exit((void *)&exit_code);
    } 

    /* Determine the current interrupt lockout condition.  */
    if (tx_linux_mutex_recursive_count == 1)
    {

        /* Interrupts are enabled.  */
        old_posture =  TX_INT_ENABLE;
    }
    else
    {

        /* Interrupts are disabled.  */
        old_posture =  TX_INT_DISABLE;
    }

    /* First, determine if this call is from a non-thread.  */
    if (_tx_thread_system_state)
    {

        /* Determine how to apply the new posture.  */
        if (new_posture == TX_INT_ENABLE)
        {

            /* Clear the disabled flag.  */
            _tx_linux_global_int_disabled_flag =  TX_FALSE;

            /* Determine if the critical section is locked.  */
            tx_linux_mutex_recursive_unlock(_tx_linux_mutex);
        }
        else if (new_posture == TX_INT_DISABLE)
        {

            /* Set the disabled flag.  */
            _tx_linux_global_int_disabled_flag =  TX_TRUE;
        }
    }
    else if (thread_ptr) 
    {

        /* Determine how to apply the new posture.  */
        if (new_posture == TX_INT_ENABLE)
        {

            /* Clear the disabled flag.  */
            _tx_thread_current_ptr -> tx_thread_linux_int_disabled_flag =  TX_FALSE;

            /* Determine if the critical section is locked.  */
            tx_linux_mutex_recursive_unlock(_tx_linux_mutex);
        }
        else if (new_posture == TX_INT_DISABLE)
        {

            /* Set the disabled flag.  */
            _tx_thread_current_ptr -> tx_thread_linux_int_disabled_flag =  TX_TRUE;
        }
    }

    /* Return the previous interrupt disable posture.  */
    return(old_posture);
}

