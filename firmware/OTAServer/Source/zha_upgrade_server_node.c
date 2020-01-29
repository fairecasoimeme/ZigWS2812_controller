/*****************************************************************************
 *
 * MODULE:             JN-AN-1189
 *
 * COMPONENT:          zha_upgrade_server_node.c
 *
 * DESCRIPTION:        ZBP-ZHA OTAServer (Implementation)
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142,
 * JN5139]. You, and any third parties must reproduce the copyright and
 * warranty notice and any other legend of ownership on each copy or partial
 * copy of the software.
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
 * Copyright NXP B.V. 2013. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "dbg.h"
#include "os.h"
#include "os_gen.h"
#include "app_pdm.h"
#include "pwrm.h"
#include "zps_gen.h"
#include "zps_apl.h"
#include "zps_apl_af.h"
#include "zps_apl_aib.h"
#include "zps_apl_zdo.h"
#include "zps_apl_zdp.h"
#include "app_ota_server.h"
#include "zha_upgrade_server_node.h"
#include "app_timer_driver.h"
//#include "AgeChildren.h"
#include "app_common.h"

#include "haEzJoin.h"

#include "PDM_IDs.h"
#include "zcl_options.h"
#include "zcl.h"
#include "ha.h"
#include "ota.h"
#include "zcl_common.h"

#include "app_zbp_utilities.h"
#include "app_zcl_server_node_task.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define USB_DONGLE
#ifndef TRACE_APP
#define TRACE_APP FALSE
#endif
#define DIO_8        (8)
#define APP_BUTTONS_DIO_MASK        ((1 << DIO_8))
#define PERMIT_JOIN_TIME_AFTER_STARTUP 180 /* Seconds    */

#ifndef USB_DONGLE
	#define LED_NWK_FORMED 0x00000004UL    /* using DIO2 */
	#define LED_PERMIT_JOIN 0x00000008UL   /* using DIO3 */
#else
	#define LED_NWK_FORMED (1<<16)         /* using DIO16 */
	#define LED_PERMIT_JOIN (1<<17)        /* using DIO17 */
#endif
#ifndef TRACE_APP_OTA
#define TRACE_APP_OTA FALSE
#else
#define TRACE_APP_OTA TRUE
#endif
/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitDR1174LED(void);
PRIVATE void app_vRestartNode (void);
PRIVATE void app_vStartNodeFactoryNew(void);
PRIVATE void vHandleJoinAndRejoinNWK( ZPS_tsAfEvent *pZPSevent,teEZ_JoinAction eJoinAction  );
/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PUBLIC PDM_tsRecordDescriptor sDevicePDDesc;
PUBLIC tsDeviceDesc sDeviceDesc;
PUBLIC bool bToggle;
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
// ZLL Commissioning trust centre link key
static const uint8 s_au8ZllLnkKeyArray[16] = {0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
                                     0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf};

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_vInitialiseNode
 *
 * DESCRIPTION:
 * Initialises the Coordinator application
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vInitialiseNode(void)
{

    /* Restore any application data previously saved to flash
     * All Application records must be loaded before the call to
     * ZPS_eAplAfInit
     */
	/* Restore device state previously saved to flash */
	eRestoreDeviceState();

    /* Initialise ZBPro stack */
    ZPS_vDefaultKeyInit();
    ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_PRECONFIGURED_LINK_KEY, (uint8 *)&s_au8LnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);
    ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_ZLL_LINK_KEY, (uint8 *)&s_au8ZllLnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);

    /* Initialise ZBPro stack */
    ZPS_eAplAfInit();
    /*Set Save default channel mask as it is going to be manipulated */
    vEZ_SetDefaultAIBChMask();

	APP_ZCL_vInitialise();

    /* Initialise other software modules
     * HERE
     */

    /* Always initialise any peripherals used by the applicatio
     * HERE
     */
    vInitDR1174LED();

    /* If the device state has been restored from flash, re-start the stack
     * and set the application running again. Note that if there is more than 1 state
     * where the network has already joined, then the other states should also be included
     * in the test below
     * E.g. E_RUNNING_1, E_RUNNING_2......
     * if (E_RUNNING_1 == s_sDevice || E_RUNNING_2 == s_sDevice)
     */
    if (E_RUNNING == eGetNodeState())
    {
    	app_vRestartNode();
    }
    else
    {
    	app_vStartNodeFactoryNew();
    }

    /* Start Child Aging Code */
    //OS_eStartSWTimer(APP_AgeOutChildrenTmr, APP_TIME_MS(1600), NULL);

    #ifdef CLD_OTA
	    vAppInitOTA();
    #endif

	OS_eStartSWTimer(APP_tmrLEDBlinkTimer,APP_TIME_MS(500),NULL);
    OS_eStartSWTimer(App_ServerTimer, APP_TIME_MS(1000),NULL);
}
/****************************************************************************
 *
 * NAME: eGetNodeState
 *
 * DESCRIPTION:
 * returns the device state
 *
 * RETURNS:
 * teNODE_STATES
 *
 ****************************************************************************/
PUBLIC teNODE_STATES eGetNodeState(void)
{
	return sDeviceDesc.eNodeState;
}
/****************************************************************************
 *
 * NAME: APP_ZPR_Upgrade_Server_Task
 *
 * DESCRIPTION:
 * Main state machine
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_ZPR_Upgrade_Server_Task)
{
    ZPS_tsAfEvent sStackEvent;
    sStackEvent.eType = ZPS_EVENT_NONE;

    /* check if any messages to collect */
    if (OS_E_OK == OS_eCollectMessage(APP_msgZpsEvents, &sStackEvent))
    {
        DBG_vPrintf(TRACE_APP, "\nAPP: ZPS event to process\n");

		/*Request response event ? - Call the child aging stuff */
    if((ZPS_EVENT_APS_DATA_INDICATION == sStackEvent.eType) &&
        (0 == sStackEvent.uEvent.sApsDataIndEvent.u8DstEndpoint))
		{
			//DBG_vPrintf(TRACE_APP, "APP: vCheckStackEvent: ZPS_EVENT_APS_ZDP_REQUEST_RESPONSE, Cluster = %x\n",	sStackEvent.uEvent.sApsZdpEvent.u16ClusterId );
			/* check if the this is a response from my end device child, if so remove */
			//vCheckIfMyChild(&sStackEvent);
		}
    }
    if (OS_eGetSWTimerStatus(App_tmr1sec) == OS_E_SWTIMER_EXPIRED)
    {
    	OS_eStopSWTimer(App_tmr1sec);
    	OS_eStartSWTimer(App_tmr1sec, APP_TIME_MS(1000), NULL);
    }

    switch (sDeviceDesc.eNodeState)
    {
        case E_STARTUP:
        	vHandleJoinAndRejoinNWK(&sStackEvent,E_EZ_JOIN);
			break;
        case E_RUNNING:
        	break;
        default:
            break;
    }
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: app_vRestartNode
 *
 * DESCRIPTION:
 * Start the Restart the ZigBee Stack after a context restore from
 * the EEPROM/Flash
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void app_vRestartNode (void)
{
	/*EZ Mode state is loaded as E_EZ_DEVICE_IN_NETWORK */
	ZPS_teStatus eStatus = eEZ_UpdateEZState(E_EZ_DEVICE_IN_NETWORK);

    /* Store the NWK frame counter increment */
    ZPS_vSaveAllZpsRecords();

    DBG_vPrintf(TRACE_APP, "APP: Re-starting Stack....\r\n");
    if (ZPS_E_SUCCESS != eStatus)
    {
        DBG_vPrintf(TRACE_APP, "APP: ZPS_eZdoStartStack() failed error %d", eStatus);
    }

    /* Re-start any other application software modules
     * HERE
     */
    DBG_vPrintf(TRACE_APP, "Restart Running\n");
}
/****************************************************************************
 *
 * NAME: app_vStartNodeFactoryNew
 *
 * DESCRIPTION:
 * Start the ZigBee Stack for the first ever Time.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void app_vStartNodeFactoryNew(void)
{
    /* Return the device to the start-up start if it was reset during the network formation stage */
	eEZ_UpdateEZState(E_EZ_START);
    vStartStopTimer( APP_JoinTimer, APP_TIME_MS(500),(uint8*)&(sDeviceDesc.eNodeState),E_STARTUP );
    DBG_vPrintf(TRACE_APP_OTA, "Start Factory New\n");
}
/****************************************************************************
 *
 * NAME: vHandleJoinAndRejoinNWK
 *
 * DESCRIPTION:
 * Handles the Start UP events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleJoinAndRejoinNWK( ZPS_tsAfEvent *pZPSevent,teEZ_JoinAction eJoinAction  )
{
	teEZ_State ezState;
	/*Call The EZ mode Handler passing the events*/
	vEZ_EZModeNWKJoinHandler(pZPSevent,eJoinAction);
    ezState = eEZ_GetJoinState();
    DBG_vPrintf(TRACE_APP_OTA, "EZ_STATE\%x r\n", ezState);
    if(ezState == E_EZ_DEVICE_IN_NETWORK)
    {
    	DBG_vPrintf(TRACE_APP_OTA, "HA EZMode E_EZ_DEVICE_IN_NETWORK \n");
    	vStartStopTimer( APP_StartUPTimer, APP_TIME_MS(500),(uint8*)&(sDeviceDesc.eNodeState),E_RUNNING );
        PDM_eSaveRecordData(PDM_ID_APP_ZLL_ROUTER,
                            &sDeviceDesc,
                            sizeof(tsDeviceDesc));
        ZPS_vSaveAllZpsRecords();
    }
}
/****************************************************************************
 *
 * NAME: vInitDR1174LED
 *
 * DESCRIPTION:
 * Initialised DIO3 and DIO6 as output
 *
 *
 ****************************************************************************/
PRIVATE void vInitDR1174LED(void)
{

	/*Enable Pull Ups : Not really required for the outputs */
	vAHI_DioSetPullup(0,LED_NWK_FORMED);
	vAHI_DioSetPullup(0,LED_PERMIT_JOIN);

	/*Make the DIo as output*/
	vAHI_DioSetDirection(0,LED_NWK_FORMED);
	vAHI_DioSetDirection(0,LED_PERMIT_JOIN);
}

/****************************************************************************
 *
 * NAME: APP_taskBlinkLED
 *
 * DESCRIPTION:
 * LED blinking
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_taskBlinkLED)
{

	/*Stop the timer and start it agian*/
	OS_eStopSWTimer(APP_tmrLEDBlinkTimer);
	OS_eStartSWTimer(APP_tmrLEDBlinkTimer,APP_TIME_MS(500),NULL);

	if (E_RUNNING == eGetNodeState())
	{
		vAHI_DioSetOutput(0,LED_NWK_FORMED);
	    vAHI_DioSetOutput(LED_PERMIT_JOIN,0);

	    if(bToggle == TRUE)
		{
			bToggle=FALSE;
			/*SetLEDs ON*/
			vAHI_DioSetOutput(0,LED_NWK_FORMED);
			vAHI_DioSetOutput(LED_PERMIT_JOIN,0);
		}
		else
		{
			/*Set LEDs OFF*/
			vAHI_DioSetOutput(LED_NWK_FORMED,0);
			vAHI_DioSetOutput(0,LED_PERMIT_JOIN);
		}
	}
	else
	{
		vAHI_DioSetOutput(0,LED_NWK_FORMED);
		vAHI_DioSetOutput(0,LED_PERMIT_JOIN);
	}
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
