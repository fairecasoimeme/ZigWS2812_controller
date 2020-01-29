/*****************************************************************************
 *
 * MODULE:             JN-AN-1171
 *
 * COMPONENT:          app_zcl_remote_task.c
 *
 * DESCRIPTION:        ZLL Demo: ZigBee Cluster Handlers - Implementation
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
#include <appapi.h>
#include "os.h"
#include "os_gen.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "dbg.h"
#include "pwrm.h"

#include "zps_apl_af.h"
#include "zps_apl_zdo.h"
#include "zps_apl_aib.h"
#include "zps_apl_zdp.h"
#include "rnd_pub.h"
#include "mac_pib.h"
#include "string.h"

#include "app_timer_driver.h"

#include "zcl_options.h"
#include "zll.h"
#include "zll_commission.h"
#include "commission_endpoint.h"
#include "app_common.h"
#include "zll_remote_node.h"
#include "ahi_aes.h"
#include "app_events.h"

#ifdef DEBUG_ZCL
#define TRACE_ZCL   TRUE
#else
#define TRACE_ZCL   FALSE
#endif

#ifdef DEBUG_REMOTE_TASK
#define TRACE_REMOTE_TASK   TRUE
#else
#define TRACE_REMOTE_TASK   FALSE
#endif

#ifdef DEBUG_LIGHT_AGE
#define TRACE_LIGHT_AGE   TRUE
#else
#define TRACE_LIGHT_AGE   FALSE
#endif

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

#define ZCL_TICK_TIME           APP_TIME_MS(1000)
#define STEP_SIZE               16
#define RATE                    5

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent);


PRIVATE void APP_ZCL_cbZllCommissionCallback(tsZCL_CallBackEvent *psEvent);
PRIVATE void APP_ZCL_cbZllUtilityCallback(tsZCL_CallBackEvent *psEvent);

extern PUBLIC bool_t bValidateTLAddress(ZPS_tsInterPanAddress *psDstAddr, uint8 u8Cmd);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

tsZLL_CommissionEndpoint sCommissionEndpoint;


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void* psGetDeviceTable(void) {
    return &sDeviceTable;
}

/****************************************************************************
 *
 * NAME: APP_ZCL_vInitialise
 *
 * DESCRIPTION:
 * Initialises ZCL related functions
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_ZCL_vInitialise(void)
{
    teZCL_Status eZCL_Status;

    /* Initialise ZLL */
    eZCL_Status = eZLL_Initialise(&APP_ZCL_cbGeneralCallback, apduZCL);
    if (eZCL_Status != E_ZCL_SUCCESS)
    {
        DBG_vPrintf(TRACE_ZCL, "Error: eZLL_Initialise returned %d\r\n", eZCL_Status);
    }

    /* Register Commission EndPoint */

    eZCL_Status = eApp_ZLL_RegisterEndpoint(&APP_ZCL_cbEndpointCallback,&sCommissionEndpoint);
    if (eZCL_Status != E_ZCL_SUCCESS)
    {
            DBG_vPrintf(TRACE_ZCL, "Error: eZLL_RegisterCommissionEndPoint:%d\r\n", eZCL_Status);
    }

    DBG_vPrintf(TRACE_REMOTE_TASK, "Chan Mask %08x\n", ZPS_psAplAibGetAib()->apsChannelMask);
    DBG_vPrintf(TRACE_REMOTE_TASK, "\nRxIdle TRUE");

    MAC_vPibSetRxOnWhenIdle( ZPS_pvAplZdoGetMacHandle(), TRUE, FALSE);
    sDeviceTable.asDeviceRecords[0].u64IEEEAddr = *((uint64*)pvAppApiGetMacAddrLocation());

    DBG_vPrintf(TRACE_REMOTE_TASK, "\ntsCLD_Groups %d", sizeof(tsCLD_Groups));
    // UNIFIED DBG_vPrintf(TRACE_REMOTE_TASK, "\ntsCLD_AS_Groups %d", sizeof(tsCLD_AS_Groups));
    DBG_vPrintf(TRACE_REMOTE_TASK, "\ntsCLD_GroupTableEntry %d", sizeof(tsCLD_GroupTableEntry));
}

/****************************************************************************
 *
 * NAME: ZCL_Task
 *
 * DESCRIPTION:
 * Main state machine for the IPD
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(ZCL_Task)
{
    APP_CommissionEvent sCommissionEvent;
    ZPS_tsAfEvent sStackEvent;
    tsZCL_CallBackEvent sCallBackEvent;
    sCallBackEvent.pZPSevent = &sStackEvent;

    /*
     * If the 1 second tick timer has expired, restart it and pass
     * the event on to ZCL
     */
    /* If there is a stack event to process, pass it on to ZCL */
    sStackEvent.eType = ZPS_EVENT_NONE;
    if (OS_eCollectMessage(APP_msgZpsEvents_ZCL, &sStackEvent) == OS_E_OK)
    {
        DBG_vPrintf(TRACE_ZCL, "ZCL_Task got event %d\r\n",sStackEvent.eType);

        switch(sStackEvent.eType)
        {

        case ZPS_EVENT_APS_DATA_INDICATION:
            DBG_vPrintf(TRACE_ZCL, "\nDATA: SEP=%d DEP=%d Profile=%04x Cluster=%04x\n",
                        sStackEvent.uEvent.sApsDataIndEvent.u8SrcEndpoint,
                        sStackEvent.uEvent.sApsDataIndEvent.u8DstEndpoint,
                        sStackEvent.uEvent.sApsDataIndEvent.u16ProfileId,
                        sStackEvent.uEvent.sApsDataIndEvent.u16ClusterId);
            break;

        case ZPS_EVENT_APS_DATA_CONFIRM:
            DBG_vPrintf(TRACE_ZCL, "\nCFM: SEP=%d DEP=%d Status=%d\n",
                        sStackEvent.uEvent.sApsDataConfirmEvent.u8SrcEndpoint,
                        sStackEvent.uEvent.sApsDataConfirmEvent.u8DstEndpoint,
                        sStackEvent.uEvent.sApsDataConfirmEvent.u8Status);

            break;

        case ZPS_EVENT_APS_DATA_ACK:
            DBG_vPrintf(TRACE_ZCL, "\nACK: SEP=%d DEP=%d Profile=%04x Cluster=%04x\n",
                        sStackEvent.uEvent.sApsDataAckEvent.u8SrcEndpoint,
                        sStackEvent.uEvent.sApsDataAckEvent.u8DstEndpoint,
                        sStackEvent.uEvent.sApsDataAckEvent.u16ProfileId,
                        sStackEvent.uEvent.sApsDataAckEvent.u16ClusterId);
            break;

        case ZPS_EVENT_APS_INTERPAN_DATA_INDICATION:
            /*
             * Validate the address used to send the command
             * filter off broadcasts and groupcast where not allowed
             */
            if ( (sStackEvent.uEvent.sApsInterPanDataIndEvent.u16ClusterId == 0x1000) &&
                 (sStackEvent.uEvent.sApsInterPanDataIndEvent.hAPduInst != NULL)
               )
            {
                if (FALSE ==  bValidateTLAddress( &sStackEvent.uEvent.sApsInterPanDataIndEvent.sDstAddr,
                                              (( pdum_tsAPduInstance* )sStackEvent.uEvent.sApsInterPanDataIndEvent.hAPduInst)->au8Storage[2] ))
                {
                    /* Reject packet on invalid addressing or command id
                     * free the apdu
                     */
                    PDUM_eAPduFreeAPduInstance( sStackEvent.uEvent.sApsInterPanDataIndEvent.hAPduInst);
                    return;
                }
            }
                break;

        case ZPS_EVENT_APS_INTERPAN_DATA_CONFIRM:

            if(sStackEvent.uEvent.sApsInterPanDataConfirmEvent.u8Status)
            {
                sCommissionEvent.eType = APP_E_COMMISSION_NOACK;
            }
            else
            {
                sCommissionEvent.eType = APP_E_COMMISSION_ACK;
            }
            OS_ePostMessage(APP_CommissionEvents, &sCommissionEvent);
            break;

        default:
            break;
        }

        sCallBackEvent.eEventType = E_ZCL_CBET_ZIGBEE_EVENT;
        vZCL_EventHandler(&sCallBackEvent);
    }
}


/****************************************************************************
 *
 * NAME: APP_ZCL_cbGeneralCallback
 *
 * DESCRIPTION:
 * General callback for ZCL events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_cbGeneralCallback(tsZCL_CallBackEvent *psEvent)
{
#if TRUE == TRACE_ZCL
    switch (psEvent->eEventType)
    {
        case E_ZCL_CBET_LOCK_MUTEX:
            DBG_vPrintf(TRACE_ZCL, "EVT: Lock Mutex\r\n");
            break;

        case E_ZCL_CBET_UNLOCK_MUTEX:
            DBG_vPrintf(TRACE_ZCL, "EVT: Unlock Mutex\r\n");
            break;

        case E_ZCL_CBET_UNHANDLED_EVENT:
            DBG_vPrintf(TRACE_ZCL, "EVT: Unhandled Event\r\n");
            break;

        case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
            DBG_vPrintf(TRACE_ZCL, "EVT: Read attributes response\r\n");
            break;

        case E_ZCL_CBET_READ_REQUEST:
            DBG_vPrintf(TRACE_ZCL, "EVT: Read request\r\n");
            break;

        case E_ZCL_CBET_DEFAULT_RESPONSE:
            DBG_vPrintf(TRACE_ZCL, "EVT: Default response\r\n");
            break;

        case E_ZCL_CBET_ERROR:
            DBG_vPrintf(TRACE_ZCL, "EVT: Error\r\n");
            break;

        case E_ZCL_CBET_TIMER:
            break;

        case E_ZCL_CBET_ZIGBEE_EVENT:
            DBG_vPrintf(TRACE_ZCL, "EVT: ZigBee\r\n");
            break;

        case E_ZCL_CBET_CLUSTER_CUSTOM:
            DBG_vPrintf(TRACE_ZCL, "EP EVT: Custom\r\n");
            break;

        default:
            DBG_vPrintf(TRACE_ZCL, "Invalid event type\r\n");
            break;
    }
#endif
}


/****************************************************************************
 *
 * NAME: APP_ZCL_cbEndpointCallback
 *
 * DESCRIPTION:
 * Endpoint specific callback for ZCL events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_cbEndpointCallback(tsZCL_CallBackEvent *psEvent)
{
uint32 i;

      DBG_vPrintf(TRACE_ZCL, "\nEntering cbZCL_EndpointCallback");

    switch (psEvent->eEventType)
    {

    case E_ZCL_CBET_LOCK_MUTEX:
    case E_ZCL_CBET_UNLOCK_MUTEX:
    case E_ZCL_CBET_UNHANDLED_EVENT:

    case E_ZCL_CBET_READ_ATTRIBUTES_RESPONSE:
    case E_ZCL_CBET_READ_REQUEST:
    case E_ZCL_CBET_DEFAULT_RESPONSE:
    case E_ZCL_CBET_ERROR:
    case E_ZCL_CBET_TIMER:
    case E_ZCL_CBET_ZIGBEE_EVENT:
//        DBG_vPrintf(TRACE_ZCL, "EP EVT:No action\r\n");
        break;

    case E_ZCL_CBET_READ_INDIVIDUAL_ATTRIBUTE_RESPONSE:
        DBG_vPrintf(TRACE_REMOTE_TASK, " Read Attrib Rsp %d %02x\n", psEvent->uMessage.sIndividualAttributeResponse.eAttributeStatus,
                *((uint8*)psEvent->uMessage.sIndividualAttributeResponse.pvAttributeData));

        for ( i=0; i<sEndpointTable.u8NumRecords; i++) {
            if (sEndpointTable.asEndpointRecords[i].u16NwkAddr == psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr) {
                sEndpointTable.au8PingCount[i] = 0;
                DBG_vPrintf(TRACE_LIGHT_AGE, "Reset Ping Count for %04x\n", sEndpointTable.asEndpointRecords[i].u16NwkAddr);
                break;
            }
        }

        uint8 u8Seq;
        tsZCL_Address sAddress;
        sAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
        sAddress.uAddress.u16DestinationAddress = psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr;
        tsCLD_Identify_IdentifyRequestPayload sPayload;
        sPayload.u16IdentifyTime = 0x0003;

        eCLD_IdentifyCommandIdentifyRequestSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8SrcEndpoint,
                        &sAddress,
                        &u8Seq,
                        &sPayload);
        break;

    case E_ZCL_CBET_CLUSTER_CUSTOM:
        DBG_vPrintf(TRACE_ZCL, "EP EVT: Custom %04x\r\n", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);

        switch (psEvent->uMessage.sClusterCustomMessage.u16ClusterId)
        {

        case GENERAL_CLUSTER_ID_IDENTIFY:
            DBG_vPrintf(TRACE_ZCL, "- for identify cluster\r\n");
            break;

        case GENERAL_CLUSTER_ID_GROUPS:
            DBG_vPrintf(TRACE_ZCL, "- for groups cluster\r\n");
            break;

        case 0x1000:
            DBG_vPrintf(TRACE_ZCL, "\n    - for 0x1000");
            if (psEvent->pZPSevent->eType == ZPS_EVENT_APS_INTERPAN_DATA_INDICATION &&  psEvent->pZPSevent->uEvent.sApsInterPanDataIndEvent.u16ProfileId == ZLL_PROFILE_ID) {
                APP_ZCL_cbZllCommissionCallback(psEvent);
            } else if (psEvent->pZPSevent->eType == ZPS_EVENT_APS_DATA_INDICATION && psEvent->pZPSevent->uEvent.sApsDataIndEvent.u16ProfileId == HA_PROFILE_ID) {
                APP_ZCL_cbZllUtilityCallback(psEvent);
            }
            break;


        default:
            DBG_vPrintf(TRACE_ZCL, "- for unknown cluster %d\r\n", psEvent->uMessage.sClusterCustomMessage.u16ClusterId);
            break;

        }
        break;

    default:
        DBG_vPrintf(TRACE_ZCL, "EP EVT: Invalid event type\r\n");
        break;

    }
}

/****************************************************************************
 *
 * NAME: APP_ZCL_cbZllCommissionCallback
 *
 * DESCRIPTION:
 * Callback from the ZCL commissioning cluster foloowing inter pan
 * messages, events created and passed to the commissioning task
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_cbZllCommissionCallback(tsZCL_CallBackEvent *psEvent)
{
    APP_CommissionEvent sEvent;
    sEvent.eType = APP_E_COMMISSION_MSG;
    sEvent.u8Lqi = psEvent->pZPSevent->uEvent.sApsInterPanDataIndEvent.u8LinkQuality;
    sEvent.sZllMessage.eCommand = ((tsCLD_ZllCommissionCustomDataStructure*)psEvent->psClusterInstance->pvEndPointCustomStructPtr)->sCallBackMessage.u8CommandId;
    sEvent.sZllMessage.sSrcAddr = ((tsCLD_ZllCommissionCustomDataStructure*)psEvent->psClusterInstance->pvEndPointCustomStructPtr)->sRxInterPanAddr.sSrcAddr;
    memcpy(&sEvent.sZllMessage.uPayload,
            (((tsCLD_ZllCommissionCustomDataStructure*)psEvent->psClusterInstance->pvEndPointCustomStructPtr)->sCallBackMessage.uMessage.psScanRspPayload),
            sizeof(tsZllPayloads));
    OS_ePostMessage(APP_CommissionEvents, &sEvent);
}

/****************************************************************************
 *
 * NAME: APP_ZCL_cbEndpointCallback
 *
 * DESCRIPTION:
 * Callback from the ZCL commissioning utility cluster,
 * events created and passed to the main remote control task
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_ZCL_cbZllUtilityCallback(tsZCL_CallBackEvent *psEvent)
{
    APP_tsEvent   sEvent;

    DBG_vPrintf(TRACE_REMOTE_TASK, "\nRx Util Cmd %02x",
            ((tsCLD_ZllUtilityCustomDataStructure*)psEvent->psClusterInstance->pvEndPointCustomStructPtr)->sCallBackMessage.u8CommandId);

    switch (((tsCLD_ZllUtilityCustomDataStructure*)psEvent->psClusterInstance->pvEndPointCustomStructPtr)->sCallBackMessage.u8CommandId ) {
        case E_CLD_UTILITY_CMD_ENDPOINT_INFO:
            sEvent.eType = APP_E_EVENT_EP_INFO_MSG;

            sEvent.uEvent.sEpInfoMsg.u16SrcAddr = psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr;
            memcpy(&sEvent.uEvent.sEpInfoMsg.sPayload,
                    ((tsCLD_ZllUtilityCustomDataStructure*)psEvent->psClusterInstance->pvEndPointCustomStructPtr)->sCallBackMessage.uMessage.psEndpointInfoPayload,
                    sizeof(tsCLD_ZllUtility_EndpointInformationCommandPayload));
            OS_ePostMessage(APP_msgEvents, &sEvent);
            break;

        case E_CLD_UTILITY_CMD_GET_ENDPOINT_LIST_REQ_RSP:
            DBG_vPrintf(TRACE_REMOTE_TASK, "\ngot ep list");
            sEvent.eType = APP_E_EVENT_EP_LIST_MSG;
            sEvent.uEvent.sEpListMsg.u8SrcEp = psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8SrcEndpoint;
            sEvent.uEvent.sEpListMsg.u16SrcAddr = psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr;
            memcpy(&sEvent.uEvent.sEpListMsg.sPayload,
                    ((tsCLD_ZllUtilityCustomDataStructure*)psEvent->psClusterInstance->pvEndPointCustomStructPtr)->sCallBackMessage.uMessage.psGetEndpointListRspPayload,
                    sizeof(tsCLD_ZllUtility_GetEndpointListRspCommandPayload));
            OS_ePostMessage(APP_msgEvents, &sEvent);
            break;
        case E_CLD_UTILITY_CMD_GET_GROUP_ID_REQ_RSP:
            DBG_vPrintf(TRACE_REMOTE_TASK, "\ngot group list");
            sEvent.eType = APP_E_EVENT_GROUP_LIST_MSG;
            sEvent.uEvent.sGroupListMsg.u8SrcEp = psEvent->pZPSevent->uEvent.sApsDataIndEvent.u8SrcEndpoint;
            sEvent.uEvent.sGroupListMsg.u16SrcAddr = psEvent->pZPSevent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr;
            memcpy(&sEvent.uEvent.sEpListMsg.sPayload,
                    ((tsCLD_ZllUtilityCustomDataStructure*)psEvent->psClusterInstance->pvEndPointCustomStructPtr)->sCallBackMessage.uMessage.psGetGroupIdRspPayload,
                    sizeof(tsCLD_ZllUtility_GetGroupIdRspCommandPayload));
            OS_ePostMessage(APP_msgEvents, &sEvent);
            break;
    }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
