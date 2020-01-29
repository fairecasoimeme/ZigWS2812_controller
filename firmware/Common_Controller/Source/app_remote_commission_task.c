/*****************************************************************************
 *
 *
 * COMPONENT:          app_remote_commission_task.c
 *
 * DESCRIPTION:        ZLL Demo: Commissioning  - Implementation
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


#include "app_common.h"
#include "app_timer_driver.h"
#include "app_led_control.h"
#include "zll_remote_node.h"

#include "app_zcl_remote_task.h"
#include "app_events.h"
#include "zcl_customcommand.h"
#include "mac_sap.h"

#include <rnd_pub.h>
#include <mac_pib.h>
#include <string.h>
#include <stdlib.h>

#include "PDM_IDs.h"
#ifndef DEBUG_SCAN
#define TRACE_SCAN            FALSE
#else
#define TRACE_SCAN            TRUE
#endif

#ifndef DEBUG_JOIN
#define TRACE_JOIN            FALSE
#else
#define TRACE_JOIN            TRUE
#endif

#ifndef DEBUG_COMMISSION
#define TRACE_COMMISSION      FALSE
#else
#define TRACE_COMMISSION      TRUE
#endif

#define TL_VALIDATE FALSE

#define ADJUST_POWER        TRUE
#define ZLL_SCAN_LQI_MIN    (110)


PRIVATE uint8 eEncryptKey(uint8* au8InData,
                    uint8* au8OutData,
                    uint32 u32TransId,
                    uint32 u32ResponseId,
                    uint8 u8KeyIndex);
PRIVATE uint8 eDecryptKey(uint8* au8InData,
                    uint8* au8OutData,
                    uint32 u32TransId,
                    uint32 u32ResponseId,
                    uint8 u8KeyIndex);




typedef enum {
    E_IDLE,
    E_SCANNING,
    E_SCAN_DONE,
    E_SCAN_WAIT_ID,
    E_SCAN_WAIT_INFO,
    E_SCAN_WAIT_RESET_SENT,
    E_WAIT_START_RSP,
    E_WAIT_JOIN_RTR_RSP,
    E_WAIT_JOIN_ZED_RSP,
    E_WAIT_LEAVE,
    E_WAIT_LEAVE_RESET,
    E_WAIT_START_UP,
    E_ACTIVE,
    E_INFORM_APP
}eState;

typedef struct {
    ZPS_tsInterPanAddress       sSrcAddr;
    tsCLD_ZllCommission_ScanRspCommandPayload                   sScanRspPayload;
}tsZllScanData;

typedef struct {
    uint16 u16LQI;
    tsZllScanData sScanDetails;
}tsZllScanTarget;

/* to hold addrees ranges over touchlink in case they need recovering */
uint16 u16TempAddrLow;
uint16 u16TempAddrHigh;
uint16 u16TempGroupLow;
uint16 u16TempGroupHigh;
bool bDoRejoin = FALSE;
bool bWithDiscovery = FALSE;

PRIVATE uint8 u8NewUpdateID(uint8 u8ID1, uint8 u8ID2);

tsZllScanTarget sScanTarget;

tsCLD_ZllCommission_NetworkJoinEndDeviceReqCommandPayload sStartParams;


PRIVATE tsReg128 sMasterKey = {0x11223344, 0x55667788, 0x99aabbcc, 0xddeeff00 };
PRIVATE tsReg128 sCertKey = {0xc0c1c2c3, 0xc4c5c6c7,0xc8c9cacb,0xcccdcecf};


PUBLIC bool_t bTLinkInProgress = FALSE;
PUBLIC bool_t bSendFactoryResetOverAir=FALSE;

tsZllRemoteState sZllState = {FACTORY_NEW_REMOTE, E_REMOTE_STARTUP, ZLL_SKIP_CH1, ZLL_MIN_ADDR, ZLL_MIN_ADDR, ZLL_MAX_ADDR, ZLL_MIN_GROUP, ZLL_MAX_GROUP};
extern tsCLD_ZllDeviceTable sDeviceTable;
extern bool_t bFailedToJoin;
extern void vRemoveLight(uint16 u16Addr);

APP_tsEventTouchLink        sTarget;
ZPS_tsInterPanAddress       sDstAddr;

uint64 au64IgnoreList[3];

typedef struct {
    eState eState;
    uint8 u8Count;
    bool_t bSecondScan;
    bool_t bResponded;
    uint32 u32TransactionId;
    uint32 u32ResponseId;
    uint32 u32TheirTransactionId;
    uint32 u32TheirResponseId;
} tsCommissionData;

tsCommissionData sCommission;

PRIVATE void vSendScanResponse( ZPS_tsNwkNib *psNib,
                                ZPS_tsInterPanAddress       *psDstAddr,
                                uint32 u32TransactionId,
                                uint32 u32ResponseId);

PRIVATE void vSendScanRequest(void *pvNwk, tsCommissionData *psCommission);
PRIVATE void vHandleScanResponse( void *pvNwk,
                                  ZPS_tsNwkNib *psNib,
                                  tsZllScanTarget *psScanTarget,
                                  APP_CommissionEvent  *psEvent);

PRIVATE void vSendStartRequest( void *pvNwk,
                                ZPS_tsNwkNib *psNib,
                                tsZllScanTarget *psScanTarget,
                                uint32 u32TransactionId);
PRIVATE void vSendFactoryResetRequest( void *pvNwk,
                                       ZPS_tsNwkNib *psNib,
                                       tsZllScanTarget *psScanTarget,
                                       uint32 u32TransactionId);
PRIVATE void vSendRouterJoinRequest( void * pvNwk,
                                     ZPS_tsNwkNib *psNib,
                                     tsZllScanTarget *psScanTarget,
                                     uint32 u32TransactionId);
PRIVATE void vSendEndDeviceJoinRequest( void *pvNwk,
                                        ZPS_tsNwkNib *psNib,
                                        tsZllScanTarget *psScanTarget,
                                        uint32 u32TransactionId);
PRIVATE void vHandleNwkUpdateRequest( void * pvNwk,
                                      ZPS_tsNwkNib *psNib,
                                      tsCLD_ZllCommission_NetworkUpdateReqCommandPayload *psNwkUpdateReqPayload);

PRIVATE void vHandleDeviceInfoRequest(APP_CommissionEvent *psEvent);
PRIVATE void vHandleEndDeviceJoinRequest( void * pvNwk,
                                          ZPS_tsNwkNib *psNib,
                                          APP_CommissionEvent *psEvent,
                                          tsCommissionData *psCommission);
PRIVATE void vHandleNwkStartResponse( void * pvNwk,
                                          APP_CommissionEvent *psEvent,
                                          tsCommissionData *psCommission);
PRIVATE void vHandleNwkStartRequest( ZPS_tsInterPanAddress  *psDstAddr,
                                     uint32                 u32TransactionId);
PRIVATE void vHandleRouterJoinRequest( ZPS_tsInterPanAddress  *psDstAddr,
                                       uint32                 u32TransactionId);
PRIVATE void vSendIdentifyRequest(uint64 u64Addr, uint32 u32TransactionId, uint8 u8Time);
PRIVATE void vEndCommissioning( void* pvNwk, eState eState, uint16 u16TimeMS);

PRIVATE void vSaveTempAddresses( void);
PRIVATE void vRecoverTempAddresses( void);
PRIVATE void vSendDeviceInfoReq(uint64 u64Addr, uint32 u32TransactionId, uint8 u8Index);
PRIVATE void vSetUpParent( ZPS_tsNwkNib *psNib, uint16 u16Addr);

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vEndCommissioning
 *
 * DESCRIPTION: cleans up after a touch link session
 *
 *
 * RETURNS: void
 * void
 *
 ****************************************************************************/
PRIVATE void vEndCommissioning( void* pvNwk, eState eState, uint16 u16TimeMS){
    sCommission.eState = eState;
    eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, sZllState.u8MyChannel);

#if ADJUST_POWER
    eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_NORMAL);
#endif
    sCommission.u32TransactionId = 0;
    sCommission.bResponded = FALSE;
    OS_eStopSWTimer(APP_CommissionTimer);
    bTLinkInProgress = FALSE;
    if (u16TimeMS > 0) {
        OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(u16TimeMS), NULL);
    }
    DBG_vPrintf(TRACE_COMMISSION, "\nRxIdle False");
    MAC_vPibSetRxOnWhenIdle( ZPS_pvAplZdoGetMacHandle(), FALSE, FALSE);
    if (sZllState.eState == NOT_FACTORY_NEW_REMOTE) {
        vStartPolling();
    }
}

/****************************************************************************
 *
 * NAME: vInitCommission
 *
 * DESCRIPTION: initialise the commissioning state machine
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vInitCommission()
{
    sCommission.eState = E_IDLE;
}

/****************************************************************************
 *
 * NAME: APP_SendIndentify_Task
 *
 * DESCRIPTION:
 * Task that handles touch linkcommissioning events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_Commission_Task) {
    APP_CommissionEvent         sEvent;
    APP_tsEvent                   sAppEvent;

    if (OS_eCollectMessage(APP_CommissionEvents, &sEvent) != OS_E_OK)
    {
        DBG_vPrintf(TRACE_COMMISSION, "\n\nCommision task error!!!\n");
        return;
    }

    if (sEvent.eType == APP_E_COMMISSION_MSG)
    {
        DBG_vPrintf(TL_VALIDATE, "TL Cmd %d From %016llx\n",
                        sEvent.sZllMessage.eCommand,
                        sEvent.sZllMessage.sSrcAddr.uAddress.u64Addr);
        if (sEvent.sZllMessage.uPayload.sScanReqPayload.u32TransactionId == 0)
        {
            /* illegal transaction id */
            DBG_vPrintf(TL_VALIDATE, "TL Reject: transaction Id 0\n");
            return;
        }
    }

    DBG_vPrintf(TRACE_COMMISSION, "Commision task state %d type %d\n", sCommission.eState, sEvent.eType);
    void *pvNwk = ZPS_pvAplZdoGetNwkHandle();
    ZPS_tsNwkNib *psNib = ZPS_psNwkNibGetHandle( pvNwk);
    switch (sCommission.eState)
    {
        case E_IDLE:
            if (sEvent.eType == APP_E_COMMISION_START)
            {
                bTLinkInProgress = TRUE;
                au64IgnoreList[0] = 0;
                au64IgnoreList[1] = 0;
                au64IgnoreList[2] = 0;
                sCommission.u8Count = 0;
                sScanTarget.u16LQI = 0;
                OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
                sCommission.eState = E_SCANNING;
                sCommission.u32TransactionId = RND_u32GetRand(1, 0xffffffff);
                DBG_vPrintf(TRACE_COMMISSION, "\nRxIdle True");
                MAC_vPibSetRxOnWhenIdle( ZPS_pvAplZdoGetMacHandle(), TRUE, FALSE);
                sCommission.bResponded = FALSE;
                /* Turn down Tx power */
#if ADJUST_POWER
                eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_LOW);
#endif
                sCommission.bSecondScan = FALSE;

                //if ( sZllState.eState != FACTORY_NEW_REMOTE ) {
                //    OS_eStopSWTimer(APP_PollTimer);
                //}
            } else if (sEvent.eType == APP_E_COMMISSION_MSG ) {
                if (sEvent.sZllMessage.eCommand == E_CLD_COMMISSION_CMD_SCAN_REQ )
                {
                    if (sEvent.u8Lqi < ZLL_SCAN_LQI_MIN) {
                        DBG_vPrintf(TRACE_COMMISSION, "\nIgnore scan reqXX");
                        return;

                    }


                    bTLinkInProgress = TRUE;
                    //if ( sZllState.eState != FACTORY_NEW_REMOTE ) {
                    //    OS_eStopSWTimer(APP_PollTimer);
                    //}

jumpHere:
                    DBG_vPrintf(TRACE_SCAN, "Got Scan Req\n");
                    /* Turn down Tx power */
#if ADJUST_POWER
                    eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_LOW);
#endif


                    sDstAddr = sEvent.sZllMessage.sSrcAddr;
                    sDstAddr.u16PanId = 0xFFFF;

                    sCommission.u32ResponseId = RND_u32GetRand(1, 0xffffffff);
                    sCommission.u32TransactionId = sEvent.sZllMessage.uPayload.sScanReqPayload.u32TransactionId;

                    vSendScanResponse( psNib,
                                       &sDstAddr,
                                       sCommission.u32TransactionId,
                                       sCommission.u32ResponseId);

                    sCommission.eState = E_ACTIVE;
                    /* Timer to end inter pan */
                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(ZLL_INTERPAN_LIFE_TIME_SEC), NULL);
                }
            }
            break;

        case E_SCANNING:
            if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED) {
                /*
                 * Send the next scan request
                 */
                vSendScanRequest(pvNwk, &sCommission);
            } else if (sEvent.eType == APP_E_COMMISSION_MSG) {
                switch (sEvent.sZllMessage.eCommand )
                {
                case E_CLD_COMMISSION_CMD_SCAN_RSP:

                    vHandleScanResponse(pvNwk,
                                        psNib,
                                        &sScanTarget,
                                        &sEvent);

                    break;

                case E_CLD_COMMISSION_CMD_SCAN_REQ:
                    /* If we are FN and get a scan req from a NFN remote then
                     * give up our scan and respond to theirs
                     */
                    if (sEvent.u8Lqi < ZLL_SCAN_LQI_MIN) {
                       DBG_vPrintf(TRACE_COMMISSION, "\nIgnore scan req");
                       return;
                    }
                    DBG_vPrintf(TRACE_COMMISSION, "Scan Req1\n");
                    if ((sZllState.eState == FACTORY_NEW_REMOTE) &&
                        ( (sEvent.sZllMessage.uPayload.sScanReqPayload.u8ZllInfo & ZLL_FACTORY_NEW) == 0 )) {

                        DBG_vPrintf(TRACE_COMMISSION, "Scan Req while scanning\n");
                        OS_eStopSWTimer(APP_CommissionTimer);
                        goto jumpHere;
                    }
                    else
                    {
                        if ( !sCommission.bResponded) {
                            /*
                             * Only respond once
                             */
                            sCommission.bResponded = TRUE;

                            sDstAddr = sEvent.sZllMessage.sSrcAddr;
                            sDstAddr.u16PanId = 0xFFFF;

                            sCommission.u32TheirResponseId = RND_u32GetRand(1, 0xffffffff);
                            sCommission.u32TheirTransactionId = sEvent.sZllMessage.uPayload.sScanReqPayload.u32TransactionId;

                            vSendScanResponse(psNib,
                                              &sDstAddr,
                                              sCommission.u32TheirTransactionId,
                                              sCommission.u32TheirResponseId);

                        }
                    }
                    break;

                case E_CLD_COMMISSION_CMD_DEVICE_INFO_REQ:
                    if (sEvent.sZllMessage.uPayload.sDeviceInfoReqPayload.u32TransactionId == sCommission.u32TheirTransactionId) {
                        vHandleDeviceInfoRequest(&sEvent);
                    }
                    break;

                case E_CLD_COMMISSION_CMD_IDENTIFY_REQ:
                    if (sEvent.sZllMessage.uPayload.sIdentifyReqPayload.u32TransactionId == sCommission.u32TheirTransactionId) {
                        vHandleIdentifyRequest(sEvent.sZllMessage.uPayload.sIdentifyReqPayload.u16Duration);
                    }
                    break;

                default:
                    DBG_vPrintf(TRACE_COMMISSION, "Unhandled during scan %02x\n", sEvent.sZllMessage.eCommand );
                    break;
                }

            }

            break;

        case E_SCAN_DONE:
            DBG_vPrintf(TRACE_SCAN, "Scan done target lqi %d\n", sScanTarget.u16LQI);
            if (sScanTarget.u16LQI )
            {
                /* scan done and we have a target */
                eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, sScanTarget.sScanDetails.sScanRspPayload.u8LogicalChannel);
                if ( (sZllState.eState == NOT_FACTORY_NEW_REMOTE) &&
                        ( (sScanTarget.sScanDetails.sScanRspPayload.u8ZllInfo & ZLL_FACTORY_NEW) == 0) &&
                        ( (sScanTarget.sScanDetails.sScanRspPayload.u8ZigbeeInfo & ZLL_TYPE_MASK) == ZLL_ZED ) &&
                        (sScanTarget.sScanDetails.sScanRspPayload.u64ExtPanId == ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid)
                        ) {
                    /* On our network, just gather endpoint details */
                    DBG_vPrintf(TRACE_SCAN, "NFN -> NFN ZED On Our nwk\n");
                    /* tell the app about the target we just touched with */
                    sTarget.u16NwkAddr = sScanTarget.sScanDetails.sScanRspPayload.u16NwkAddr;
                    sTarget.u8Endpoint = sScanTarget.sScanDetails.sScanRspPayload.u8Endpoint;
                    sTarget.u16ProfileID = sScanTarget.sScanDetails.sScanRspPayload.u16ProfileId;
                    sTarget.u16DeviceID = sScanTarget.sScanDetails.sScanRspPayload.u16DeviceId;
                    sTarget.u8Version = sScanTarget.sScanDetails.sScanRspPayload.u8Version;

                    vEndCommissioning(pvNwk, E_INFORM_APP, 6500 );
                    return;
                }
                DBG_vPrintf(TRACE_SCAN, "Send an Id Req to %016llx Id %08x\n", sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr, sCommission.u32TransactionId);
                vSendIdentifyRequest(sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr, sCommission.u32TransactionId, 3);
                OS_eStopSWTimer(APP_CommissionTimer);
                OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(3), NULL);
                sCommission.eState = E_SCAN_WAIT_ID;
            }
            else
            {
                DBG_vPrintf(TRACE_SCAN, "scan No results\n");
                if (sCommission.bSecondScan)
                {
                    vEndCommissioning(pvNwk, E_IDLE, 0);
                    return;
                }
                else
                {
                    DBG_vPrintf(TRACE_SCAN,  "Second scan\n");
                    sCommission.eState = E_SCANNING;
                    sCommission.bSecondScan = TRUE;
                    #ifdef ZLL_PRIMARY
                        sCommission.u8Count = 12;
                    #else
                        sCommission.u8Count = 11;
                    #endif
                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
                }
            }
            break;

        case E_SCAN_WAIT_ID:
            if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED)
            {
                DBG_vPrintf(TRACE_SCAN, "Wait id time time out\n");

                /* send a device info req */
                vSendDeviceInfoReq(sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr, sCommission.u32TransactionId, 0);

                OS_eStopSWTimer(APP_CommissionTimer);
                OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(10), NULL);
                sCommission.eState = E_SCAN_WAIT_INFO;
            }
            else if (sEvent.eType == APP_E_COMMISION_START)
            {
                DBG_vPrintf(TRACE_SCAN, "Got Key during wait id\n");
                if ( au64IgnoreList[0] == 0 )
                {
                    DBG_vPrintf(TRACE_SCAN, "1st black list %016llx\n", sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr);
                    au64IgnoreList[0] = sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr;
                }
                else if ( au64IgnoreList[1] == 0 )
                {
                    DBG_vPrintf(TRACE_SCAN, "2nd black list %016llx\n", sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr);
                    au64IgnoreList[1] = sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr;
                }
                else if ( au64IgnoreList[2] == 0 )
                {
                    DBG_vPrintf(TRACE_SCAN, "3rd black list %016llx\n", sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr);
                    au64IgnoreList[2] = sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr;
                }
                else
                {
                    DBG_vPrintf(TRACE_SCAN, "Run out give up\n");
                    vEndCommissioning(pvNwk, E_IDLE, 0);
                    return;
                }
                /* stop that one identifying */
                vSendIdentifyRequest(sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr, sCommission.u32TransactionId, 0);
                OS_eStopSWTimer(APP_CommissionTimer);
                OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
                /* restart scanning */
                sCommission.eState = E_SCANNING;

                sCommission.u8Count = 0;
                sScanTarget.u16LQI = 0;
                sCommission.u32TransactionId = RND_u32GetRand(1, 0xffffffff);
                sCommission.bResponded = FALSE;
                sCommission.bSecondScan = FALSE;

                bTLinkInProgress = TRUE;
            }
            break;

        case E_SCAN_WAIT_INFO:

            if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED)
            {
                DBG_vPrintf(TRACE_SCAN, "Wait Info time out\n");
                vEndCommissioning(pvNwk, E_IDLE, 0);
                return;
            }
            else if (sEvent.eType == APP_E_COMMISSION_MSG)
            {
                DBG_vPrintf(TRACE_SCAN, "Wsit info com msg Cmd=%d  %d\n", sEvent.sZllMessage.eCommand, E_CLD_COMMISSION_CMD_DEVICE_INFO_RSP);
                if ( ( sEvent.sZllMessage.eCommand == E_CLD_COMMISSION_CMD_DEVICE_INFO_RSP )
                        &&  ( sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.u32TransactionId == sCommission.u32TransactionId ) )
                {
                    DBG_vPrintf(TRACE_SCAN, "Got Device Info Rsp\n");

                    if ( (sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.u8StartIndex == 0) &&
                         (sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.u8DeviceInfoRecordCount > 0))
                    {
                        /* catch the first device */
                        sScanTarget.sScanDetails.sScanRspPayload.u16ProfileId = sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.asDeviceRecords[0].u16ProfileId;
                        sScanTarget.sScanDetails.sScanRspPayload.u16DeviceId = sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.asDeviceRecords[0].u16DeviceId;
                        sScanTarget.sScanDetails.sScanRspPayload.u8Endpoint = sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.asDeviceRecords[0].u8Endpoint;
                        sScanTarget.sScanDetails.sScanRspPayload.u8Version = sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.asDeviceRecords[0].u8Version;
                        sScanTarget.sScanDetails.sScanRspPayload.u8GroupIdCount = sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.asDeviceRecords[0].u8NumberGroupIds;

                    }

                    if ( (sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.u8DeviceInfoRecordCount +
                            sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.u8StartIndex)
                            < sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.u8NumberSubDevices)
                    {
                        uint8 u8Index = sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.u8StartIndex + sEvent.sZllMessage.uPayload.sDeviceInfoRspPayload.u8DeviceInfoRecordCount;
                        vSendDeviceInfoReq(sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr, sCommission.u32TransactionId, u8Index);
                    }
                    else
                    {

                        sTarget.u16ProfileID = sScanTarget.sScanDetails.sScanRspPayload.u16ProfileId;
                        sTarget.u16DeviceID = sScanTarget.sScanDetails.sScanRspPayload.u16DeviceId;
                        sTarget.u8Endpoint = sScanTarget.sScanDetails.sScanRspPayload.u8Endpoint;
                        sTarget.u8Version = sScanTarget.sScanDetails.sScanRspPayload.u8Version;
                        // nwk addr will be assigned in a bit

                        sDstAddr.eMode = 3;
                        sDstAddr.u16PanId = 0xffff;
                        sDstAddr.uAddress.u64Addr = sScanTarget.sScanDetails.sSrcAddr.uAddress.u64Addr;

                        DBG_vPrintf(TRACE_SCAN, "Back to %016llx Mode %d\n", sDstAddr.uAddress.u64Addr, sDstAddr.eMode);

                        if(bSendFactoryResetOverAir)
                        {
                            bSendFactoryResetOverAir=FALSE;
                            OS_eStopSWTimer(APP_CommissionTimer);
                            vSendFactoryResetRequest( pvNwk,
                                               psNib,
                                               &sScanTarget,
                                               sCommission.u32TransactionId);
                            sCommission.eState = E_SCAN_WAIT_RESET_SENT;
                            /* wait a bit for message to go, then finsh up */
                            OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
                            DBG_vPrintf(TRACE_COMMISSION, "\nvSendFactoryResetRequest and going to E_IDLE\n");
                        }
                        else if (sZllState.eState == FACTORY_NEW_REMOTE)
                        {
                            // we are FN ZED, target must be a FN or NFN router
                            vSendStartRequest(pvNwk,psNib,&sScanTarget,sCommission.u32TransactionId);
                            sCommission.eState = E_WAIT_START_RSP;
                            DBG_vPrintf(TRACE_JOIN, "Sent start req\n");
                            OS_eStopSWTimer(APP_CommissionTimer);
                            OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(2), NULL);
                        }
                        else
                        {
                            DBG_vPrintf(TRACE_SCAN, "NFN -> ??? %02x\n", sScanTarget.sScanDetails.sScanRspPayload.u8ZllInfo);
                            if (sScanTarget.sScanDetails.sScanRspPayload.u8ZllInfo & ZLL_FACTORY_NEW)
                            {
                                /* FN target */
                                if ((sScanTarget.sScanDetails.sScanRspPayload.u8ZigbeeInfo & ZLL_TYPE_MASK) != ZLL_ZED)
                                {
                                    DBG_vPrintf(TRACE_SCAN, "NFN -> FN ZR\n");
                                    /* router join */
                                    vSendRouterJoinRequest(pvNwk, psNib, &sScanTarget, sCommission.u32TransactionId);
                                    sCommission.eState = E_WAIT_JOIN_RTR_RSP;
                                    OS_eStopSWTimer(APP_CommissionTimer);
                                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(2), NULL);

                                }
                                else
                                {
                                    DBG_vPrintf(TRACE_SCAN, "NFN -> FN ZED\n");
                                    /* end device join */
                                    vSendEndDeviceJoinRequest( pvNwk, psNib, &sScanTarget, sCommission.u32TransactionId);
                                    sCommission.eState = E_WAIT_JOIN_ZED_RSP;
                                    OS_eStopSWTimer(APP_CommissionTimer);
                                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(2), NULL);
                                }
                            }
                            else
                            {
                                /* NFN target, must be ZR on different nwk */

                                //DBG_vPrintf(TRACE_COMMISSION, "From Pan %016llx\n", sScanTarget.sScanDetails.sScanRspPayload.u64ExtPanId);
                                //DBG_vPrintf(TRACE_COMMISSION, "NFN -> NFN ZR Our Pan %016llx\n", ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid);
                                if (sScanTarget.sScanDetails.sScanRspPayload.u64ExtPanId == ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid) {
                                    DBG_vPrintf(TRACE_SCAN, "On our nwk\n"  );
                                    /* On our network, just gather endpoint details */
                                    DBG_vPrintf(TRACE_SCAN, "NFN -> NFN ZR On Our nwk\n");
                                    /* tell the app about the target we just touched with */
                                    sTarget.u16NwkAddr = sScanTarget.sScanDetails.sScanRspPayload.u16NwkAddr;
                                    sTarget.u8Endpoint = sScanTarget.sScanDetails.sScanRspPayload.u8Endpoint;
                                    sTarget.u16ProfileID = sScanTarget.sScanDetails.sScanRspPayload.u16ProfileId;
                                    sTarget.u16DeviceID = sScanTarget.sScanDetails.sScanRspPayload.u16DeviceId;
                                    sTarget.u8Version = sScanTarget.sScanDetails.sScanRspPayload.u8Version;

                                    vEndCommissioning(pvNwk, E_INFORM_APP, 1500);
                                    return;
                                }
                                DBG_vPrintf(TRACE_SCAN, "NFN -> NFN ZR other nwk\n");

                                vSendRouterJoinRequest( pvNwk, psNib, &sScanTarget, sCommission.u32TransactionId);
                                sCommission.eState = E_WAIT_JOIN_RTR_RSP;
                                OS_eStopSWTimer(APP_CommissionTimer);
                                OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(2), NULL);
                            }
                        }
                    }
                }
            }
            break;

        case E_SCAN_WAIT_RESET_SENT:
            /* Factory reset command has been sent,
             * end commissioning
             */
            vEndCommissioning(pvNwk, E_IDLE, 0);
            break;

        case E_WAIT_START_RSP:
            if (sEvent.eType == APP_E_COMMISSION_MSG) {
                DBG_vPrintf(TRACE_JOIN, "Start Rsp %d\n", sEvent.sZllMessage.eCommand);
                if (sEvent.sZllMessage.eCommand == E_CLD_COMMISSION_CMD_NETWORK_START_RSP) {
                    vHandleNwkStartResponse(pvNwk, &sEvent, &sCommission);
                }
            }
            else if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED)  {
                DBG_vPrintf(TRACE_COMMISSION, "wait startrsp time out\n");

                vEndCommissioning(pvNwk, E_IDLE, 0);
            }
             break;

        case E_WAIT_JOIN_RTR_RSP:
            if (sEvent.eType == APP_E_COMMISSION_MSG) {
                if (sEvent.sZllMessage.eCommand == E_CLD_COMMISSION_CMD_NETWORK_JOIN_ROUTER_RSP) {
                    if (sEvent.sZllMessage.uPayload.sNwkJoinRouterRspPayload.u8Status == ZLL_SUCCESS) {
                        DBG_vPrintf(TRACE_JOIN, "Got Router join Rsp %d\n", sEvent.sZllMessage.uPayload.sNwkJoinRouterRspPayload.u8Status);
                        OS_eStopSWTimer(APP_CommissionTimer);
                        OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(ZLL_START_UP_TIME_SEC), NULL);
                        bDoRejoin = TRUE;
                        bWithDiscovery = FALSE;
                        sCommission.eState = E_WAIT_START_UP;
                    } else {
                        /* recover assigned addresses */
                        vRecoverTempAddresses();
                        vEndCommissioning(pvNwk, E_IDLE, 0);
                    }
                }
            }
            else if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED)
            {
                DBG_vPrintf(TRACE_COMMISSION, "zr join time out\n");

                vEndCommissioning(pvNwk, E_IDLE, 0);
            }
            break;

        case E_WAIT_JOIN_ZED_RSP:
            if (sEvent.eType == APP_E_COMMISSION_MSG) {
                if (sEvent.sZllMessage.eCommand == E_CLD_COMMISSION_CMD_NETWORK_JOIN_END_DEVICE_RSP) {
                    if (sEvent.sZllMessage.uPayload.sNwkJoinEndDeviceRspPayload.u8Status == ZLL_SUCCESS) {
                        DBG_vPrintf(TRACE_JOIN, "Got Zed join Rsp %d\n", sEvent.sZllMessage.uPayload.sNwkJoinEndDeviceRspPayload.u8Status);
                        OS_eStopSWTimer(APP_CommissionTimer);
                        OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(1 /*ZLL_START_UP_TIME_SEC*/), NULL);
                        bDoRejoin = FALSE;
                        sCommission.eState = E_WAIT_START_UP;
                    } else {
                        /* recover assigned addresses */
                        vRecoverTempAddresses();
                        vEndCommissioning(pvNwk, E_IDLE, 0);
                    }
                }
            }
            else if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED)
            {
                DBG_vPrintf(TRACE_COMMISSION, "ed join time out\n");

                vEndCommissioning(pvNwk, E_IDLE, 0);
            }
            break;

        /*
         * We are the target of a scan
         */
        case E_ACTIVE:
            /* handle process after scan rsp */
            if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED)
            {
                DBG_vPrintf(TRACE_COMMISSION, "Active time out\n");
                vEndCommissioning(pvNwk, E_IDLE, 0);
            }
            else if (sEvent.eType == APP_E_COMMISSION_MSG)
            {
                DBG_vPrintf(TRACE_COMMISSION, "Zll cmd %d in active\n", sEvent.sZllMessage.eCommand);
                if (sEvent.sZllMessage.uPayload.sScanReqPayload.u32TransactionId ==  sCommission.u32TransactionId)
                {
                    /* Set up return address */
                    sDstAddr = sEvent.sZllMessage.sSrcAddr;
                    sDstAddr.u16PanId = 0xffff;
                    switch(sEvent.sZllMessage.eCommand)
                    {
                        /* if we receive the factory reset clear the persisted data and do a reset.
                         */
                        case E_CLD_COMMISSION_CMD_FACTORY_RESET_REQ:
                            if (sZllState.eState == NOT_FACTORY_NEW_REMOTE) {
                                sCommission.eState = E_WAIT_LEAVE_RESET;
                                /* leave req */
                                u32OldFrameCtr = psNib->sTbl.u32OutFC + 10;
                                ZPS_eAplZdoLeaveNetwork(0, FALSE,FALSE);
                            }
                            break;
                        case E_CLD_COMMISSION_CMD_NETWORK_UPDATE_REQ:
                            vHandleNwkUpdateRequest(pvNwk,
                                                    psNib,
                                                    &sEvent.sZllMessage.uPayload.sNwkUpdateReqPayload);
                            break;

                        case E_CLD_COMMISSION_CMD_IDENTIFY_REQ:
                             vHandleIdentifyRequest(sEvent.sZllMessage.uPayload.sIdentifyReqPayload.u16Duration);
                            break;

                        case E_CLD_COMMISSION_CMD_DEVICE_INFO_REQ:
                            vHandleDeviceInfoRequest(&sEvent);
                            break;

                        case E_CLD_COMMISSION_CMD_NETWORK_JOIN_ROUTER_REQ:
                            /* we are a ZED send error */
                            vHandleRouterJoinRequest( &sDstAddr,
                                                      sCommission.u32TransactionId);
                            break;

                        case E_CLD_COMMISSION_CMD_NETWORK_START_REQ:
                            vHandleNwkStartRequest( &sDstAddr,
                                                    sCommission.u32TransactionId);
                            break;

                        case E_CLD_COMMISSION_CMD_NETWORK_JOIN_END_DEVICE_REQ:
                            vHandleEndDeviceJoinRequest( pvNwk,
                                                         psNib,
                                                         &sEvent,
                                                         &sCommission);
                            break;

                        default:
                            break;

                    }               /* endof switch(sEvent.sZllMessage.eCommand) */
                }
            }               /* endof if (sEvent.eType == APP_E_COMMISSION_MSG) */

            break;

        case E_WAIT_LEAVE:
            break;

        case E_WAIT_LEAVE_RESET:
            if ((sEvent.eType == APP_E_COMMISSION_LEAVE_CFM) ||
                 (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED )) {
                DBG_vPrintf(TRACE_JOIN,"WARNING: Received Factory reset \n");
                //PDM_vDelete();
                PDM_vDeleteAllDataRecords();
                vAHI_SwReset();
            }
            break;

        case E_WAIT_START_UP:
            if (sEvent.eType == APP_E_COMMISSION_TIMER_EXPIRED) {
                #if ADJUST_POWER
                eAppApiPlmeSet(PHY_PIB_ATTR_TX_POWER, TX_POWER_NORMAL);
                #endif

                DBG_vPrintf(TRACE_COMMISSION, "\nRxIdle False");
                MAC_vPibSetRxOnWhenIdle( ZPS_pvAplZdoGetMacHandle(), FALSE, FALSE);
                if ( sZllState.eState == FACTORY_NEW_REMOTE ) {
                    ZPS_eAplAibSetApsTrustCenterAddress( 0xffffffffffffffffULL );
                    sZllState.eState = NOT_FACTORY_NEW_REMOTE;
                } else {
                    DBG_vPrintf(TRACE_JOIN, "Back on Ch %d\n", sZllState.u8MyChannel);
                    eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, sZllState.u8MyChannel);

                    DBG_vPrintf(TRACE_JOIN, "restart poll timer\n");

                }

                DBG_vPrintf(TRACE_JOIN, "Free addr %04x to %04x\n", sZllState.u16FreeAddrLow, sZllState.u16FreeAddrHigh);
                DBG_vPrintf(TRACE_JOIN, "Free Gp  %04x to %04x\n", sZllState.u16FreeGroupLow, sZllState.u16FreeGroupHigh);

                sCommission.eState = E_INFORM_APP;
                //PDM_vSaveRecord( &sZllPDDesc);
                PDM_eSaveRecordData(PDM_ID_APP_ZLL_CMSSION, &sZllState,sizeof(tsZllRemoteState));
                sCommission.u32TransactionId = 0;
                sCommission.bResponded = FALSE;
                if (bDoRejoin == TRUE)
                {
                    DBG_vPrintf(TRACE_JOIN, "TLINK: Req rejoin\n");
                    if (bWithDiscovery == FALSE)
                    {
                        vSetUpParent( psNib, sTarget.u16NwkAddr);
                    }
                    bFailedToJoin = FALSE;
                    vSetRejoinFilter();
                    DBG_vPrintf(TRACE_JOIN, "\n ZPS_eAplZdoRejoinNetwork 1");
                    ZPS_eAplZdoRejoinNetwork(bWithDiscovery);

                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(3000), NULL );
                    DBG_vPrintf(TRACE_JOIN, "\nWait inform 3000");
                }
                else
                {
                    OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(1500), NULL );
                    DBG_vPrintf(TRACE_JOIN, "\nWait inform 1500");
                    DBG_vPrintf(TRACE_JOIN, "restart poll timer\n");
                    vStartPolling();
                }

                //OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(1500), NULL );
                //DBG_vPrintf(TRACE_JOIN, "\nWait inform");
            }
            break;

        case E_INFORM_APP:
            /* tell the ap about the target we just joined with */
            if (sTarget.u16NwkAddr > 0) {
                sAppEvent.eType = APP_E_EVENT_TOUCH_LINK;
                sAppEvent.uEvent.sTouchLink = sTarget;
                DBG_vPrintf(TRACE_COMMISSION, "\nInform on %04x", sTarget.u16NwkAddr);
                OS_ePostMessage(APP_msgEvents, &sAppEvent);
            }
            sCommission.eState = E_IDLE;
            bTLinkInProgress = FALSE;
            DBG_vPrintf(TRACE_COMMISSION, "\nInform App, stop");
            break;

        default:
            DBG_vPrintf(TRACE_COMMISSION, "Unhandled event %d\n", sEvent.eType);
            break;
    }
}

/****************************************************************************
 *
 * NAME: vSendScanRequest
 *
 * DESCRIPTION: sends a touch link scan request to a touch link target
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendScanRequest(void *pvNwk, tsCommissionData *psCommission)
{
    ZPS_tsInterPanAddress       sDstAddr;
    tsCLD_ZllCommission_ScanReqCommandPayload                   sScanReqPayload;
    uint8 u8Seq;


    if (((psCommission->u8Count == 8) && !psCommission->bSecondScan) ||
        ( (psCommission->u8Count == 27) && psCommission->bSecondScan ) ){
        psCommission->eState = E_SCAN_DONE;
        OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
    } else {

        if (psCommission->bSecondScan) {
            DBG_vPrintf(TRACE_SCAN, "2nd Scan Ch %d\n", psCommission->u8Count);
            eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, psCommission->u8Count++);
            if ( (psCommission->u8Count == ZLL_SKIP_CH1) || (psCommission->u8Count == ZLL_SKIP_CH2) || (psCommission->u8Count == ZLL_SKIP_CH3) || (sCommission.u8Count == ZLL_SKIP_CH4) ) {
                psCommission->u8Count++;
#ifdef ZLL_PRIMARY_PLUS3
            if (psCommission->u8Count==ZLL_SKIP_CH4) {psCommission->u8Count++;}
#endif
            }
        } else {
            DBG_vPrintf(TRACE_SCAN, "\nScan Ch %d", au8ZLLChannelSet[psCommission->u8Count]);
            eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, au8ZLLChannelSet[psCommission->u8Count++]);
        }

        sDstAddr.eMode = ZPS_E_AM_INTERPAN_SHORT;
        sDstAddr.u16PanId = 0xffff;
        sDstAddr.uAddress.u16Addr = 0xffff;
        sScanReqPayload.u32TransactionId = psCommission->u32TransactionId;
        sScanReqPayload.u8ZigbeeInfo =  ZLL_ZED;              // Rxon idle router

        sScanReqPayload.u8ZllInfo =
            (sZllState.eState == FACTORY_NEW_REMOTE)? (ZLL_FACTORY_NEW|ZLL_LINK_INIT|ZLL_ADDR_ASSIGN):(ZLL_LINK_INIT|ZLL_ADDR_ASSIGN);                  // factory New Addr assign Initiating

        //DBG_vPrintf(TRACE_COMMISSION, "Scan Req ret %02x\n",  );
        eCLD_ZllCommissionCommandScanReqCommandSend(
                                    &sDstAddr,
                                     &u8Seq,
                                     &sScanReqPayload);
        OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(ZLL_SCAN_RX_TIME_MS), NULL);
    }
}

/****************************************************************************
 *
 * NAME: vSendScanResponse
 *
 * DESCRIPTION: sends a scan response following a touch link scan request
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendScanResponse(ZPS_tsNwkNib *psNib,
                               ZPS_tsInterPanAddress       *psDstAddr,
                               uint32 u32TransactionId,
                               uint32 u32ResponseId)
{
    tsCLD_ZllCommission_ScanRspCommandPayload                   sScanRsp;
    uint8 u8Seq;
    uint8 i;


    memset(&sScanRsp, 0, sizeof(tsCLD_ZllCommission_ScanRspCommandPayload));

    sScanRsp.u32TransactionId = u32TransactionId;
    sScanRsp.u32ResponseId = u32ResponseId;


    sScanRsp.u8RSSICorrection = 0;
    sScanRsp.u8ZigbeeInfo = ZLL_ZED;
    sScanRsp.u16KeyMask = ZLL_SUPPORTED_KEYS;
    if (sZllState.eState == FACTORY_NEW_REMOTE) {
        sScanRsp.u8ZllInfo = ZLL_FACTORY_NEW|ZLL_ADDR_ASSIGN;
        // Ext Pan 0
        // nwk update 0
        // logical channel zero
        // Pan Id 0
        sScanRsp.u16NwkAddr = 0xffff;
    } else {
        sScanRsp.u8ZllInfo = ZLL_ADDR_ASSIGN;
        sScanRsp.u64ExtPanId = ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid;
        sScanRsp.u8NwkUpdateId = psNib->sPersist.u8UpdateId;
        sScanRsp.u16PanId = psNib->sPersist.u16VsPanId;
        sScanRsp.u16NwkAddr = psNib->sPersist.u16NwkAddr;
    }

    sScanRsp.u8LogicalChannel = psNib->sPersist.u8VsChannel;
    for (i=0; i<sDeviceTable.u8NumberDevices; i++) {
        sScanRsp.u8TotalGroupIds += sDeviceTable.asDeviceRecords[i].u8NumberGroupIds;
    }
    sScanRsp.u8NumberSubDevices = sDeviceTable.u8NumberDevices;
    if (sScanRsp.u8NumberSubDevices == 1) {
        sScanRsp.u8Endpoint = sDeviceTable.asDeviceRecords[0].u8Endpoint;
        sScanRsp.u16ProfileId = sDeviceTable.asDeviceRecords[0].u16ProfileId;
        sScanRsp.u16DeviceId = sDeviceTable.asDeviceRecords[0].u16DeviceId;
        sScanRsp.u8Version = sDeviceTable.asDeviceRecords[0].u8Version;
        sScanRsp.u8GroupIdCount = sDeviceTable.asDeviceRecords[0].u8NumberGroupIds;
    }

    eCLD_ZllCommissionCommandScanRspCommandSend( psDstAddr,
            &u8Seq,
            &sScanRsp);
}

/****************************************************************************
 *
 * NAME: vHandleScanResponse
 *
 * DESCRIPTION: handles scan responses, decodes if the responder
 * should become the target for the touch link
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleScanResponse(void *pvNwk,
                                 ZPS_tsNwkNib *psNib,
                                 tsZllScanTarget *psScanTarget,
                                 APP_CommissionEvent         *psEvent)
{
    uint16 u16AdjustedLqi;
    ZPS_tsInterPanAddress       sDstAddr;

    uint8 u8Seq;

    //DBG_vPrintf(TRACE_COMMISSION, "Scan Rsp\n");
    u16AdjustedLqi = psEvent->u8Lqi + psEvent->sZllMessage.uPayload.sScanRspPayload.u8RSSICorrection;
    if (u16AdjustedLqi < ZLL_SCAN_LQI_MIN ) {
        DBG_vPrintf(TRACE_SCAN, "Reject LQI %d\n", psEvent->u8Lqi);
        return;
    }

    DBG_vPrintf(TRACE_COMMISSION, "Their %016llx Mine %016llx TC %016llx\n",
            psEvent->sZllMessage.uPayload.sScanRspPayload.u64ExtPanId,
            ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid,
            ZPS_eAplAibGetApsTrustCenterAddress()
            );
    if ( (sZllState.eState == NOT_FACTORY_NEW_REMOTE) &&
            (psEvent->sZllMessage.uPayload.sScanRspPayload.u64ExtPanId != ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid) &&
            ( ZPS_eAplAibGetApsTrustCenterAddress() != 0xffffffffffffffffULL )
            )
    {
        DBG_vPrintf(TRACE_COMMISSION, "Non Zll nwk, target not my nwk\n");
        return;
    }

    if ( (ZLL_SUPPORTED_KEYS & psEvent->sZllMessage.uPayload.sScanRspPayload.u16KeyMask) == 0)
    {
        // No common supported key index
        if ( (sZllState.eState == FACTORY_NEW_REMOTE) ||
            ( (sZllState.eState == NOT_FACTORY_NEW_REMOTE) && (psEvent->sZllMessage.uPayload.sScanRspPayload.u64ExtPanId != ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid) )
                )
        {
            /* Either we are factory new or
             * we are not factory new and the target is on another pan
             * there is no common key index, to exchange keys so drop the scan response without further action
             */
            DBG_vPrintf(TRACE_COMMISSION, "No common security level, target not my nwk\n");
            return;
        }
    }

    /*
     * Check for Nwk Update Ids
     */
    if ( (sZllState.eState == NOT_FACTORY_NEW_REMOTE) &&
         (psEvent->sZllMessage.uPayload.sScanRspPayload.u64ExtPanId == ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid) &&
         (psEvent->sZllMessage.uPayload.sScanRspPayload.u16PanId == psNib->sPersist.u16VsPanId))
    {
        DBG_vPrintf(TRACE_COMMISSION, "Our Nwk Id %d theirs %d\n", psNib->sPersist.u8UpdateId, psEvent->sZllMessage.uPayload.sScanRspPayload.u8NwkUpdateId );
        if (psNib->sPersist.u8UpdateId != psEvent->sZllMessage.uPayload.sScanRspPayload.u8NwkUpdateId ) {
            if ( psNib->sPersist.u8UpdateId == u8NewUpdateID(psNib->sPersist.u8UpdateId, psEvent->sZllMessage.uPayload.sScanRspPayload.u8NwkUpdateId) ) {
                DBG_vPrintf(TRACE_COMMISSION, "Use ours on %d\n", sZllState.u8MyChannel);
                /*
                 * Send them a network update request
                 */
                tsCLD_ZllCommission_NetworkUpdateReqCommandPayload          sNwkUpdateReqPayload;

                sDstAddr.eMode = ZPS_E_AM_INTERPAN_IEEE;
                sDstAddr.u16PanId = 0xffff;
                sDstAddr.uAddress.u64Addr = psEvent->sZllMessage.sSrcAddr.uAddress.u64Addr;

                sNwkUpdateReqPayload.u32TransactionId = sCommission.u32TransactionId;
                sNwkUpdateReqPayload.u16NwkAddr = psEvent->sZllMessage.uPayload.sScanRspPayload.u16NwkAddr;
                sNwkUpdateReqPayload.u64ExtPanId = ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid;
                sNwkUpdateReqPayload.u8NwkUpdateId = psNib->sPersist.u8UpdateId;
                sNwkUpdateReqPayload.u8LogicalChannel = sZllState.u8MyChannel;

                sNwkUpdateReqPayload.u16PanId = psNib->sPersist.u16VsPanId;


                eCLD_ZllCommissionCommandNetworkUpdateReqCommandSend(
                                            &sDstAddr,
                                            &u8Seq,
                                            &sNwkUpdateReqPayload);

                return;

            } else {
                //DBG_vPrintf(TRACE_COMMISSION, "Use theirs\n");
                /*
                 * Part of a network and our Update Id is out of date
                 */
                ZPS_vNwkNibSetUpdateId( pvNwk, psEvent->sZllMessage.uPayload.sScanRspPayload.u8NwkUpdateId);
                sZllState.u8MyChannel = psEvent->sZllMessage.uPayload.sScanRspPayload.u8LogicalChannel;

                vEndCommissioning(pvNwk, E_IDLE, 0);

                DBG_vPrintf(TRACE_SCAN, "Update Id and rejoin\n");
                vSetRejoinFilter();
                DBG_vPrintf(TRACE_SCAN, "\n ZPS_eAplZdoRejoinNetwork 2");
                ZPS_eAplZdoRejoinNetwork(FALSE);
                return;
            }
        }
    }

    if ( ((sZllState.eState == FACTORY_NEW_REMOTE) &&
          ((psEvent->sZllMessage.uPayload.sScanRspPayload.u8ZigbeeInfo & ZLL_TYPE_MASK) == ZLL_ZED)) ||
         ( (sZllState.eState == NOT_FACTORY_NEW_REMOTE) &&
                 ((psEvent->sZllMessage.uPayload.sScanRspPayload.u8ZigbeeInfo & ZLL_TYPE_MASK) == ZLL_ZED) &&
                 !(psEvent->sZllMessage.uPayload.sScanRspPayload.u8ZllInfo & ZLL_FACTORY_NEW) &&
                 (psEvent->sZllMessage.uPayload.sScanRspPayload.u64ExtPanId != ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid))
    )
    {
        DBG_vPrintf(TRACE_SCAN, "Drop scan result\n");
    }
    else
    {
        if (u16AdjustedLqi > psScanTarget->u16LQI) {
            if (  ( au64IgnoreList[0] != psEvent->sZllMessage.sSrcAddr.uAddress.u64Addr  ) &&
                    (au64IgnoreList[1] != psEvent->sZllMessage.sSrcAddr.uAddress.u64Addr ) &&
                    (au64IgnoreList[2] != psEvent->sZllMessage.sSrcAddr.uAddress.u64Addr )
              )
            {
                psScanTarget->u16LQI = u16AdjustedLqi;
                /*
                 *  joining scenarios we are a ZED
                 * 1) Any router,
                 * 2) we are NFN then FN device
                 * 3) we are NFN then any NFN ED that is not on our Pan
                 */
                DBG_vPrintf(TRACE_SCAN, "Accept %d from %016llx\n", psScanTarget->u16LQI, psEvent->sZllMessage.sSrcAddr.uAddress.u64Addr );
                psScanTarget->sScanDetails.sSrcAddr = psEvent->sZllMessage.sSrcAddr;
                psScanTarget->sScanDetails.sScanRspPayload = psEvent->sZllMessage.uPayload.sScanRspPayload;

                /* If the number of sub devices was not 1, the device and ep details will be missing
                 * these must be got from the device info request response
                 */
            }
            else
            {
                DBG_vPrintf(TRACE_SCAN, "%016llx is on the black list\n", psEvent->sZllMessage.sSrcAddr.uAddress.u64Addr);
            }

        }/* else {DBG_vPrintf(TRACE_COMMISSION, "Drop lower lqi Old %d Rej%d\n", psScanTarget->u16LQI, u16AdjustedLqi);}*/
    }
}

/****************************************************************************
 *
 * NAME: vSendDeviceInfoReq
 *
 * DESCRIPTION: Sends the touch link device info request
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendDeviceInfoReq(uint64 u64Addr, uint32 u32TransactionId, uint8 u8Index) {
    ZPS_tsInterPanAddress       sDstAddr;
    uint8 u8Seq;
    tsCLD_ZllCommission_DeviceInfoReqCommandPayload             sDeviceInfoReqPayload;

    sDstAddr.eMode = ZPS_E_AM_INTERPAN_IEEE;
    sDstAddr.u16PanId = 0xffff;
    sDstAddr.uAddress.u64Addr = u64Addr;

    sDeviceInfoReqPayload.u32TransactionId = u32TransactionId;
    sDeviceInfoReqPayload.u8StartIndex = u8Index;
    eCLD_ZllCommissionCommandDeviceInfoReqCommandSend( &sDstAddr,
                                                       &u8Seq,
                                                       &sDeviceInfoReqPayload );
}


/****************************************************************************
 *
 * NAME: vSendStartRequest
 *
 * DESCRIPTION: send a network start request to touch link target
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendStartRequest(void *pvNwk,
                               ZPS_tsNwkNib *psNib,
                               tsZllScanTarget *psScanTarget,
                               uint32 u32TransactionId )
{
    ZPS_tsInterPanAddress       sDstAddr;
    tsCLD_ZllCommission_NetworkStartReqCommandPayload           sNwkStartReqPayload;
    uint8 u8Seq;
    uint16 u16KeyMask;
    int i;

    if (psScanTarget->sScanDetails.sScanRspPayload.u8ZllInfo & ZLL_FACTORY_NEW) {
        DBG_vPrintf(TRACE_SCAN, "FN -> FN Router target\n");
        /* nwk start */
    } else {
        DBG_vPrintf(TRACE_SCAN, "FN -> NFN Router target\n");
        /* nwk start */
    }
    memset(&sNwkStartReqPayload, 0, sizeof(tsCLD_ZllCommission_NetworkStartReqCommandPayload));
    sNwkStartReqPayload.u32TransactionId = sCommission.u32TransactionId;
    // ext pan 0, target decides
#if FIX_CHANNEL
    sNwkStartReqPayload.u8LogicalChannel = au8ZLLChannelSet[0];
#endif
    u16KeyMask = ZLL_SUPPORTED_KEYS & psScanTarget->sScanDetails.sScanRspPayload.u16KeyMask;

    if (u16KeyMask & ZLL_MASTER_KEY_MASK) {
        sNwkStartReqPayload.u8KeyIndex = ZLL_MASTER_KEY_INDEX;
    } else if (u16KeyMask & ZLL_CERTIFICATION_KEY_MASK) {
        sNwkStartReqPayload.u8KeyIndex = ZLL_CERTIFICATION_KEY_INDEX;
    } else {
        sNwkStartReqPayload.u8KeyIndex = ZLL_TEST_KEY_INDEX;
    }

    for (i=0; i<16; i++) {
#if RAND_KEY
        psNib->sTbl.psSecMatSet[0].au8Key[i] = (uint8)(RND_u32GetRand256() & 0xFF);
#else
        psNib->sTbl.psSecMatSet[0].au8Key[i] = 0x12;
#endif
    }
    psNib->sTbl.psSecMatSet[0].u8KeySeqNum = 0;
    memset(psNib->sTbl.pu32InFCSet, 0,
            (sizeof(uint32) * psNib->sTblSize.u16NtActv));
    psNib->sTbl.psSecMatSet[0].u8KeyType = ZPS_NWK_SEC_NETWORK_KEY;
    eEncryptKey(psNib->sTbl.psSecMatSet[0].au8Key,
            sNwkStartReqPayload.au8NwkKey,
            u32TransactionId,
            psScanTarget->sScanDetails.sScanRspPayload.u32ResponseId,
            sNwkStartReqPayload.u8KeyIndex);

    /* save security material to flash */
    ZPS_vNwkSaveSecMat( pvNwk);

    /* Make this the Active Key */
    ZPS_vNwkNibSetKeySeqNum( pvNwk, 0);

    // Pan Id 0 target decides

    /* temp save addreess and group ranges */
    vSaveTempAddresses();

    // take our group address requirement
    vSetGroupAddress(sZllState.u16FreeGroupLow, GROUPS_REQUIRED);
    sZllState.u16FreeGroupLow += GROUPS_REQUIRED;

    sNwkStartReqPayload.u16InitiatorNwkAddr = sZllState.u16FreeAddrLow;
    sZllState.u16MyAddr = sZllState.u16FreeAddrLow++;
    sTarget.u16NwkAddr = sZllState.u16FreeAddrLow;
    sNwkStartReqPayload.u16NwkAddr = sZllState.u16FreeAddrLow++;
    DBG_vPrintf(TRACE_COMMISSION, "Give it addr %04x\n", sNwkStartReqPayload.u16NwkAddr);
    if (psScanTarget->sScanDetails.sScanRspPayload.u8ZllInfo & ZLL_ADDR_ASSIGN) {
        DBG_vPrintf(TRACE_JOIN, "Assign addresses\n");

        sNwkStartReqPayload.u16FreeNwkAddrBegin = ((sZllState.u16FreeAddrLow+sZllState.u16FreeAddrHigh-1) >> 1);
        sNwkStartReqPayload.u16FreeNwkAddrEnd = sZllState.u16FreeAddrHigh;
        sZllState.u16FreeAddrHigh = sNwkStartReqPayload.u16FreeNwkAddrBegin - 1;

        sNwkStartReqPayload.u16FreeGroupIdBegin = ((sZllState.u16FreeGroupLow+sZllState.u16FreeGroupHigh-1) >> 1);
        sNwkStartReqPayload.u16FreeGroupIdEnd = sZllState.u16FreeGroupHigh;
        sZllState.u16FreeGroupHigh = sNwkStartReqPayload.u16FreeGroupIdBegin - 1;
    }
    if (psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount) {
        /* allocate count group ids */
        sNwkStartReqPayload.u16GroupIdBegin = sZllState.u16FreeGroupLow;
        sNwkStartReqPayload.u16GroupIdEnd = sZllState.u16FreeGroupLow + psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount-1;
        sZllState.u16FreeGroupLow += psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount;
    }

    sNwkStartReqPayload.u64InitiatorIEEEAddr = sDeviceTable.asDeviceRecords[0].u64IEEEAddr;

    sDstAddr.eMode = 3;
    sDstAddr.u16PanId = 0xffff;
    sDstAddr.uAddress.u64Addr = psScanTarget->sScanDetails.sSrcAddr.uAddress.u64Addr;

    eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, psScanTarget->sScanDetails.sScanRspPayload.u8LogicalChannel);

    //DBG_vPrintf(TRACE_COMMISSION, "Ch %d", psScanTarget->sScanDetails.sScanRspPayload.u8LogicalChannel);
    eCLD_ZllCommissionCommandNetworkStartReqCommandSend( &sDstAddr,
                                                         &u8Seq,
                                                         &sNwkStartReqPayload  );
}

/****************************************************************************
 *
 * NAME: vSendRouterJoinRequest
 *
 * DESCRIPTION: sends router join request to touch link target
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendRouterJoinRequest(void *pvNwk,
                                    ZPS_tsNwkNib *psNib,
                                    tsZllScanTarget *psScanTarget,
                                    uint32 u32TransactionId)
{
    ZPS_tsInterPanAddress       sDstAddr;
    tsCLD_ZllCommission_NetworkJoinRouterReqCommandPayload      sNwkJoinRouterReqPayload;
    uint16 u16KeyMask;
    uint8 u8Seq;


    memset(&sNwkJoinRouterReqPayload, 0, sizeof(tsCLD_ZllCommission_NetworkJoinRouterReqCommandPayload));
    sNwkJoinRouterReqPayload.u32TransactionId = u32TransactionId;
    // ext pan 0, target decides
    u16KeyMask = ZLL_SUPPORTED_KEYS & psScanTarget->sScanDetails.sScanRspPayload.u16KeyMask;

    if (u16KeyMask & ZLL_MASTER_KEY_MASK) {
        sNwkJoinRouterReqPayload.u8KeyIndex = ZLL_MASTER_KEY_INDEX;
    } else if (u16KeyMask & ZLL_CERTIFICATION_KEY_MASK) {
        sNwkJoinRouterReqPayload.u8KeyIndex = ZLL_CERTIFICATION_KEY_INDEX;
    } else {
        sNwkJoinRouterReqPayload.u8KeyIndex = ZLL_TEST_KEY_INDEX;
    }

    eEncryptKey(psNib->sTbl.psSecMatSet[0].au8Key,
            sNwkJoinRouterReqPayload.au8NwkKey,
            u32TransactionId,
            psScanTarget->sScanDetails.sScanRspPayload.u32ResponseId,
            sNwkJoinRouterReqPayload.u8KeyIndex);



    sNwkJoinRouterReqPayload.u64ExtPanId = ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid;

    DBG_vPrintf(TRACE_COMMISSION, "RJoin nwk epid %016llx\n", ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid);
    DBG_vPrintf(TRACE_COMMISSION, "RJoin nwk epid %016llx\n",ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid);

    sNwkJoinRouterReqPayload.u8NwkUpdateId = psNib->sPersist.u8UpdateId;
    sNwkJoinRouterReqPayload.u8LogicalChannel = sZllState.u8MyChannel;
    sNwkJoinRouterReqPayload.u16PanId = psNib->sPersist.u16VsPanId;

    /* temp save addreess and group ranges */
    vSaveTempAddresses();

    if (sZllState.u16FreeAddrLow == 0) {
        sTarget.u16NwkAddr = RND_u32GetRand(1, 0xfff6);
        sNwkJoinRouterReqPayload.u16NwkAddr = sTarget.u16NwkAddr;
    } else  {
        sTarget.u16NwkAddr = sZllState.u16FreeAddrLow;
        sNwkJoinRouterReqPayload.u16NwkAddr = sZllState.u16FreeAddrLow++;
        if (psScanTarget->sScanDetails.sScanRspPayload.u8ZllInfo & ZLL_ADDR_ASSIGN) {
            DBG_vPrintf(TRACE_JOIN, "Assign addresses\n");

            sNwkJoinRouterReqPayload.u16FreeNwkAddrBegin = ((sZllState.u16FreeAddrLow+sZllState.u16FreeAddrHigh-1) >> 1);
            sNwkJoinRouterReqPayload.u16FreeNwkAddrEnd = sZllState.u16FreeAddrHigh;
            sZllState.u16FreeAddrHigh = sNwkJoinRouterReqPayload.u16FreeNwkAddrBegin - 1;

            sNwkJoinRouterReqPayload.u16FreeGroupIdBegin = ((sZllState.u16FreeGroupLow+sZllState.u16FreeGroupHigh-1) >> 1);
            sNwkJoinRouterReqPayload.u16FreeGroupIdEnd = sZllState.u16FreeGroupHigh;
            sZllState.u16FreeGroupHigh = sNwkJoinRouterReqPayload.u16FreeGroupIdBegin - 1;
        }
    }
    if (psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount) {
        /* allocate count group ids */
        sNwkJoinRouterReqPayload.u16GroupIdBegin = sZllState.u16FreeGroupLow;
        sNwkJoinRouterReqPayload.u16GroupIdEnd = sZllState.u16FreeGroupLow + psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount-1;
        sZllState.u16FreeGroupLow += psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount;
    }
    DBG_vPrintf(TRACE_JOIN, "Give it addr %04x\n", sNwkJoinRouterReqPayload.u16NwkAddr);
    /* rest of params zero */
    eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, psScanTarget->sScanDetails.sScanRspPayload.u8LogicalChannel);

    sDstAddr.eMode = 3;
    sDstAddr.u16PanId = 0xffff;
    sDstAddr.uAddress.u64Addr = psScanTarget->sScanDetails.sSrcAddr.uAddress.u64Addr;

    DBG_vPrintf(TRACE_COMMISSION, "Send rtr join on Ch %d for ch %d",psScanTarget->sScanDetails.sScanRspPayload.u8LogicalChannel, sZllState.u8MyChannel);
    eCLD_ZllCommissionCommandNetworkJoinRouterReqCommandSend( &sDstAddr,
                                                             &u8Seq,
                                                             &sNwkJoinRouterReqPayload  );
}

/****************************************************************************
 *
 * NAME: vSendEndDeviceJoinRequest
 *
 * DESCRIPTION: sends end device join request to touch link target
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendEndDeviceJoinRequest(void *pvNwk,
                                       ZPS_tsNwkNib *psNib,
                                       tsZllScanTarget *psScanTarget,
                                       uint32 u32TransactionId)
{
    ZPS_tsInterPanAddress       sDstAddr;
    tsCLD_ZllCommission_NetworkJoinEndDeviceReqCommandPayload      sNwkJoinEndDeviceReqPayload;
    uint16 u16KeyMask;
    uint8 u8Seq;

    memset(&sNwkJoinEndDeviceReqPayload, 0, sizeof(tsCLD_ZllCommission_NetworkJoinEndDeviceReqCommandPayload));
    sNwkJoinEndDeviceReqPayload.u32TransactionId = sCommission.u32TransactionId;

    u16KeyMask = ZLL_SUPPORTED_KEYS & psScanTarget->sScanDetails.sScanRspPayload.u16KeyMask;
    if (u16KeyMask & ZLL_MASTER_KEY_MASK) {
        sNwkJoinEndDeviceReqPayload.u8KeyIndex = ZLL_MASTER_KEY_INDEX;
    } else if (u16KeyMask & ZLL_CERTIFICATION_KEY_MASK) {
        sNwkJoinEndDeviceReqPayload.u8KeyIndex = ZLL_CERTIFICATION_KEY_INDEX;
    } else {
        sNwkJoinEndDeviceReqPayload.u8KeyIndex = ZLL_TEST_KEY_INDEX;
    }


    eEncryptKey(psNib->sTbl.psSecMatSet[0].au8Key,
                sNwkJoinEndDeviceReqPayload.au8NwkKey,
                u32TransactionId,
                psScanTarget->sScanDetails.sScanRspPayload.u32ResponseId,
                sNwkJoinEndDeviceReqPayload.u8KeyIndex);



    sNwkJoinEndDeviceReqPayload.u64ExtPanId = ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid;
    sNwkJoinEndDeviceReqPayload.u8NwkUpdateId = psNib->sPersist.u8UpdateId;
    sNwkJoinEndDeviceReqPayload.u8LogicalChannel = sZllState.u8MyChannel;
    sNwkJoinEndDeviceReqPayload.u16PanId = psNib->sPersist.u16VsPanId;

    /* temp save addreess and group ranges */
    vSaveTempAddresses();

    if (sZllState.u16FreeAddrLow == 0) {
        sTarget.u16NwkAddr = RND_u32GetRand(1, 0xfff6);;
        sNwkJoinEndDeviceReqPayload.u16NwkAddr = sTarget.u16NwkAddr;
    } else  {
        sTarget.u16NwkAddr = sZllState.u16FreeAddrLow;
        sNwkJoinEndDeviceReqPayload.u16NwkAddr = sZllState.u16FreeAddrLow++;
        if (psScanTarget->sScanDetails.sScanRspPayload.u8ZllInfo & ZLL_ADDR_ASSIGN) {
            DBG_vPrintf(TRACE_JOIN, "Assign addresses\n");

            sNwkJoinEndDeviceReqPayload.u16FreeNwkAddrBegin = ((sZllState.u16FreeAddrLow+sZllState.u16FreeAddrHigh-1) >> 1);
            sNwkJoinEndDeviceReqPayload.u16FreeNwkAddrEnd = sZllState.u16FreeAddrHigh;
            sZllState.u16FreeAddrHigh = sNwkJoinEndDeviceReqPayload.u16FreeNwkAddrBegin - 1;

            sNwkJoinEndDeviceReqPayload.u16FreeGroupIdBegin = ((sZllState.u16FreeGroupLow+sZllState.u16FreeGroupHigh-1) >> 1);
            sNwkJoinEndDeviceReqPayload.u16FreeGroupIdEnd = sZllState.u16FreeGroupHigh;
            sZllState.u16FreeGroupHigh = sNwkJoinEndDeviceReqPayload.u16FreeGroupIdBegin - 1;
        }
    }
    DBG_vPrintf(TRACE_JOIN, "Give it addr %04x\n", sNwkJoinEndDeviceReqPayload.u16NwkAddr);
    if (psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount) {
        /* allocate count group ids */
        sNwkJoinEndDeviceReqPayload.u16GroupIdBegin = sZllState.u16FreeGroupLow;
        sNwkJoinEndDeviceReqPayload.u16GroupIdEnd = sZllState.u16FreeGroupLow + psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount-1;
        sZllState.u16FreeGroupLow += psScanTarget->sScanDetails.sScanRspPayload.u8GroupIdCount;
    }

    eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL,psScanTarget->sScanDetails.sScanRspPayload.u8LogicalChannel);

    sDstAddr.eMode = 3;
    sDstAddr.u16PanId = 0xffff;
    sDstAddr.uAddress.u64Addr = psScanTarget->sScanDetails.sSrcAddr.uAddress.u64Addr;

    DBG_vPrintf(TRACE_JOIN, "Send zed join on Ch %d for ch %d\n", psScanTarget->sScanDetails.sScanRspPayload.u8LogicalChannel,
                                                                    sZllState.u8MyChannel);
    eCLD_ZllCommissionCommandNetworkJoinEndDeviceReqCommandSend( &sDstAddr,
                                                                 &u8Seq,
                                                                 &sNwkJoinEndDeviceReqPayload  );
}


/****************************************************************************
 *
 * NAME: vHandleNwkUpdateRequest
 *
 * DESCRIPTION: handles network id update requests, updates nib
 * param if required
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleNwkUpdateRequest(void * pvNwk,
                                     ZPS_tsNwkNib *psNib,
                                     tsCLD_ZllCommission_NetworkUpdateReqCommandPayload *psNwkUpdateReqPayload)
{
    if ((psNwkUpdateReqPayload->u64ExtPanId == ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid) &&
         (psNwkUpdateReqPayload->u16PanId == psNib->sPersist.u16VsPanId) &&
          (psNib->sPersist.u16VsPanId != u8NewUpdateID(psNib->sPersist.u16VsPanId, psNwkUpdateReqPayload->u16PanId) ))
    {
        /* Update the UpdateId, Nwk addr and Channel */
        ZPS_vNwkNibSetUpdateId( pvNwk, psNwkUpdateReqPayload->u8NwkUpdateId);
        ZPS_vNwkNibSetNwkAddr( pvNwk,  psNwkUpdateReqPayload->u16NwkAddr);
        ZPS_vNwkNibSetChannel( pvNwk,  psNwkUpdateReqPayload->u8LogicalChannel);
    }
}

/****************************************************************************
 *
 * NAME: vHandleDeviceInfoRequest
 *
 * DESCRIPTION: handles touch link device info request, sends the reqponse
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleDeviceInfoRequest(APP_CommissionEvent *psEvent)
{
    tsCLD_ZllCommission_DeviceInfoRspCommandPayload sDeviceInfoRspPayload;
    ZPS_tsInterPanAddress       sDstAddr;
    int i, j;
    uint8 u8Seq;

    DBG_vPrintf(TRACE_JOIN, "Device Info request\n");
    memset(&sDeviceInfoRspPayload, 0, sizeof(tsCLD_ZllCommission_DeviceInfoRspCommandPayload));
    sDeviceInfoRspPayload.u32TransactionId = psEvent->sZllMessage.uPayload.sDeviceInfoReqPayload.u32TransactionId;
    sDeviceInfoRspPayload.u8NumberSubDevices = ZLL_NUMBER_DEVICES;
    sDeviceInfoRspPayload.u8StartIndex = psEvent->sZllMessage.uPayload.sDeviceInfoReqPayload.u8StartIndex;
    sDeviceInfoRspPayload.u8DeviceInfoRecordCount = 0;
    // copy from table sDeviceTable

    j = psEvent->sZllMessage.uPayload.sDeviceInfoReqPayload.u8StartIndex;
    for (i=0; i<ZLL_MAX_DEVICE_RECORDS && j<ZLL_NUMBER_DEVICES; i++, j++)
    {
        sDeviceInfoRspPayload.asDeviceRecords[i] = sDeviceTable.asDeviceRecords[j];
        sDeviceInfoRspPayload.u8DeviceInfoRecordCount++;
    }

    sDstAddr = psEvent->sZllMessage.sSrcAddr;
    sDstAddr.u16PanId = 0xffff;

    eCLD_ZllCommissionCommandDeviceInfoRspCommandSend( &sDstAddr,
                                                       &u8Seq,
                                                       &sDeviceInfoRspPayload);
}


/****************************************************************************
 *
 * NAME: vHandleEndDeviceJoinRequest
 *
 * DESCRIPTION: handles the response to a device info request
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleEndDeviceJoinRequest( void * pvNwk,
                                          ZPS_tsNwkNib *psNib,
                                          APP_CommissionEvent *psEvent,
                                          tsCommissionData *psCommission)
{
    tsCLD_ZllCommission_NetworkJoinEndDeviceRspCommandPayload   sNwkJoinEndDeviceRspPayload;
    ZPS_tsInterPanAddress       sDstAddr;
    uint8 u8Seq;

    sDstAddr = psEvent->sZllMessage.sSrcAddr;
    sDstAddr.u16PanId = 0xffff;


    DBG_vPrintf(TRACE_JOIN, "Join End Device Req\n");
    sNwkJoinEndDeviceRspPayload.u32TransactionId = psCommission->u32TransactionId;
    // return error if NFN (not supported), else success
    sNwkJoinEndDeviceRspPayload.u8Status =
            (sZllState.eState == NOT_FACTORY_NEW_REMOTE)? ZLL_ERROR: ZLL_SUCCESS;
    eCLD_ZllCommissionCommandNetworkJoinEndDeviceRspCommandSend( &sDstAddr,
                                                                 &u8Seq,
                                                                 &sNwkJoinEndDeviceRspPayload);

    if (sZllState.eState == FACTORY_NEW_REMOTE)
    {
        /* save params in case leave required */
        sStartParams = psEvent->sZllMessage.uPayload.sNwkJoinEndDeviceReqPayload;

        /* security stuff */

       eDecryptKey(sStartParams.au8NwkKey,
                   psNib->sTbl.psSecMatSet[0].au8Key,
                   psCommission->u32TransactionId,
                   psCommission->u32ResponseId,
                   sStartParams.u8KeyIndex);

       psNib->sTbl.psSecMatSet[0].u8KeySeqNum = 0;
       memset(psNib->sTbl.pu32InFCSet, 0,
               (sizeof(uint32) * psNib->sTblSize.u16NtActv));
       psNib->sTbl.psSecMatSet[0].u8KeyType = ZPS_NWK_SEC_NETWORK_KEY;

       /* save security material to flash */
       ZPS_vNwkSaveSecMat( pvNwk);

       /* Make this the Active Key */
       ZPS_vNwkNibSetKeySeqNum( pvNwk, 0);

        /* Set params */
        ZPS_vNwkNibSetChannel( pvNwk, sStartParams.u8LogicalChannel);
       sZllState.u8MyChannel = sStartParams.u8LogicalChannel;
       ZPS_vNwkNibSetPanId( pvNwk, sStartParams.u16PanId);
       ZPS_eAplAibSetApsUseExtendedPanId(sStartParams.u64ExtPanId);
       sZllState.u16MyAddr = sStartParams.u16NwkAddr;
       ZPS_vNwkNibSetNwkAddr( pvNwk, sZllState.u16MyAddr);

       sZllState.u16FreeAddrLow = sStartParams.u16FreeNwkAddrBegin;
       sZllState.u16FreeAddrHigh = sStartParams.u16FreeNwkAddrEnd;
       sZllState.u16FreeGroupLow = sStartParams.u16FreeGroupIdBegin;
       sZllState.u16FreeGroupHigh = sStartParams.u16FreeGroupIdEnd;
       // take our group addr requirement
       vSetGroupAddress( sStartParams.u16GroupIdBegin, GROUPS_REQUIRED);

       ZPS_vNwkNibSetUpdateId( pvNwk, sStartParams.u8NwkUpdateId);

       DBG_vPrintf(TRACE_JOIN, "Start with pan %016llx\n", sStartParams.u64ExtPanId);

       OS_eStopSWTimer(APP_CommissionTimer);
       OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_MS(10), NULL);
       bDoRejoin = TRUE;
       bWithDiscovery = TRUE;
       psCommission->eState = E_WAIT_START_UP;
    }
}

/****************************************************************************
 *
 * NAME: vHandleNwkStartResponse
 *
 * DESCRIPTION: handles the start response, either sets up the network params
 *  ready to join, or recovers assigned addresses in a fail case
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleNwkStartResponse(void * pvNwk,
                                     APP_CommissionEvent *psEvent,
                                     tsCommissionData *psCommission)
{
    if (psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u8Status == ZLL_SUCCESS)
    {
        ZPS_vNwkNibSetChannel( pvNwk, psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u8LogicalChannel);
        sZllState.u8MyChannel = psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u8LogicalChannel;
        /* Set channel maskl to primary channels */
        ZPS_vNwkNibSetExtPanId(pvNwk, psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u64ExtPanId);
        ZPS_eAplAibSetApsUseExtendedPanId(psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u64ExtPanId);
        ZPS_vNwkNibSetNwkAddr( pvNwk, sZllState.u16MyAddr);
        ZPS_vNwkNibSetUpdateId(pvNwk, psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u8NwkUpdateId);
        ZPS_vNwkNibSetPanId(pvNwk, psEvent->sZllMessage.uPayload.sNwkStartRspPayload.u16PanId);
        OS_eStopSWTimer(APP_CommissionTimer);
        OS_eStartSWTimer(APP_CommissionTimer, APP_TIME_SEC(ZLL_START_UP_TIME_SEC), NULL);
        DBG_vPrintf(TRACE_JOIN, "Start on Ch %d\n", sZllState.u8MyChannel);
        bDoRejoin = TRUE;
        bWithDiscovery = FALSE;
        psCommission->eState = E_WAIT_START_UP;
    } else {
        /* recover assigned addresses */
        /* temp save addreess and group ranges */
        vRecoverTempAddresses();
    }
}

/****************************************************************************
 *
 * NAME: vHandleNwkStartRequest
 *
 * DESCRIPTION: handles a network start request
 * This is an end device this should never happen so send error response
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleNwkStartRequest( ZPS_tsInterPanAddress  *psDstAddr,
                                     uint32 u32TransactionId)
{
    tsCLD_ZllCommission_NetworkStartRspCommandPayload           sNwkStartRspPayload;
    uint8 u8Seq;

    sNwkStartRspPayload.u32TransactionId = u32TransactionId;
    sNwkStartRspPayload.u8Status = ZLL_ERROR;

    eCLD_ZllCommissionCommandNetworkStartRspCommandSend( psDstAddr,
                                                         &u8Seq,
                                                         &sNwkStartRspPayload);
}

/****************************************************************************
 *
 * NAME: vHandleRouterJoinRequest
 *
 * DESCRIPTION: handles the response to router join request
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleRouterJoinRequest( ZPS_tsInterPanAddress  *psDstAddr,
                                       uint32                 u32TransactionId)
{
    tsCLD_ZllCommission_NetworkJoinRouterRspCommandPayload      sNwkJoinRouterRspPayload;
    uint8  u8Seq;

    sNwkJoinRouterRspPayload.u32TransactionId = u32TransactionId;
    sNwkJoinRouterRspPayload.u8Status = ZLL_ERROR;

    eCLD_ZllCommissionCommandNetworkJoinRouterRspCommandSend( psDstAddr,
                                                              &u8Seq,
                                                              &sNwkJoinRouterRspPayload);
}

/****************************************************************************
 *
 * NAME: vSendIdentifyRequest
 *
 * DESCRIPTION: sends an identify request to a touch link target
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendIdentifyRequest(uint64 u64Addr, uint32 u32TransactionId, uint8 u8Time)
{
    ZPS_tsInterPanAddress  sDstAddr;
    tsCLD_ZllCommission_IdentifyReqCommandPayload               sIdentifyReqPayload;
    uint8 u8Seq;

    sDstAddr.eMode = ZPS_E_AM_INTERPAN_IEEE;
    sDstAddr.u16PanId = 0xffff;
    sDstAddr.uAddress.u64Addr = u64Addr;
    sIdentifyReqPayload.u32TransactionId = sCommission.u32TransactionId;
    sIdentifyReqPayload.u16Duration = u8Time;

    eCLD_ZllCommissionCommandDeviceIndentifyReqCommandSend(
                                        &sDstAddr,
                                        &u8Seq,
                                        &sIdentifyReqPayload);
}

/****************************************************************************
 *
 * NAME: APP_CommissionTimerTask
 *
 * DESCRIPTION: handles commissioning timer expiry events
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_CommissionTimerTask)
{
    APP_CommissionEvent sEvent;
    sEvent.eType = APP_E_COMMISSION_TIMER_EXPIRED;
    OS_ePostMessage(APP_CommissionEvents, &sEvent);
}

/****************************************************************************
 *
 * NAME: eEncryptKey
 *
 * DESCRIPTION: encrypts the network key prior to transmission
 * over inter pan
 * Encrypt the nwk key use AES ECB
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE uint8 eEncryptKey(uint8* au8InData,
                    uint8* au8OutData,
                    uint32 u32TransId,
                    uint32 u32ResponseId,
                    uint8 u8KeyIndex)
{


    tsReg128 sExpanded;
    tsReg128 sTransportKey;

    tsReg128 sDataIn,sDataOut;

    sExpanded.u32register0 = u32TransId;
    sExpanded.u32register1 = u32TransId;
    sExpanded.u32register2 = u32ResponseId;
    sExpanded.u32register3 = u32ResponseId;

    switch (u8KeyIndex)
    {
        case ZLL_TEST_KEY_INDEX:
            sTransportKey.u32register0 = 0x50684c69;
            sTransportKey.u32register1 = u32TransId;
            sTransportKey.u32register2 = 0x434c534e;
            sTransportKey.u32register3 = u32ResponseId;
            break;
        case ZLL_MASTER_KEY_INDEX:
            bACI_ECBencodeStripe( &sMasterKey,
                                  TRUE,
                                  &sExpanded,
                                  &sTransportKey);
            break;
        case ZLL_CERTIFICATION_KEY_INDEX:
            bACI_ECBencodeStripe( &sCertKey,
                                  TRUE,
                                  &sExpanded,
                                  &sTransportKey);
            break;

        default:
            return 3;
            break;
    }

    memcpy(&sDataIn,au8InData,0x10);
    memcpy(&sDataOut,au8OutData,0x10);
    bACI_ECBencodeStripe(&sTransportKey,
                         TRUE,
                         &sDataIn,
                         &sDataOut);
    memcpy(au8OutData,&sDataOut,0x10);

    return 0;
}

/****************************************************************************
 *
 * NAME: eDecryptKey
 *
 * DESCRIPTION:
 * Decrypt the nwk key use AES ECB
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE uint8 eDecryptKey(uint8* au8InData,
                    uint8* au8OutData,
                    uint32 u32TransId,
                    uint32 u32ResponseId,
                    uint8 u8KeyIndex)
{
    tsReg128 sTransportKey;
    tsReg128 sExpanded;

    sExpanded.u32register0 = u32TransId;
    sExpanded.u32register1 = u32TransId;
    sExpanded.u32register2 = u32ResponseId;
    sExpanded.u32register3 = u32ResponseId;

    switch (u8KeyIndex)
    {
        case ZLL_TEST_KEY_INDEX:
            sTransportKey.u32register0 = 0x50684c69;
            sTransportKey.u32register1 = u32TransId;
            sTransportKey.u32register2 = 0x434c534e;
            sTransportKey.u32register3 = u32ResponseId;
            break;
        case ZLL_MASTER_KEY_INDEX:
            bACI_ECBencodeStripe( &sMasterKey,
                                  TRUE,
                                  &sExpanded,
                                  &sTransportKey);
            break;
        case ZLL_CERTIFICATION_KEY_INDEX:
            bACI_ECBencodeStripe( &sCertKey,
                                  TRUE,
                                  &sExpanded,
                                  &sTransportKey);
            break;

        default:
            DBG_vPrintf(TRACE_COMMISSION, "***Ooops***\n");
            return 3;
            break;
    }

    vECB_Decrypt( (uint8*)&sTransportKey,
                  au8InData,
                  au8OutData);
#if SHOW_KEY
    int i;
    for (i=0; i<16; i++) {
        DBG_vPrintf(TRACE_COMMISSION, "%02x ", au8OutData[i]);
    }
    DBG_vPrintf(TRACE_COMMISSION, "\n");
#endif

    return 0;
}

/****************************************************************************
 *
 * NAME: u8NewUpdateID
 *
 * DESCRIPTION: determines which of 2 network update ids is
 * the freshest
 *
 *
 * RETURNS: the frestest nwk update id
 *
 *
 ****************************************************************************/
PRIVATE uint8 u8NewUpdateID(uint8 u8ID1, uint8 u8ID2 )
{
    if ( (abs(u8ID1-u8ID2)) > 200) {
        return MIN(u8ID1, u8ID2);
    }
    return MAX(u8ID1, u8ID2);
}


/****************************************************************************
 *
 * NAME: vSendFactoryResetRequest
 *
 * DESCRIPTION: send a factory reset touch link command to a target
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendFactoryResetRequest(void *pvNwk,
                                      ZPS_tsNwkNib *psNib,
                                      tsZllScanTarget *psScanTarget,
                                      uint32 u32TransactionId)
{
    ZPS_tsInterPanAddress       sDstAddr;
    tsCLD_ZllCommission_FactoryResetReqCommandPayload sFacRstPayload;
    uint8 u8Seq;
    memset(&sFacRstPayload, 0, sizeof(tsCLD_ZllCommission_FactoryResetReqCommandPayload));
    sDstAddr.eMode = 3;
    sDstAddr.u16PanId = 0xffff;
    sDstAddr.uAddress.u64Addr = psScanTarget->sScanDetails.sSrcAddr.uAddress.u64Addr;

    eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, psScanTarget->sScanDetails.sScanRspPayload.u8LogicalChannel);
    sFacRstPayload.u32TransactionId= psScanTarget->sScanDetails.sScanRspPayload.u32TransactionId;
    eCLD_ZllCommissionCommandFactoryResetReqCommandSend( &sDstAddr,
                                                         &u8Seq,
                                                         &sFacRstPayload  );

    vRemoveLight(psScanTarget->sScanDetails.sScanRspPayload.u16NwkAddr);
}

/****************************************************************************
 *
 * NAME: vSaveTempAddresses
 *
 * DESCRIPTION: temp save the address and group ranges in case the join or network start request fails
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSaveTempAddresses( void) {
    /* save address ranges, in case needed to recover */
    u16TempAddrLow = sZllState.u16FreeAddrLow;
     u16TempAddrHigh = sZllState.u16FreeAddrHigh;
     u16TempGroupLow = sZllState.u16FreeGroupLow;
     u16TempGroupHigh = sZllState.u16FreeGroupHigh;
}

/****************************************************************************
 *
 * NAME: vRecoverTempAddresses
 *
 * DESCRIPTION: restore the address and group ranges after a join or network start failed
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vRecoverTempAddresses( void) {
    /* recover assigned addresses */
    sZllState.u16FreeAddrLow = u16TempAddrLow;
    sZllState.u16FreeAddrHigh = u16TempAddrHigh;
    sZllState.u16FreeGroupLow = u16TempGroupLow;
    sZllState.u16FreeGroupHigh = u16TempGroupHigh;
}

/****************************************************************************
 *
 * NAME: vSetUpParent
 *
 * DESCRIPTION: sets up the parent in the neighbor table before join
 * with out discovery
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSetUpParent( ZPS_tsNwkNib *psNib, uint16 u16Addr)
{
    void *pvNwk = ZPS_pvAplZdoGetNwkHandle();
    psNib->sTbl.psNtActv[ZPS_NWK_NT_ACTV_PARENT].u16NwkAddr = u16Addr;
    psNib->sTbl.pu32InFCSet[0] = 0;
    ZPS_vNtSetUsedStatus(pvNwk, &psNib->sTbl.psNtActv[ZPS_NWK_NT_ACTV_PARENT], TRUE);
    psNib->sTbl.psNtActv[ZPS_NWK_NT_ACTV_PARENT].uAncAttrs.bfBitfields.u1DeviceType = TRUE; /* ZC/ZR */
    psNib->sTbl.psNtActv[ZPS_NWK_NT_ACTV_PARENT].uAncAttrs.bfBitfields.u1PowerSource = TRUE; /* Mains */
    psNib->sTbl.psNtActv[ZPS_NWK_NT_ACTV_PARENT].uAncAttrs.bfBitfields.u1RxOnWhenIdle = TRUE;
    psNib->sTbl.psNtActv[ZPS_NWK_NT_ACTV_PARENT].uAncAttrs.bfBitfields.u1Authenticated = TRUE;
    psNib->sTbl.psNtActv[ZPS_NWK_NT_ACTV_PARENT].uAncAttrs.bfBitfields.u2Relationship = ZPS_NWK_NT_AP_RELATIONSHIP_PARENT;
    psNib->sTbl.psNtActv[ZPS_NWK_NT_ACTV_PARENT].uAncAttrs.bfBitfields.u3OutgoingCost = 1;
    psNib->sTbl.psNtActv[ZPS_NWK_NT_ACTV_PARENT].u8Age = 0;
}



/****************************************************************************
 *
 * NAME: bValidateTLAddress
 *
 * DESCRIPTION:
 * Validates that the address mode and address used to send the TL command
 * is allowed for the specific command
 *
 * RETURNS: uint8, the newest update id
 * void
 *
 ****************************************************************************/
PUBLIC bool_t bValidateTLAddress(ZPS_tsInterPanAddress * psDstAddr, uint8 u8Cmd)
{
#if TL_VALIDATE == TRUE
    DBG_vPrintf(TL_VALIDATE, "\nRcvd on Pan %04x eMode %d Addr ",
                        psDstAddr->u16PanId,
                        psDstAddr->eMode);
    if (psDstAddr->eMode < ZPS_E_AM_INTERPAN_IEEE)
    {
        DBG_vPrintf(TL_VALIDATE, "%04x\n", psDstAddr->uAddress.u16Addr);
    }
    else
    {
        DBG_vPrintf(TL_VALIDATE, "%016llx\n", psDstAddr->uAddress.u64Addr);
    }
    DBG_vPrintf(TL_VALIDATE, "IP CMD %d\n", u8Cmd);
#endif
    switch (u8Cmd)
    {
        case E_CLD_COMMISSION_CMD_SCAN_REQ:
        case E_CLD_COMMISSION_CMD_SCAN_RSP:
        case E_CLD_COMMISSION_CMD_DEVICE_INFO_REQ:
        case E_CLD_COMMISSION_CMD_IDENTIFY_REQ:
            /* allowed to receive on broadcast */
            break;
        case E_CLD_COMMISSION_CMD_DEVICE_INFO_RSP:
        case E_CLD_COMMISSION_CMD_FACTORY_RESET_REQ:
        case E_CLD_COMMISSION_CMD_NETWORK_START_REQ:
        case E_CLD_COMMISSION_CMD_NETWORK_START_RSP:
        case E_CLD_COMMISSION_CMD_NETWORK_JOIN_ROUTER_REQ:
        case E_CLD_COMMISSION_CMD_NETWORK_JOIN_ROUTER_RSP:
        case E_CLD_COMMISSION_CMD_NETWORK_JOIN_END_DEVICE_REQ:
        case E_CLD_COMMISSION_CMD_NETWORK_JOIN_END_DEVICE_RSP:
        case E_CLD_COMMISSION_CMD_NETWORK_UPDATE_REQ:
            if (  (( psDstAddr->eMode == ZPS_E_AM_INTERPAN_SHORT ) && ( psDstAddr->uAddress.u16Addr == 0xFFFF ) )
                    || (psDstAddr->eMode == ZPS_E_AM_INTERPAN_GROUP)
               )
            {
                DBG_vPrintf(TL_VALIDATE, "Drop IP on broadcast\n");
                return FALSE;
            }
            break;

    default:
        // unknown command id, dro it
        return FALSE;
    }; // end of sweitch

    return TRUE;
}
