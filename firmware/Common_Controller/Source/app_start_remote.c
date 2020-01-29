/*****************************************************************************
 *
 * MODULE:             JN-AN-1171
 *
 * COMPONENT:          app_start_remote.c
 *
 * DESCRIPTION:        ZLL Demo: System Initialisation - Implementation
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5164,
 * JN5161, JN5148, JN5142, JN5139].
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "os.h"
#include "os_gen.h"
#include "pwrm.h"
#include "pdum_nwk.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "dbg.h"
#include "dbg_uart.h"

#include "zps_apl_af.h"
#include "appapi.h"
#include "app_timer_driver.h"
#include "zll_remote_node.h"
#include "DriverCapTouch.h"
#include "low_bat_indicator.h"
#include "app_led_control.h"

#include <string.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define RAM_HELD 2

#ifndef DEBUG_APP
#define TRACE_APP   FALSE
#else
#define TRACE_APP   TRUE
#endif

#ifndef DEBUG_START_UP
#define TRACE_START FALSE
#else
#define TRACE_START TRUE
#endif

#ifndef DEBUG_SLEEP
#define TRACE_SLEEP FALSE
#else
#define TRACE_SLEEP TRUE
#endif

#ifndef DEBUG_EXCEPTIONS
#define TRACE_EXCEPTION FALSE
#else
#define TRACE_EXCEPTION TRUE
#endif

#define HALT_ON_EXCEPTION   FALSE

#define POWER_BTN (1)
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void vInitialiseApp(void);
PRIVATE void vUnclaimedInterrupt(void);
PRIVATE void vSetUpWakeUpConditions(void);

PRIVATE void vOSError(OS_teStatus eStatus, void *hObject);

PRIVATE void vPdmEventHandlerCallback(uint32 u32EventNumber, PDM_eSystemEventCode eSystemEventCode);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern void *_stack_low_water_mark;

#if JENNIC_CHIP_FAMILY == JN516x
    extern uint32 u32HeapStart;
    extern uint32 u32HeapEnd;
#endif

bool_t bDeepSleep = FALSE;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/**
 * Power manager Callback.
 * Called just before the device is put to sleep
 */

static PWRM_DECLARE_CALLBACK_DESCRIPTOR(PreSleep);
/**
 * Power manager Callback.
 * Called just after the device wakes up from sleep
 */
static PWRM_DECLARE_CALLBACK_DESCRIPTOR(Wakeup);
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: PreSleep
 *
 * DESCRIPTION:
 *
 * PreSleep call back by the power manager before the controller put into sleep.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PWRM_CALLBACK(PreSleep)
{
    DBG_vPrintf(TRACE_SLEEP,"Sleeping mode %d...\n", bDeepSleep);
    /* If the power mode is with RAM held do the following
     * else not required as the entry point will init everything*/
    if(!bDeepSleep)
    {
        /* sleep memory held */
        eTouchSleep();
       vAppApiSaveMacSettings();
    }

    /* Disable UART */
    vAHI_UartDisable(E_AHI_UART_0);

    /* Set up wake up dio input */
    vSetUpWakeUpConditions();
}

/****************************************************************************
 *
 * NAME: Wakeup
 *
 * DESCRIPTION:
 *
 * Wakeup call back by the power manager after the controller wakes up from sleep.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PWRM_CALLBACK(Wakeup)
{
    /*Stabilise the oscillator*/
    #if JENNIC_CHIP_FAMILY == JN516x
        // Wait until FALSE i.e. on XTAL  - otherwise uart data will be at wrong speed
        while (bAHI_GetClkSource() == TRUE);
        // Now we are running on the XTAL, optimise the flash memory wait states.
        vAHI_OptimiseWaitStates();
    #endif

    /*Clear wakeup status by reading the wake status*/
    u32AHI_DioWakeStatus();

    /* Reinitialise the battery monitor as sleep restores default 0f 2.0v */
    vLowBatIndicationInit( (LHS_LED_DIO | RHS_LED_DIO), E_BO_TRIP_2V4 );

    /*Wake up the touch interface */
    eTouchWake();

    /* Don't use RTS/CTS pins on UART0 as they are used for buttons */
    vAHI_UartSetRTSCTS(E_AHI_UART_0, FALSE);
    DBG_vUartInit(DBG_E_UART_0, DBG_E_UART_BAUD_RATE_115200);

    DBG_vPrintf(TRACE_SLEEP, "\n\nAPP: Woken up (CB)");
    DBG_vPrintf(TRACE_APP, "\nAPP: Warm Waking powerStatus = 0x%x", u8AHI_PowerStatus());

    /* If the power status is OK and RAM held while sleeping
     * restore the MAC settings
     * */
    if( (u8AHI_PowerStatus()) && ( !bDeepSleep) )
    {
        // Restore Mac settings (turns radio on)
        vMAC_RestoreSettings();
        DBG_vPrintf(TRACE_APP, "\nAPP: MAC settings restored");
    }
    DBG_vPrintf(TRACE_APP, "\nAPP: Restarting OS \n");
    /* Restart the OS */
    OS_vRestart();

    /* Activate the SleepTask, that would start the SW timer and polling would continue
     * */
    vStartUpHW();

}

/****************************************************************************
 *
 * NAME: vAppMain
 *
 * DESCRIPTION:
 *
 * Entry point for application from a cold start.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppMain(void)
{

    #if JENNIC_CHIP_FAMILY == JN516x
        /*
         * Wait until FALSE i.e. on XTAL  - otherwise uart data will be at wrong speed
         * */
        while (bAHI_GetClkSource() == TRUE);
        /*
         * Now we are running on the XTAL, optimise the flash memory wait states.
         * */
        vAHI_OptimiseWaitStates();
    #endif

     /* Enable the Low Battery Detection function */

     vLowBatIndicationInit( (LHS_LED_DIO | RHS_LED_DIO), E_BO_TRIP_2V4 );


    /* Don't use RTS/CTS pins on UART0 as they are used for buttons */

    vAHI_UartSetRTSCTS(E_AHI_UART_0, FALSE);

    /* Initialize the debug diagnostics module to use UART0 at 115K Baud; */
    /* Do not use UART 1 if LEDs are used, as it shares DIO with the LEDS */

    DBG_vUartInit(DBG_E_UART_0, DBG_E_UART_BAUD_RATE_115200);
    DBG_vPrintf(TRACE_START, "\n\nAPP: Switch Power Up");

    /*
     * Delay to allow programmer to end and serial terminal to start on PC
     * */
    #ifdef TRACE_START
        /* Startup delay
         * */
        uint16 n;
        for(n = 0; n < 10000; n++)
        {
            DBG_vPrintf(TRACE_START, ".");
            vAHI_WatchdogRestart();
        }
    #endif

    #if JENNIC_CHIP_FAMILY == JN516x
        DBG_vPrintf(TRACE_START, "\nHeap size at step 1 is %d bytes (start=%08x end=%08x)", u32HeapEnd - u32HeapStart, u32HeapStart, u32HeapEnd);
    #endif

    /*
     * Initialise the stack overflow exception to trigger if the end of the
     * stack is reached. See the linker command file to adjust the allocated
     * stack size.
     */
    vAHI_SetStackOverflow(TRACE_EXCEPTION, (uint32)&_stack_low_water_mark);

    /*
     * Catch resets due to watchdog timer expiry. Comment out to harden code.
     * */
    if (bAHI_WatchdogResetEvent())
    {
        DBG_vPrintf(TRACE_EXCEPTION, "\nAPP: Watchdog timer has reset device!");
        #if HALT_ON_EXCEPTION
            vAHI_WatchdogStop();
            while (1);
        #endif
    }

#ifndef JENNIC_MAC_MiniMacShim
    /*
     * Initialize ROM based software modules
     * */
    u32AppApiInit(NULL, NULL, NULL, NULL, NULL, NULL);
#endif

    #if JENNIC_CHIP_FAMILY == JN516x
        DBG_vPrintf(TRACE_START, "\nHeap size at step 2a is %d bytes (start=%08x end=%08x)", u32HeapEnd - u32HeapStart, u32HeapStart, u32HeapEnd);
    #endif

    /*
     * Start the RTOS
     * */
    OS_vStart(vInitialiseApp, vUnclaimedInterrupt, vOSError);
    #if JENNIC_CHIP_FAMILY == JN516x
        DBG_vPrintf(TRACE_START, "\nHeap size at step 2b is %d bytes (start=%08x end=%08x)", u32HeapEnd - u32HeapStart, u32HeapStart, u32HeapEnd);
    #endif

    /*
     * Idle task commences here
     * */

    while (TRUE)
    {
        /*
         * Re-load the watch-dog timer. Execution must return through the idle
         * task before the CPU is suspended by the power manager. This ensures
         * that at least one task / ISR has executed with in the watchdog period
         * otherwise the system will be reset.
         */
        vAHI_WatchdogRestart();

        /*
         * suspends CPU operation when the system is idle or puts the device to
         * sleep if there are no activities in progress
         */
        PWRM_vManagePower();
    }
}

/****************************************************************************
 *
 * NAME: vAppMain
 *
 * DESCRIPTION:
 *
 * Power manager callback.
 * Called to allow the application to register
 * sleep and wake callbacks.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vAppRegisterPWRMCallbacks(void)
{
    PWRM_vRegisterPreSleepCallback(PreSleep);
    PWRM_vRegisterWakeupCallback(Wakeup);
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: vSetUpWakeUpConditions
 *
 * DESCRIPTION:
 *
 * Set up the wake up inputs while going to sleep  or preserve as an output
 * to drive the LHS led indicator if never sleeping
 *
 ****************************************************************************/
PRIVATE void vSetUpWakeUpConditions(void)
{
    if ( bDeepSleep)
    {
        APP_vSetLeds(0);                    /* ensure leds are off */
        u32AHI_DioWakeStatus();             /* clear interrupts */
        vAHI_DioSetDirection(POWER_BTN,0);  /* Set Power Button(DIO0) as Input  */
        vAHI_DioWakeEdge(0,POWER_BTN);      /* Set wake up as DIO Falling Edge  */
        vAHI_DioWakeEnable(POWER_BTN,0);    /* Enable Wake up DIO Power Button  */
    }

}

/****************************************************************************
 *
 * NAME: vInitialiseApp
 *
 * DESCRIPTION:
 *
 * Initialises Zigbee stack, hardware and application.
 *
 * PARAMETERS:      Name            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitialiseApp(void)
{

    #if JENNIC_CHIP_FAMILY == JN516x
        DBG_vPrintf(TRACE_START, "\nHeap size at step 3 is %d bytes (start=%08x end=%08x)", u32HeapEnd - u32HeapStart, u32HeapStart, u32HeapEnd);
    #endif

    /*
     * Initialise JenOS modules. Initialise Power Manager even on non-sleeping nodes
     * as it allows the device to doze when in the idle task.
     * Parameter options: E_AHI_SLEEP_OSCON_RAMON or E_AHI_SLEEP_DEEP or ...
     */
    PWRM_vInit(E_AHI_SLEEP_OSCON_RAMON);

    #if JENNIC_CHIP_FAMILY == JN516x
        DBG_vPrintf(TRACE_START, "\nHeap size at step 4 is %d bytes (start=%08x end=%08x)", u32HeapEnd - u32HeapStart, u32HeapStart, u32HeapEnd);
    #endif

    /* Initialise the Persistent Data Manager */
    PDM_eInitialise(0, NULL);
    PDM_vRegisterSystemCallback(vPdmEventHandlerCallback);


    #if JENNIC_CHIP_FAMILY == JN516x
        DBG_vPrintf(TRACE_START, "\nHeap size at step 5 is %d bytes (start=%08x end=%08x)", u32HeapEnd - u32HeapStart, u32HeapStart, u32HeapEnd);
    #endif

    /* Initialise Protocol Data Unit Manager */
    PDUM_vInit();
    #if JENNIC_CHIP_FAMILY == JN516x
        DBG_vPrintf(TRACE_START, "\nHeap size at step 6 is %d bytes (start=%08x end=%08x)", u32HeapEnd - u32HeapStart, u32HeapStart, u32HeapEnd);
    #endif

    /* initialise application */
    APP_vInitialiseNode();
    #if JENNIC_CHIP_FAMILY == JN516x
        DBG_vPrintf(TRACE_START, "\nHeap size at step 7 is %d bytes (start=%08x end=%08x)", u32HeapEnd - u32HeapStart, u32HeapStart, u32HeapEnd);
    #endif
}


/****************************************************************************
 *
 * NAME: vOSError
 *
 * DESCRIPTION:
 * Catches any unexpected OS errors
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vOSError(OS_teStatus eStatus, void *hObject)
{
    OS_thTask hTask;

    //* ignore queue underruns */
    if ((OS_E_QUEUE_EMPTY == eStatus) ||
        (( eStatus >= OS_E_SWTIMER_STOPPED) && ( eStatus <= OS_E_SWTIMER_RUNNING)) )
    {
        return;
    }

    DBG_vPrintf(TRACE_EXCEPTION, "OS Error %d, offending object handle = 0x%08x\n", eStatus, hObject);

    /* NB the task may have been pre-empted by an ISR which may be at fault */
    OS_eGetCurrentTask(&hTask);
    DBG_vPrintf(TRACE_EXCEPTION, "Currently active task handle = 0x%08x\n", hTask);
#ifdef OS_STRICT_CHECKS
    DBG_vPrintf(TRACE_EXCEPTION, "Currently active ISR fn address = 0x%08x\n", OS_prGetActiveISR());
#endif

}

PRIVATE void vPdmEventHandlerCallback(uint32 u32EventNumber, PDM_eSystemEventCode eSystemEventCode)
{
    DBG_vPrintf(TRACE_EXCEPTION, "\nPDM: Event %x:%d", u32EventNumber, eSystemEventCode);
}

/****************************************************************************
 *
 * NAME: vUnclaimedInterrupt
 *
 * DESCRIPTION:
 * Catches any unexpected interrupts
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vUnclaimedInterrupt(void)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Unclaimed interrupt\n");
    DBG_vDumpStack();
#if HALT_ON_EXCEPTION
    while (1);
#else
    vAHI_SwReset();
#endif
}
/****************************************************************************
 *
 * NAME: APP_isrStackOverflowException
 *
 * DESCRIPTION:
 * Catches any unexpected interrupts
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrStackOverflowException)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Stack overflowed\n");
#if HALT_ON_EXCEPTION
    while (1);
#else
    vAHI_SwReset();
#endif
}
/****************************************************************************
 *
 * NAME: APP_isrUnimplementedModuleException
 *
 * DESCRIPTION:
 * Catches any unexpected interrupts
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrUnimplementedModuleException)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Unimplemented module exception\n");
    DBG_vDumpStack();
#if HALT_ON_EXCEPTION
    while (1);
#else
    vAHI_SwReset();
#endif
}
/****************************************************************************
 *
 * NAME: APP_isrBusErrorException
 *
 * DESCRIPTION:
 * Catches any unexpected interrupts
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrBusErrorException)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Bus error\n");
    DBG_vDumpStack();
#if HALT_ON_EXCEPTION
    while(1);
#else
    vAHI_SwReset();
#endif
}
/****************************************************************************
 *
 * NAME: APP_isrAlignmentException
 *
 * DESCRIPTION:
 * Catches any unexpected interrupts
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrAlignmentException)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Alignment exception\n");
#if HALT_ON_EXCEPTION
    DBG_vDumpStack();
    while(1);
#else
    vAHI_SwReset();
#endif
}
/****************************************************************************
 *
 * NAME: APP_isrIllegalInstruction
 *
 * DESCRIPTION:
 * Catches any unexpected interrupts
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_ISR(APP_isrIllegalInstruction)
{
    DBG_vPrintf(TRACE_EXCEPTION, "Illegal instruction\n");
#if HALT_ON_EXCEPTION
    DBG_vDumpStack();
    while(1);
#else
    vAHI_SwReset();
#endif
}


/****************************************************************************
 *
 * NAME: vDebug
 *
 * DESCRIPTION:
 * prints a string
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PUBLIC void vDebug(char *pcMessage)
{
    DBG_vPrintf(TRACE_EXCEPTION, "%s",pcMessage);
}
/****************************************************************************
 *
 * NAME: vDebugHex
 *
 * DESCRIPTION:
 * prints a hex
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vDebugHex(uint32 u32Data, int iSize)
{
    DBG_vPrintf(TRACE_EXCEPTION, "%x",u32Data);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
