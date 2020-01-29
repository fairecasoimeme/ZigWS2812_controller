/* MODULE:             JN-AN-1171
 *
 * COMPONENT:          zll_remote_node.c
 *
 * DESCRIPTION:        ZLL Demo: Remote Control Functionality -Implementation
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
#include <string.h>
#include <appapi.h>
#include "os.h"
#include "os_gen.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "dbg.h"
#include "dbg_uart.h"
#include "pwrm.h"
#include "zps_gen.h"
#include "zps_apl_af.h"
#include "zps_apl_zdo.h"
#include "zps_apl_aib.h"
#include "zps_apl_zdp.h"
#include "rnd_pub.h"

#include "app_common.h"
#include "groups.h"

#include "PDM_IDs.h"

#include "app_timer_driver.h"
#include "app_captouch_buttons.h"
#include "app_led_control.h"
#include "zll_remote_node.h"

#include "eventStrings.h"
#include "app_zcl_remote_task.h"
#include "zll_remote_node.h"
#include "app_events.h"
#include "zcl_customcommand.h"

#include "appZpsBeaconHandler.h"

#ifdef DUT_CONTROLLER
#include "DUT_Controller.h"
#endif
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifdef DEBUG_APP
    #define TRACE_APP   TRUE
#else
    #define TRACE_APP   FALSE
#endif

#ifdef DEBUG_REMOTE_NODE
    #define TRACE_REMOTE_NODE   TRUE
#else
    #define TRACE_REMOTE_NODE   FALSE
#endif

#ifndef DEBUG_CLASSIC_JOIN
#define TRACE_CLASSIC FALSE
#else
#define TRACE_CLASSIC TRUE
#endif

#ifdef DEBUG_LIGHT_AGE
#define TRACE_LIGHT_AGE   TRUE
#else
#define TRACE_LIGHT_AGE   FALSE
#endif

#ifndef DEBUG_SLEEP
#define TRACE_SLEEP FALSE
#else
#define TRACE_SLEEP TRUE
#endif

#ifndef DEBUG_REJOIN
#define TRACE_REJOIN FALSE
#else
#define TRACE_REJOIN TRUE
#endif

#define KEEP_ALIVE_FACTORY_NEW  (45)
#define KEEP_ALIVETIME (20)
#define DEEP_SLEEPTIME (20)

#define SLEEP_DURATION_MS (800)
#define SLEEP_TIMER_TICKS_PER_MS (32)

#define LEVEL_CHANGE_STEPS_PER_SEC_SLOW        (20)
#define LEVEL_CHANGE_STEPS_PER_SEC_MED        (60)
#define LEVEL_CHANGE_STEPS_PER_SEC_FAST     (100)
#define SAT_CHANGE_STEPS_PER_SEC     (20)
#define COL_TEMP_CHANGE_STEPS_PER_SEC     (100)
#define COL_TEMP_PHY_MIN                (0U)       /* Seting these two paramters to zero means the colour temperature    */
#define COL_TEMP_PHY_MAX                (0U)       /* is limited by the bulb capabilities, not what the remote sends out */
#define STOP_COLOUR_LOOP                 FALSE

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

PUBLIC void vDisplayNwkKey(void);
PUBLIC void vU16ToString(char * str, uint16 u16Num);


#ifdef TRACE_UNUSED
PRIVATE void vAppOffEffect(uint8 u8Effect, uint8 u8Varient);
#endif

PRIVATE void APP_vHandleKeyPress(teUserKeyCodes eKeyCode);
PRIVATE void vSetAddress(tsZCL_Address * psAddress, bool_t bBroadcast);
PRIVATE void vAppSetGroupId(uint8 u8GroupId);
PUBLIC void vStartPolling(void);
PRIVATE void APP_vHandleKeyRelease(teUserKeyCodes eKeyCode);
PRIVATE void vWakeCallBack(void);
PUBLIC void vStopAllTimers(void);

PRIVATE void vSendPermitJoin(void);
PUBLIC void vAppMoveToColourTemperature( uint16 u16ColourTemperatureMired, uint16 u16StepsPerSec);
PUBLIC void vFactoryResetRecords( void);

PRIVATE void vDiscoverNetworks(void);
PRIVATE void vTryNwkJoin(void);
PRIVATE void vPickChannel( void *pvNwk);

PUBLIC void vSetAssociationFilter( void);

PRIVATE void vHandleWaitStartAppEvent(APP_tsEvent* psAppEvent);
PRIVATE void vHandleRunningAppEvent(APP_tsEvent* psAppEvent);
PRIVATE void vHandleRunningStackEvent(ZPS_tsAfEvent* psStackEvent);
PRIVATE void vHandleWaitStartStackEvent(ZPS_tsAfEvent* psStackEvent);
PRIVATE void vHandleClassicDiscoveryAndJoin(ZPS_tsAfEvent* psStackEvent);
PRIVATE void vAppHandleTouchLink(APP_tsEvent* psAppEvent);
PRIVATE void vAppHandleEndpointInfo(APP_tsEvent* psAppEvent);
PRIVATE void vAppHandleEndpointList(APP_tsEvent* psAppEvent);
PRIVATE void vAppHandleGroupList(APP_tsEvent* psAppEvent);
PRIVATE void vResetSleepAndJoinCounters(void);

PRIVATE uint8 u8SearchEndpointTable(APP_tsEventTouchLink *psEndpointData,
        uint8* pu8Index);



/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PRIVATE char *apcStateNames[] = { "E_STARTUP", "E_NFN_START", "E_WAIT_START",
                                  "E_NETWORK_DISCOVER", "E_RUNNING" };


tsZllEndpointInfoTable sEndpointTable;
tsZllGroupInfoTable sGroupTable;



extern bool_t bDeepSleep;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

PRIVATE uint8 u8GroupId=0;
PRIVATE bool bAddModeBroadcast=FALSE;

teShiftLevel eShiftLevel = E_SHIFT_0;

#if (defined DUT_CONTROLLER)
    PRIVATE bool_t bAddrMode = FALSE;
#else
    PRIVATE bool_t bAddrMode = TRUE;
#endif

PRIVATE uint16 u16FastPoll;
PRIVATE uint8 u8KeepAliveTime = KEEP_ALIVETIME;
PRIVATE uint8 u8DeepSleepTime = DEEP_SLEEPTIME;
bool_t bFailedToJoin = FALSE;
PRIVATE uint8 u8RejoinAttemptsRemaining = ZLL_MAX_REJOIN_ATTEMPTS;

PRIVATE    pwrm_tsWakeTimerEvent    sWake;

const uint8 u8DiscChannels[] = { 11, 15, 20, 25, 12, 13, 14, 16, 17, 18, 19, 21, 22, 23, 24, 26 };

uint8 u8ChanIdx;
uint8 u8NwkIdx;
uint8 u8NwkCount;
ZPS_tsNwkNetworkDescr *psNwkDescTbl;

uint32 u32OldFrameCtr;


extern bool_t bDeepSleep;
tsBeaconFilterType sBeaconFilter;
uint64 au64ExtPanList[1];

uint8 u8AddGroupToIndex;
bool_t bAddGroupToAll;
uint16 u16IdTime = 0;

extern bool_t bSendFactoryResetOverAir;

static const uint8 s_au8LnkKeyArray[16] = {0x5a, 0x69, 0x67, 0x42, 0x65, 0x65, 0x41, 0x6c,
                                     0x6c, 0x69, 0x61, 0x6e, 0x63, 0x65, 0x30, 0x39};
// ZLL Commissioning trust centre link key
static const uint8 s_au8ZllLnkKeyArray[16] = {0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
                                     0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf};


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
extern void zps_vNwkSecClearMatSet(void *psNwk);


/****************************************************************************
 *
 * NAME: vDiscoverNetworks
 *
 * DESCRIPTION:
 * Initialites discovery on each channel in turn, till sucessful join or all have been tried
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vDiscoverNetworks(void) {
uint8 u8Status;
void* pvNwk;

    pvNwk = ZPS_pvAplZdoGetNwkHandle();
    while (u8ChanIdx < sizeof(u8DiscChannels))
    {
        DBG_vPrintf(TRACE_CLASSIC, "\nDiscover on ch %d\n", u8DiscChannels[u8ChanIdx] );
        vSetAssociationFilter();
        ZPS_vNwkNibClearDiscoveryNT( pvNwk);
        u8Status = ZPS_eAplZdoDiscoverNetworks( 1 << u8DiscChannels[u8ChanIdx]);
        DBG_vPrintf(TRACE_CLASSIC, "disc status %02x\n", u8Status );
        u8ChanIdx++;
        if (u8Status == 0) {
            return;
        }
    }
    if (u8ChanIdx == sizeof(u8DiscChannels)) {
        DBG_vPrintf(TRACE_CLASSIC, "Discover complete\n");
        vPickChannel( pvNwk);
    }
}

/****************************************************************************
 *
 * NAME: vTryNwkJoin
 *
 * DESCRIPTION:
 * Attempts to join each of the discovered networks, if unsuccessful initiate discovery on next channel
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vTryNwkJoin(void) {
uint8 u8Status;

    while (u8NwkIdx < u8NwkCount) {
        if (psNwkDescTbl[u8NwkIdx].u8PermitJoining  ) {
            DBG_vPrintf(TRACE_CLASSIC, "Try To join %016llx on Ch %d\n", psNwkDescTbl[u8NwkIdx].u64ExtPanId, psNwkDescTbl[u8NwkIdx].u8LogicalChan);


            u8Status = ZPS_eAplZdoJoinNetwork( &psNwkDescTbl[u8NwkIdx] );
            DBG_vPrintf(TRACE_CLASSIC, "Try join status %02x\n", u8Status);
            if (u8Status == 0) {
                u8NwkIdx++;
                return;
            }
#if TRACE_CLASSIC
            else if (u8Status == 0xc3) {
                ZPS_tsNwkNib * thisNib;
                thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());
                int i=0;
                while ( i<thisNib->sTblSize.u8NtDisc && thisNib->sTbl.psNtDisc[i].u64ExtPanId !=0 ) {
                    DBG_vPrintf(TRACE_CLASSIC, "DiscNT %d Pan %016llx Addr %04x Chan %d LQI %d Depth %d\n",
                            i,
                            thisNib->sTbl.psNtDisc[i].u64ExtPanId,
                            thisNib->sTbl.psNtDisc[i].u16NwkAddr,
                            thisNib->sTbl.psNtDisc[i].u8LogicalChan,
                            thisNib->sTbl.psNtDisc[i].u8LinkQuality,
                            thisNib->sTbl.psNtDisc[i].uAncAttrs.bfBitfields.u4Depth);
                    i++;
                }
            }
#endif
        }
        u8NwkIdx++;
    }
    if (u8NwkIdx >= u8NwkCount) {
        DBG_vPrintf(TRACE_CLASSIC, "No more nwks to try\n");
        vDiscoverNetworks();
    }
}
/****************************************************************************
 *
 * NAME: vAppSetGroupId
 *
 * DESCRIPTION:
 * sets the group id
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vAppSetGroupId(uint8 u8GroupId_l)
{
    u8GroupId = u8GroupId_l;
}

/****************************************************************************
 *
 * NAME: psGetEndpointRecordTable
 *
 * DESCRIPTION:
 * returns the address of the light data base
 *
 * RETURNS:
 * point to table of end point records
 *
 ****************************************************************************/
PUBLIC tsZllEndpointInfoTable * psGetEndpointRecordTable(void)
{
    return &sEndpointTable;
}

/****************************************************************************
 *
 * NAME: psGetGroupRecordTable
 *
 * DESCRIPTION:
 * return the address of the group record table
 *
 * RETURNS:
 * pointer to the group record table
 *
 ****************************************************************************/
PUBLIC tsZllGroupInfoTable * psGetGroupRecordTable(void)
{
    return &sGroupTable;
}

/****************************************************************************
 *
 * NAME: APP_vInitialiseNode
 *
 * DESCRIPTION:
 * Initials the node before launching the application
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vInitialiseNode(void)
{
    DBG_vPrintf(TRACE_APP, "\nAPP_vInitialiseNode*");

    APP_vInitLeds();
    sZllState.eNodeState = E_REMOTE_STARTUP;

    uint16 u16BytesRead;


    /* Initialise buttons;
     */
    APP_bButtonInitialise();


    PDM_eReadDataFromRecord(PDM_ID_APP_ZLL_CMSSION, &sZllState,sizeof(tsZllRemoteState), &u16BytesRead);
    PDM_eReadDataFromRecord(PDM_ID_APP_END_P_TABLE, &sEndpointTable, sizeof(tsZllEndpointInfoTable), &u16BytesRead);
    PDM_eReadDataFromRecord(PDM_ID_APP_GROUP_TABLE, &sGroupTable, sizeof(tsZllGroupInfoTable), &u16BytesRead);

    DBG_vPrintf(TRACE_APP, "\nContext state:%s(%d) Zll FN %02x",
            apcStateNames[sZllState.eNodeState], sZllState.eNodeState,
            sZllState.eState);

    /* Initialise ZBPro stack */


    //Inter-PAN Messaging is enabled in the ZPS diagram for each node
    /* Set security state */
    ZPS_vDefaultKeyInit();
    ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_PRECONFIGURED_LINK_KEY, (uint8 *)&s_au8LnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);
    ZPS_vAplSecSetInitialSecurityState(ZPS_ZDO_ZLL_LINK_KEY,           (uint8 *)&s_au8ZllLnkKeyArray, 0x00, ZPS_APS_GLOBAL_LINK_KEY);
    DBG_vPrintf(TRACE_REMOTE_NODE, "Set Sec state\n");

    ZPS_eAplAfInit();

    APP_ZCL_vInitialise();

    /* Set channel maskl to primary channels */
    ZPS_eAplAibSetApsChannelMask( ZLL_CHANNEL_MASK);
    DBG_vPrintf(TRACE_REMOTE_NODE, "\nSet channel mask %08x\n", ZLL_CHANNEL_MASK);

    vInitCommission();

    /* If the device state has been restored from flash, re-start the stack
     * and set the application running again.
     */
    if ((sZllState.eNodeState == E_REMOTE_RUNNING) || (ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid != 0))
    {
        DBG_vPrintf(TRACE_APP, "\nNon Factory New Start");
        sZllState.eNodeState = E_REMOTE_NFN_START;
    }
    else
    {
        sZllState.u8MyChannel = au8ZLLChannelSet[0];
        ZPS_vNwkNibSetChannel( ZPS_pvAplZdoGetNwkHandle(), sZllState.u8MyChannel);
        DBG_vPrintf(TRACE_REMOTE_NODE, "\nStart out on ch %d", sZllState.u8MyChannel);
        /* Stay awake for joining */
        DBG_vPrintf(TRACE_APP, "\nFactory New Start");

        /* Setting random PANId for Factory New remote */
        ZPS_vNwkNibSetPanId(ZPS_pvAplZdoGetNwkHandle(),
                (uint16) RND_u32GetRand(1, 0xfff0));
    }

    OS_eActivateTask(APP_ZLL_Remote_Task);

}

/****************************************************************************
 *
 * NAME: vPickChannel
 *
 * DESCRIPTION:
 * Following failure of classical join attempt at start up,
 * reset the channel and wait for user to attemp join or touch
 * link again again
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vPickChannel( void *pvNwk)
{
    DBG_vPrintf(TRACE_CLASSIC, "\nPicked Ch %d", sZllState.u8MyChannel);
    ZPS_vNwkNibSetChannel( ZPS_pvAplZdoGetNwkHandle(), sZllState.u8MyChannel);
    ZPS_vNwkNibSetPanId( pvNwk, (uint16)RND_u32GetRand(1, 0xfff0) );
    /* Set channel mask to primary channels */
    ZPS_eAplAibSetApsChannelMask( ZLL_CHANNEL_MASK);
    sZllState.eNodeState = E_REMOTE_WAIT_START;
}

/****************************************************************************
 *
 * NAME: vStartPolling
 *
 * DESCRIPTION:
 * start te remote polling, initial at a fast rate
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStartPolling(void) {
    uint8 u8Start;
    u16FastPoll = (uint16)(5/0.25);
    if (OS_eGetSWTimerStatus(APP_PollTimer) == OS_E_SWTIMER_RUNNING)
    {
        OS_eStopSWTimer(APP_PollTimer);
        DBG_vPrintf(TRACE_REMOTE_NODE, "\nStop App_PollTimer");
    }
    u8Start = OS_eStartSWTimer(APP_PollTimer, POLL_TIME_FAST, NULL);
    DBG_vPrintf(TRACE_REMOTE_NODE, "\nStart Fpoll start %d", u8Start);
}

/****************************************************************************
 *
 * NAME: vSetRejoinFilter
 *
 * DESCRIPTION:
 * Set the rejoin filter, prior to a nwk rejoin attempt
 * sets a white list pan for the current epid
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSetRejoinFilter( void)
{
    au64ExtPanList[0] = ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid;
    sBeaconFilter.pu64ExtendPanIdList = au64ExtPanList;
    sBeaconFilter.u8ListSize = 1;
    sBeaconFilter.u8Lqi = 30;
    sBeaconFilter.u8FilterMap = BF_BITMAP_WHITELIST | BF_BITMAP_LQI;
    ZPS_bAppAddBeaconFilter( &sBeaconFilter);
}

/****************************************************************************
 *
 * NAME: vDiscoverNetworks
 *
 * DESCRIPTION:
 * Sets an association filter prior to an association attempt
 * no pan filtering, filter for permit join, parent capacity and lqi
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSetAssociationFilter( void)
{
    sBeaconFilter.pu64ExtendPanIdList = NULL;
    sBeaconFilter.u8ListSize = 0;
    sBeaconFilter.u8Lqi = 41;
    sBeaconFilter.u8FilterMap = BF_BITMAP_CAP_ENDDEVICE | BF_BITMAP_PERMIT_JOIN | BF_BITMAP_LQI;
    ZPS_bAppAddBeaconFilter( &sBeaconFilter);
}

/****************************************************************************
 *
 * NAME: APP_ZLL_Remote_Task
 *
 * DESCRIPTION:
 * Task that handles the remote control related functionality
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_ZLL_Remote_Task)
{

    APP_tsEvent sAppEvent;
    ZPS_tsAfEvent sStackEvent;

    sStackEvent.eType = ZPS_EVENT_NONE;
    sAppEvent.eType = APP_E_EVENT_NONE;

    if (OS_eCollectMessage(APP_msgZpsEvents, &sStackEvent) == OS_E_OK)
    {
        if (sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION)
        {
            DBG_vPrintf(TRACE_APP, "\nDATAIND: PF :%x CL :%x EP: %x",
                        sStackEvent.uEvent.sApsDataIndEvent.u16ProfileId,
                        sStackEvent.uEvent.sApsDataIndEvent.u16ClusterId,
                        sStackEvent.uEvent.sApsDataIndEvent.u8DstEndpoint);
        }
    }
    else if (OS_eCollectMessage(APP_msgEvents, &sAppEvent) == OS_E_OK)
    {
        DBG_vPrintf(TRACE_APP, "\n%s(%d) in state %s(%d)",
                    apcAPPEventStrings[sAppEvent.eType],sAppEvent.eType,
                    apcStateNames[sZllState.eNodeState],sZllState.eNodeState);
    }

    if (sAppEvent.eType == APP_E_EVENT_TOUCH_LINK) {
        vAppHandleTouchLink( &sAppEvent);
    }

    if (sAppEvent.eType == APP_E_EVENT_EP_INFO_MSG)
    {
        vAppHandleEndpointInfo( &sAppEvent);
    }

    if (sAppEvent.eType == APP_E_EVENT_EP_LIST_MSG)
    {
        vAppHandleEndpointList( &sAppEvent);
    }

    if (sAppEvent.eType == APP_E_EVENT_GROUP_LIST_MSG) {
        vAppHandleGroupList( &sAppEvent);
    }


    /* State-dependent actions */
    switch (sZllState.eNodeState)
    {

    case E_REMOTE_STARTUP:
        /* factory new start up */
        sZllState.eNodeState = E_REMOTE_WAIT_START;
        u8KeepAliveTime = KEEP_ALIVE_FACTORY_NEW;
        OS_eActivateTask(APP_ZLL_Remote_Task);
        OS_eStartSWTimer(APP_PollTimer, APP_TIME_MS(1000), NULL);
        break;

    case E_REMOTE_WAIT_START:
        /* Wait for touchlink to form network
         * or classicval join
         * if nothing happens Sleep and Join task will force deep sleep
         */

        if (sAppEvent.eType == APP_E_EVENT_BUTTON_DOWN)
        {
            vHandleWaitStartAppEvent(&sAppEvent);
        }

        if (sStackEvent.eType != ZPS_EVENT_NONE) {
            vHandleWaitStartStackEvent(&sStackEvent);
        }
        break;

    case E_REMOTE_NETWORK_DISCOVER:
        vHandleClassicDiscoveryAndJoin(&sStackEvent);
        break;

    case E_REMOTE_NFN_START:
        sZllState.eNodeState = E_REMOTE_RUNNING;
        /* Device already in the network */
        vSetRejoinFilter();
        bFailedToJoin = TRUE;
       ZPS_eAplZdoRejoinNetwork(FALSE);
       DBG_vPrintf(TRACE_APP|TRACE_REJOIN, "\nAPP: Re-start Stack... with rejoin\n" );
       OS_eStartSWTimer(APP_PollTimer, APP_TIME_MS(1000), NULL);
       break;

    case E_REMOTE_RUNNING:
        if (sStackEvent.eType != ZPS_EVENT_NONE) {
            vHandleRunningStackEvent( &sStackEvent);
        }

        if (sAppEvent.eType != APP_E_EVENT_NONE) {
            vHandleRunningAppEvent( &sAppEvent);
        }
        break;

    default:
        break;

    }

    /*
     * Global clean up to make sure any PDUs have been freed
     */
    if (sStackEvent.eType == ZPS_EVENT_APS_DATA_INDICATION)
    {
        PDUM_eAPduFreeAPduInstance(sStackEvent.uEvent.sApsDataIndEvent.hAPduInst);
    }
}

/****************************************************************************
 *
 * NAME: vHandleWaitStartAppEvent
 *
 * DESCRIPTION: hanles events while in the eait start state, joining and touch link
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleWaitStartAppEvent(APP_tsEvent* psAppEvent)
{
    APP_CommissionEvent sEvent;

    // wait button to start touch link
    switch (psAppEvent->uEvent.sButton.u8Button)
    {
        case KEY_8:
            bSendFactoryResetOverAir=TRUE;
            break;

        case KEY_16:
            /*Add to Group 0 */
            vAppSetGroupId(0);
            sEvent.eType = APP_E_COMMISION_START;
            OS_ePostMessage(APP_CommissionEvents, &sEvent);
            break;

        case KEY_1:      // !!!!!!!!!!
            // try classic discover and join
            u8ChanIdx = 0;
            vDiscoverNetworks();
            sZllState.eNodeState = E_REMOTE_NETWORK_DISCOVER;
            break;

        case KEY_4:
            DBG_vPrintf(TRACE_APP, "\nAPP: Erasing flash records");
            PDM_vDeleteAllDataRecords();
            vAHI_SwReset();
            break;

        default:
            break;
    }
}

/****************************************************************************
 *
 * NAME: vHandleRunningAppEvent
 *
 * DESCRIPTION: handle events in the application running state,
 * stack events and button presses
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleRunningAppEvent(APP_tsEvent* psAppEvent)
{
    switch(psAppEvent->eType)
    {

    case APP_E_EVENT_NONE:
        break;

    case APP_E_EVENT_BUTTON_DOWN:
        {
            teUserKeyCodes eKeyCode = psAppEvent->uEvent.sButton.u8Button;

            #if (defined DUT_CONTROLLER)
                DUT_vHandleKeyPress(eKeyCode);
            #else
                // Hook to rejoin the network if the Light was unavailable
                if(bFailedToJoin && (u8RejoinAttemptsRemaining==0) &&
                   (KEY_16 != eKeyCode)  &&
                   ( bTLinkInProgress == FALSE))
                {
                    vSetRejoinFilter();
                    ZPS_eAplZdoRejoinNetwork(TRUE);
                    u8RejoinAttemptsRemaining = ZLL_MAX_REJOIN_ATTEMPTS;
                }
                else
                {
                    APP_vHandleKeyPress(eKeyCode);
                }
            #endif

            u8KeepAliveTime = KEEP_ALIVETIME;
            u8DeepSleepTime = DEEP_SLEEPTIME;
        }
        break;

    case APP_E_EVENT_BUTTON_UP:
        #if !(defined DUT_CONTROLLER)
            APP_vHandleKeyRelease(psAppEvent->uEvent.sButton.u8Button);
        #endif
        break;

    default:
        break;
    }
}

/****************************************************************************
 *
 * NAME: vHandleWaitStartStackEvent
 *
 * DESCRIPTION: handle events in the wait start state - stack join related events
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleWaitStartStackEvent(ZPS_tsAfEvent* psStackEvent)
{
    switch (psStackEvent->eType)
    {
        case ZPS_EVENT_NWK_JOINED_AS_ENDDEVICE:
            DBG_vPrintf(TRACE_REMOTE_NODE|TRACE_REJOIN, "\nWAIT START Joined as Ed");
            vResetSleepAndJoinCounters();
            bTLinkInProgress = FALSE;
            sZllState.eNodeState = E_REMOTE_RUNNING;
            PDM_eSaveRecordData(PDM_ID_APP_ZLL_CMSSION, &sZllState,sizeof(tsZllRemoteState));

            if (sEndpointTable.asEndpointRecords[0].u16NwkAddr) {
                bAddrMode = FALSE;           // ensure not in group mode
                DBG_vPrintf(TRACE_REMOTE_NODE, "\nNEW3 Send group add %d to %04x\n", sGroupTable.asGroupRecords[0].u16GroupId,
                sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr);
                u8AddGroupToIndex = sEndpointTable.u8CurrentLight;
                bAddGroupToAll = FALSE;
                OS_eStartSWTimer(APP_AddGroupTimer, APP_TIME_MS(500), NULL);
            }
            vHandleIdentifyRequest(5);
            vStartPolling();
            break;

        case ZPS_EVENT_NWK_FAILED_TO_JOIN:
            DBG_vPrintf(TRACE_REMOTE_NODE|TRACE_REJOIN, "Wait Start: Failed to join \n");
            bFailedToJoin = TRUE;
            if (ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid != 0)
            {
                DBG_vPrintf(TRACE_REMOTE_NODE, "Restore epid %016llx\n", ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid);
                ZPS_vNwkNibSetExtPanId(ZPS_pvAplZdoGetNwkHandle(), ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid);
            }
            break;
        default:
            break;
    }
}

/****************************************************************************
 *
 * NAME: vHandleRunningStackEvent
 *
 * DESCRIPTION: handles stack events in th running state
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleRunningStackEvent(ZPS_tsAfEvent* psStackEvent)
{
    static bool bTrySecondary = FALSE;
    void * pvNwk = ZPS_pvAplZdoGetNwkHandle();

    switch (psStackEvent->eType)
    {
    case ZPS_EVENT_NWK_JOINED_AS_ENDDEVICE:
        bFailedToJoin = FALSE;
        bTLinkInProgress = FALSE;
        vResetSleepAndJoinCounters();
        DBG_vPrintf(TRACE_REMOTE_NODE|TRACE_REJOIN, "Joined as Ed while running\n");
        vStartPolling();
        sZllState.u16MyAddr = psStackEvent->uEvent.sNwkJoinedEvent.u16Addr;
        sZllState.u8MyChannel = ZPS_psNwkNibGetHandle( pvNwk)->sPersist.u8VsChannel;
        PDM_eSaveRecordData(PDM_ID_APP_ZLL_CMSSION, &sZllState,sizeof(tsZllRemoteState));
        bTrySecondary = FALSE;
        vHandleIdentifyRequest(5);
        break;

    case ZPS_EVENT_NWK_FAILED_TO_JOIN:
        DBG_vPrintf(TRACE_REMOTE_NODE, "failed join running %02x\n",
                            psStackEvent->uEvent.sNwkJoinFailedEvent.u8Status );
        bFailedToJoin = TRUE;
        if (ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid != 0)
        {
            DBG_vPrintf(TRACE_REMOTE_NODE, "Restore epid %016llx\n", ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid);
            ZPS_vNwkNibSetExtPanId(ZPS_pvAplZdoGetNwkHandle(), ZPS_psAplAibGetAib()->u64ApsUseExtendedPanid);
        }
        if (ZPS_eAplAibGetApsTrustCenterAddress() != 0xffffffffffffffffULL)
        {
            /* on a non ZLL network */
            if (!bTrySecondary)
            {
                DBG_vPrintf(TRACE_REMOTE_NODE|TRACE_REJOIN, "Trying Secondary\n");
                bTrySecondary = TRUE;
                ZPS_eAplAibSetApsChannelMask( ZLL_SECOND_CHANNEL_MASK);
                vSetRejoinFilter();
                ZPS_eAplZdoRejoinNetwork(TRUE);
            }
            else
            {
                /* put channel back back */
                DBG_vPrintf(TRACE_REMOTE_NODE, "Back On primary\n");
                ZPS_eAplAibSetApsChannelMask( ZLL_CHANNEL_MASK);
                bTrySecondary = FALSE;
            }
        }
        break;
    case ZPS_EVENT_NWK_LEAVE_CONFIRM:
        if (psStackEvent->uEvent.sNwkLeaveConfirmEvent.u64ExtAddr == 0) {
            /* reset app data and restart */
            vFactoryResetRecords();
            /* force a restart */
            vAHI_SwReset();
        }
        break;
    case ZPS_EVENT_NWK_LEAVE_INDICATION:
        if (psStackEvent->uEvent.sNwkLeaveIndicationEvent.u64ExtAddr == 0)
        {
            /* reset app data and restart */
            vFactoryResetRecords();
            /* force a restart */
            vAHI_SwReset();
        }
        break;
    default:
        break;
    }
}

/****************************************************************************
 *
 * NAME: vHandleClassicDiscoveryAndJoin
 *
 * DESCRIPTION: handles classic join and discovery events (association)
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleClassicDiscoveryAndJoin(ZPS_tsAfEvent* psStackEvent)
{
    void* pvNwk;

    pvNwk = ZPS_pvAplZdoGetNwkHandle();

    switch (psStackEvent->eType)
    {
        case ZPS_EVENT_NWK_DISCOVERY_COMPLETE:
            DBG_vPrintf(TRACE_CLASSIC, "Disc st %d c %d sel %d\n",
                                psStackEvent->uEvent.sNwkDiscoveryEvent.eStatus,
                                psStackEvent->uEvent.sNwkDiscoveryEvent.u8NetworkCount,
                                psStackEvent->uEvent.sNwkDiscoveryEvent.u8SelectedNetwork);
            if ( (psStackEvent->uEvent.sNwkDiscoveryEvent.eStatus == 0) ||
                 (psStackEvent->uEvent.sNwkDiscoveryEvent.eStatus += ZPS_NWK_ENUM_NEIGHBOR_TABLE_FULL) )
            {
#if TRACE_CLASSIC
                int i;
                for (i=0; i<psStackEvent->uEvent.sNwkDiscoveryEvent.u8NetworkCount; i++) {
                    DBG_vPrintf(TRACE_CLASSIC, "Pan %016llx Ch %d RCap %d PJoin %d Sfpl %d ZBVer %d\n",
                            psStackEvent->uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u64ExtPanId,
                            psStackEvent->uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u8LogicalChan,
                            psStackEvent->uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u8RouterCapacity,
                            psStackEvent->uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u8PermitJoining,
                            psStackEvent->uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u8StackProfile,
                            psStackEvent->uEvent.sNwkDiscoveryEvent.psNwkDescriptors[i].u8ZigBeeVersion);
                }
#endif
                u8NwkIdx = 0;
                u8NwkCount = psStackEvent->uEvent.sNwkDiscoveryEvent.u8NetworkCount;
                psNwkDescTbl = psStackEvent->uEvent.sNwkDiscoveryEvent.psNwkDescriptors;
                vTryNwkJoin();
            }
            else
            {
                DBG_vPrintf(TRACE_CLASSIC, "Fail and not full 502x\n", psStackEvent->uEvent.sNwkDiscoveryEvent.eStatus);
                vDiscoverNetworks();
            }
            break;

        case ZPS_EVENT_NWK_FAILED_TO_JOIN:
            DBG_vPrintf(TRACE_CLASSIC, "Join failed %02x\n", psStackEvent->uEvent.sNwkJoinFailedEvent.u8Status  );
            vTryNwkJoin();
            break;

        case ZPS_EVENT_NWK_JOINED_AS_ENDDEVICE:
            sZllState.eNodeState = E_REMOTE_RUNNING;
            vResetSleepAndJoinCounters();
            sZllState.eNodeState = E_REMOTE_RUNNING;

            /* set the aps use pan id */
            ZPS_eAplAibSetApsUseExtendedPanId( ZPS_u64NwkNibGetEpid(ZPS_pvAplZdoGetNwkHandle()) );

            sZllState.eState = FACTORY_NEW_REMOTE;
            sZllState.u16MyAddr = psStackEvent->uEvent.sNwkJoinedEvent.u16Addr;
            sZllState.u8MyChannel = ZPS_psNwkNibGetHandle( pvNwk)->sPersist.u8VsChannel;
            vSetGroupAddress(0x0001, GROUPS_REQUIRED);
            sZllState.u16FreeAddrLow = 0;
            sZllState.u16FreeAddrHigh = 0;
            sZllState.u16FreeGroupLow = 0;
            sZllState.u16FreeGroupHigh = 0;
            PDM_eSaveRecordData(PDM_ID_APP_ZLL_CMSSION, &sZllState,sizeof(tsZllRemoteState));
            DBG_vPrintf(TRACE_CLASSIC, "Classic Join as Zed\n");
            vStartPolling();
            vHandleIdentifyRequest(5);
            break;
        default:
            break;
    }
}

/****************************************************************************
 *
 * NAME: vAppHandleTouchLink
 *
 * DESCRIPTION: handles touck link events, ie new lights pr remotes
 * completed a touch link process. add to light data pbase or query
 *  their data base as required.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vAppHandleTouchLink(APP_tsEvent* psAppEvent)
{
    tsCLD_ZllUtility_EndpointInformationCommandPayload sPayload;
    tsZCL_Address               sDestinationAddress;
    tsCLD_ZllDeviceTable * psDevTab = (tsCLD_ZllDeviceTable*)psGetDeviceTable();
    uint8 u8SeqNo;

    DBG_vPrintf(TRACE_REMOTE_NODE, "\nTLink %04x Ep %d Dev %04x", psAppEvent->uEvent.sTouchLink.u16NwkAddr,
            psAppEvent->uEvent.sTouchLink.u8Endpoint,
            psAppEvent->uEvent.sTouchLink.u16DeviceID);

    if ((psAppEvent->uEvent.sTouchLink.u16DeviceID >= COLOUR_REMOTE_DEVICE_ID) &&
         (psAppEvent->uEvent.sTouchLink.u16DeviceID <= ON_OFF_SENSOR_DEVICE_ID ))
    {
        /*
         * Just added a controller device, send endpoint info
         */

        sPayload.u64IEEEAddr = psDevTab->asDeviceRecords[0].u64IEEEAddr;
        sPayload.u16NwkAddr = ZPS_u16AplZdoGetNwkAddr();
        sPayload.u16DeviceID = psDevTab->asDeviceRecords[0].u16DeviceId;
        sPayload.u16ProfileID = psDevTab->asDeviceRecords[0].u16ProfileId;
        sPayload.u8Endpoint = psDevTab->asDeviceRecords[0].u8Endpoint;
        sPayload.u8Version = psDevTab->asDeviceRecords[0].u8Version;

        DBG_vPrintf(TRACE_REMOTE_NODE, "\nTell new controller about us %04x", psAppEvent->uEvent.sTouchLink.u16NwkAddr);

        sDestinationAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
        sDestinationAddress.uAddress.u16DestinationAddress = psAppEvent->uEvent.sTouchLink.u16NwkAddr;
        eCLD_ZllUtilityCommandEndpointInformationCommandSend(
                            sDeviceTable.asDeviceRecords[0].u8Endpoint,                        // src ep
                            psAppEvent->uEvent.sTouchLink.u8Endpoint,            // dst ep
                            &sDestinationAddress,
                            &u8SeqNo,
                            &sPayload);
    }
    else
    {
        if (psAppEvent->uEvent.sTouchLink.u16DeviceID <= COLOUR_TEMPERATURE_LIGHT_DEVICE_ID )
        {
            /*
             * Controlled device attempt to add to device endpoint table
             */
            if (bAddToEndpointTable( &psAppEvent->uEvent.sTouchLink)) {
                /* Added new or updated old
                 * ensure that it has our group address
                 */
                PDM_eSaveRecordData(PDM_ID_APP_END_P_TABLE, &sEndpointTable,
                        sizeof(tsZllEndpointInfoTable));
                bAddrMode = FALSE;           // ensure not in group mode
                DBG_vPrintf(TRACE_REMOTE_NODE, "\nNEW Send group add %d to %04x\n", sGroupTable.asGroupRecords[0].u16GroupId,
                    sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr);

                vAppIdentifyEffect(E_CLD_IDENTIFY_EFFECT_OKAY);
                u8AddGroupToIndex = sEndpointTable.u8CurrentLight;
                bAddGroupToAll = FALSE;
                OS_eStartSWTimer(APP_AddGroupTimer, APP_TIME_MS(500), NULL);

            }
        }
    }
}

/****************************************************************************
 *
 * NAME: vAppHandleEndpointInfo
 *
 * DESCRIPTION: handle end point info requests from the zll utility cluster
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vAppHandleEndpointInfo(APP_tsEvent* psAppEvent)
{
    tsZCL_Address sAddress;
    uint8 u8SeqNo = 0xaf;

    // we are end device so no point looking in NT
    ZPS_eAplZdoAddAddrMapEntry(psAppEvent->uEvent.sEpInfoMsg.sPayload.u16NwkAddr,
                               psAppEvent->uEvent.sEpInfoMsg.sPayload.u64IEEEAddr, FALSE);

    /* Respond with a request for the endpoint information
     *
     */

    sAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
    sAddress.uAddress.u16DestinationAddress = psAppEvent->uEvent.sEpInfoMsg.sPayload.u16NwkAddr;

    DBG_vPrintf(TRACE_REMOTE_NODE, "Reply to %04x\n", sAddress.uAddress.u16DestinationAddress);

    eCLD_ZllUtilityCommandGetEndpointListReqCommandSend(
            sDeviceTable.asDeviceRecords[0].u8Endpoint,         // src ep
            psAppEvent->uEvent.sEpInfoMsg.sPayload.u8Endpoint,                 // dst ep
            &sAddress,
            &u8SeqNo,
            0);
}

/****************************************************************************
 *
 * NAME: vAppHandleEndpointList
 *
 * DESCRIPTION: handle the end point list response from the
 * zll utility cluster
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vAppHandleEndpointList(APP_tsEvent* psAppEvent)
{
    int i;
    tsZCL_Address sAddress;
    uint8 u8SeqNo = 0xad;

    //DBG_vPrintf(TRACE_REMOTE_NODE, "got list with %d\n", sAppEvent.uEvent.sEpListMsg.sPayload.u8Count);
    for (i=0; i<psAppEvent->uEvent.sEpListMsg.sPayload.u8Count; i++) {
        /* For every endpoint in the list if it is controlled device (light etc)
         * add it to the end point table
         */

        if (psAppEvent->uEvent.sEpListMsg.sPayload.asEndpointRecords[i].u16DeviceID <= COLOUR_TEMPERATURE_LIGHT_DEVICE_ID )
        {
            if ( !bAddToEndpointTable( &psAppEvent->uEvent.sEpListMsg.sPayload.asEndpointRecords[i]))
            {
                /* If added to table or table updated ensure the device has our group address
                 *
                 */
                DBG_vPrintf(TRACE_REMOTE_NODE, "\nFaile to add %04x to table\n",
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr);

            }
        }
    }
    /* Save the end point table */
    PDM_eSaveRecordData(PDM_ID_APP_END_P_TABLE, &sEndpointTable,
                                sizeof(tsZllEndpointInfoTable));
    u8AddGroupToIndex = 0;
    bAddGroupToAll = TRUE;
    OS_eStartSWTimer(APP_AddGroupTimer, APP_TIME_MS(1000), NULL);

    /* Ask for the list of group addresses
     *
     */

    sAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
    sAddress.uAddress.u16DestinationAddress = psAppEvent->uEvent.sEpListMsg.u16SrcAddr;

    eCLD_ZllUtilityCommandGetGroupIdReqCommandSend( sDeviceTable.asDeviceRecords[0].u8Endpoint,                     // src ep
                                                    psAppEvent->uEvent.sEpListMsg.u8SrcEp,              // dst ep
                                                    &sAddress,
                                                    &u8SeqNo,
                                                    0);
}

/****************************************************************************
 *
 * NAME: vAppHandleGroupList
 *
 * DESCRIPTION: handle the group list response from the
 * zll utility cluster
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vAppHandleGroupList(APP_tsEvent* psAppEvent)
{
    DBG_vPrintf(TRACE_REMOTE_NODE, "\ngot gp list with %d", psAppEvent->uEvent.sGroupListMsg.sPayload.u8Count);
    int i;

    for (i=0; i<psAppEvent->uEvent.sGroupListMsg.sPayload.u8Count; i++)
    {
        DBG_vPrintf(TRACE_REMOTE_NODE, "\nGroup %04x %d",
                psAppEvent->uEvent.sGroupListMsg.sPayload.asGroupRecords[i].u16GroupID,
                psAppEvent->uEvent.sGroupListMsg.sPayload.asGroupRecords[i].u8GroupType);
    }
}


/****************************************************************************
 *
 * NAME: APP_vHandleKeyPress
 *
 * Button map is as follow, please update as changes are made:
 *
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * |0 Brightness Up     | |0 Move Hue Up       | |0 Move Saturation Up| |0 On                |
 * |1 Brightness Up     | |1 Move Color Temp Up| |1 Set Color Loop    | |1 On                |
 * |2 Brightness Up     | |2                   | |2                   | |2 On                |
 * |3 Factory New(Note1)| |3                   | |3                   | |3 On                |
 * |                    | |                    | |                    | |                    |
 * |        1(+)        | |       2(1)         | |       3(2)         | |       4(|)         |
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * |0 Brightness Down   | |0 Move Hue Down     | |0 Move Saturation Down|0 Off               |
 * |1 Brightness Down   | |1 Move Color Temp Down|1 Set Color Loop Stop |1 Off               |
 * |2 Brightness Down   | |2                   | |2                   | |2 Off               |
 * |3 Factory New(Note1)| |3                   | |3                   | |3 Off               |
 * |                    | |                    | |                    | |                    |
 * |       5(-)         | |       6(3)         | |        7(4)        | |        8(O)        | |                    | |                    | |                    | |                    |
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * |0 Recall Scene 1    | |0 Recall Scene 2    | |0 Recall Scene 3    | |0 Recall Scene 4    |
 * |1 Scene Save 1      | |1 Scene Save 2      | |1 Scene Save 3      | |1 Scene Save 4      |
 * |2                   | |2                   | |2                   | |2                   |
 * |3                   | |3                   | |3                   | |3                   |
 * |                    | |                    | |                    | |                    |
 * |        9(A)        | |       10(B)        | |       11(C)        | |       12(D)        |
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * |0 Shift             | |0 Unicast/Groupcast | |0 Identify/Next Light |0 TouchLink         |
 * |1 Shift             | |1 Unicast/Groupcast | |1 Identify/Next Light |1 TouchLink         |
 * |2 Shift             | |2 Unicast/Groupcast | |2 Identify/Next Light |2 TouchLink         |
 * |3 Shift             | |3 Unicast/Groupcast | |3 Identify/Next Light |3 TouchLink         |
 * |                    | |                    | |                    | |                    |
 * |       13(*)        | |       14(?)        | |       15(>)        | |      16(#)         |
 * +--------------------+ +--------------------+ +--------------------+ +--------------------+
 * Note1: FactoryNew(erases persistent data from the self(remote control unit))
 *        KeySequence: '-' '+' '-'
 *
 * DESCRIPTION:
 * Handles key presses
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_vHandleKeyPress(teUserKeyCodes eKeyCode)
{
    static uint8 u8SelfFR=0;

    APP_CommissionEvent sEvent;

    switch (eShiftLevel)
    {
        case E_SHIFT_0:
           bSendFactoryResetOverAir=FALSE;
            u8SelfFR = 0;

            switch (eKeyCode)
            {
                case KEY_1: // Brightness Up
                            vAppLevelMove(E_CLD_LEVELCONTROL_MOVE_MODE_UP, LEVEL_CHANGE_STEPS_PER_SEC_FAST, TRUE);

                            break;
                case KEY_2: // Move Hue Up
                            #ifdef CLD_COLOUR_CONTROL
#if STOP_COLOUR_LOOP
                            vAPPColourLoopStop();
#endif
                            vAppEnhancedMoveHue(E_CLD_COLOURCONTROL_MOVE_MODE_UP, 4000);
                            #endif
                            break;
                case KEY_3: // Move Saturation Up
                            #ifdef CLD_COLOUR_CONTROL
#if STOP_COLOUR_LOOP
                                vAPPColourLoopStop();
#endif
                                vAppMoveSaturation(E_CLD_COLOURCONTROL_MOVE_MODE_UP, SAT_CHANGE_STEPS_PER_SEC);
                            #endif
                            break;
                case KEY_4: // On
                            vAppOnOff(E_CLD_ONOFF_CMD_ON);
                            break;
                case KEY_5: // Brightness Down
                            vAppLevelMove(E_CLD_LEVELCONTROL_MOVE_MODE_DOWN, LEVEL_CHANGE_STEPS_PER_SEC_FAST, FALSE);
                            break;
                case KEY_6: // Move Hue Down
                            #ifdef CLD_COLOUR_CONTROL
#if STOP_COLOUR_LOOP
                                vAPPColourLoopStop();
#endif
                                vAppEnhancedMoveHue(E_CLD_COLOURCONTROL_MOVE_MODE_DOWN, 4000);
                            #endif
                            break;
                case KEY_7: // Move Saturation Down
                            #ifdef CLD_COLOUR_CONTROL
#if STOP_COLOUR_LOOP
                                vAPPColourLoopStop();
#endif
                                vAppMoveSaturation(E_CLD_COLOURCONTROL_MOVE_MODE_DOWN, SAT_CHANGE_STEPS_PER_SEC);
                            #endif
                            break;
                case KEY_8: // Off
                            //vAppOnOff( E_CLD_ONOFF_CMD_OFF_EFFECT);
                            vAppOnOff( E_CLD_ONOFF_CMD_OFF);
                            break;
                case KEY_9: // Recall Scene 1
                            #ifdef CLD_SCENES
                                bAddrMode=TRUE;
                                vAppRecallSceneById(1,sGroupTable.asGroupRecords[0].u16GroupId);
                            #endif
                            break;
                case KEY_10: // Recall Scene 2
                             #ifdef CLD_SCENES
                                 bAddrMode=TRUE;
                                 vAppRecallSceneById(2,sGroupTable.asGroupRecords[0].u16GroupId);
                             #endif
                             break;
                case KEY_11: // Recall Scene 3
                            #ifdef CLD_SCENES
                                bAddrMode=TRUE;
                                vAppRecallSceneById(3,sGroupTable.asGroupRecords[0].u16GroupId);
                            #endif
                            break;
                case KEY_12: // Recall Scene 4
                             #ifdef CLD_SCENES
                                 bAddrMode=TRUE;
                                 vAppRecallSceneById(4,sGroupTable.asGroupRecords[0].u16GroupId);
                            #endif
                             break;
                case KEY_13: // Shift
                             eShiftLevel += 1;
                             break;
                case KEY_14: // Unicast/Groupcast Toggle
                             bAddrMode = !bAddrMode;
                             break;
                case KEY_15: // Identify / Next Light
                             bAddrMode=FALSE;
                             vSelectLight();
                             break;
                case KEY_16: // TouchLink and Add to Default Group
                             vAppSetGroupId(0);
                             sEvent.eType = APP_E_COMMISION_START;
                             u8RejoinAttemptsRemaining = 10;
                             OS_ePostMessage(APP_CommissionEvents, &sEvent);
                             break;
                default : //Default
                          break;
            }
            break;

        case E_SHIFT_1:
            bSendFactoryResetOverAir=FALSE;
            u8SelfFR =0;
            switch (eKeyCode)
            {
                case KEY_1: // Brightness Up
                            vAppLevelMove(E_CLD_LEVELCONTROL_MOVE_MODE_UP, LEVEL_CHANGE_STEPS_PER_SEC_MED, TRUE);
                            break;
                case KEY_2: // Move Color Temperature Up
                            #ifdef CLD_COLOUR_CONTROL
#if STOP_COLOUR_LOOP
                                vAPPColourLoopStop();
#endif
                                vAppMoveColourTemperature(E_CLD_COLOURCONTROL_MOVE_MODE_UP,
                                                          COL_TEMP_CHANGE_STEPS_PER_SEC,
                                                          COL_TEMP_PHY_MIN,
                                                          COL_TEMP_PHY_MAX);
                            #endif
                            break;
                case KEY_3: // Set Color Loop
                            #ifdef CLD_COLOUR_CONTROL
                                vAppColorLoopSet(0x07,                                                            // flags
                                                 E_CLD_COLOURCONTROL_COLOURLOOP_ACTION_ACTIVATE_FROM_CURRENT,    // action
                                                 E_CLD_COLOURCONTROL_COLOURLOOP_DIRECTION_INCREMENT,            // direction
                                                 20,                                                             // Finish a loop in 60 secs
                                                 0x00);                                                            // start hue
                            #endif
                            break;
                case KEY_4: // On
                            vAppOnOff(E_CLD_ONOFF_CMD_ON);
                            break;
                case KEY_5: // Brightness Down
                            vAppLevelMove(E_CLD_LEVELCONTROL_MOVE_MODE_DOWN, LEVEL_CHANGE_STEPS_PER_SEC_MED, FALSE);
                            break;
                case KEY_6: //Move Temperature Down
                            #ifdef CLD_COLOUR_CONTROL
#if STOP_COLOUR_LOOP
                                vAPPColourLoopStop();
#endif
                                /* move the real colour temperature down */
                                   vAppMoveColourTemperature(E_CLD_COLOURCONTROL_MOVE_MODE_DOWN,
                                                             COL_TEMP_CHANGE_STEPS_PER_SEC,
                                                             COL_TEMP_PHY_MIN,
                                                             COL_TEMP_PHY_MAX);

                            #endif
                            break;
                case KEY_7: //Stop color Loop
                            #ifdef CLD_COLOUR_CONTROL
                                vAPPColourLoopStop();                                                    // stat hue
                            #endif
                            break;
                case KEY_8: // Off
                            vAppOnOff(E_CLD_ONOFF_CMD_OFF_EFFECT);
                            break;
                case KEY_9: // Scene Save 1
                            #ifdef CLD_SCENES
                                bAddrMode=TRUE;
                                vAppStoreSceneById(1,sGroupTable.asGroupRecords[0].u16GroupId);
                                eShiftLevel = 0;
                            #endif
                            break;
                case KEY_10: // Scene Save 2
                             #ifdef CLD_SCENES
                                 bAddrMode=TRUE;
                                 vAppStoreSceneById(2,sGroupTable.asGroupRecords[0].u16GroupId);
                                 eShiftLevel = 0;
                             #endif
                             break;
                case KEY_11: // Scene Save 3
                             #ifdef CLD_SCENES
                                bAddrMode=TRUE;
                                vAppStoreSceneById(3,sGroupTable.asGroupRecords[0].u16GroupId);
                                eShiftLevel = 0;
                             #endif
                             break;
                case KEY_12: // Scene Save 4
                             #ifdef CLD_SCENES
                                 bAddrMode=TRUE;
                                 vAppStoreSceneById(4,sGroupTable.asGroupRecords[0].u16GroupId);
                                 eShiftLevel = 0;
                             #endif
                             break;
                case KEY_13: // Shift
                             eShiftLevel += 1;
                             break;
                case KEY_14: // Unicast/Groupcast Toggle
                             bAddrMode = !bAddrMode;
                             break;
                case KEY_15: // Identify/Next Light
                             bAddrMode=FALSE;
                             vSelectLight();
                             break;
                case KEY_16: // TouchLink and Add to Default Group
                             vAppSetGroupId(0);
                             u8RejoinAttemptsRemaining = 10;
                             sEvent.eType = APP_E_COMMISION_START;
                             OS_ePostMessage(APP_CommissionEvents, &sEvent);
                             break;
                default : //Default
                          break;
            }
            break;

        case E_SHIFT_2:
            switch(eKeyCode)
            {
            case KEY_1: // Brightness Up
                        vAppLevelMove(E_CLD_LEVELCONTROL_MOVE_MODE_UP, LEVEL_CHANGE_STEPS_PER_SEC_SLOW, TRUE);
                        break;
            case KEY_2:
            #ifdef CLD_COLOUR_CONTROL
#if STOP_COLOUR_LOOP
                vAPPColourLoopStop();
#endif
                vAppGotoHueAndSat( 1);
            #endif
                break;
            case KEY_4: // On
                        vAppOnOff(E_CLD_ONOFF_CMD_ON);
                        break;
            case KEY_5: // Brightness Down
                        vAppLevelMove(E_CLD_LEVELCONTROL_MOVE_MODE_DOWN, LEVEL_CHANGE_STEPS_PER_SEC_SLOW, FALSE);
                        break;
            case KEY_6:
            #ifdef CLD_COLOUR_CONTROL
#if STOP_COLOUR_LOOP
                vAPPColourLoopStop();
#endif
                 vAppGotoHueAndSat( 0);
            #endif
                 break;
            case KEY_8: // Off
                        vAppOnOff(E_CLD_ONOFF_CMD_OFF_EFFECT);
                        break;
            case KEY_11:
                        vSendPermitJoin();
                        break;
            case KEY_12: //Channel change from any channel other than 15 to 15 or from 15 to 11
                         vAppChangeChannel();
                         break;
            case KEY_13: // Shift
                         eShiftLevel += 1;
                         break;
            case KEY_14: // Unicast/Groupcast Toggle
                         bAddrMode = !bAddrMode;
                         break;
            case KEY_15: // Identify/Next Light
                         bAddrMode=FALSE;
                         vSelectLight();
                         break;
            case KEY_16: // TouchLink and Add to Default Group
                         vAppSetGroupId(0);
                         u8RejoinAttemptsRemaining = 10;
                         sEvent.eType = APP_E_COMMISION_START;
                         OS_ePostMessage(APP_CommissionEvents, &sEvent);
                         break;
            default : //Default
                      break;
            }
            break;

        case E_SHIFT_3:
            switch(eKeyCode)
            {
                case KEY_1:
                            if(u8SelfFR==1)u8SelfFR=2;
                            break;
                case KEY_4: // On
                            vAppOnOff(E_CLD_ONOFF_CMD_ON);
                            break;
                case KEY_5: // Reset Self to Factory New
                            if (u8SelfFR==0)u8SelfFR=1;
                            if(u8SelfFR==2)
                            {
                                u8SelfFR=0;
                                void *pvNwk = ZPS_pvAplZdoGetNwkHandle();
                                ZPS_tsNwkNib *psNib = ZPS_psNwkNibGetHandle( pvNwk);
                                u32OldFrameCtr = psNib->sTbl.u32OutFC + 10;

                                if (ZPS_E_SUCCESS !=  ZPS_eAplZdoLeaveNetwork(0, FALSE,FALSE)) {
                                    /* Leave failed, probably lost parent, so just reset everything */

                                    vFactoryResetRecords();
                                    /* force a restart */
                                    vAHI_SwReset();
                                }


                            }
                            break;
                case KEY_8: // Off
                            vAppOnOff(E_CLD_ONOFF_CMD_OFF_EFFECT);
                            break;
                case KEY_9:
                    vAppIdentifyEffect(E_CLD_IDENTIFY_EFFECT_BLINK);
                    break;
                case KEY_10:
                     vAppIdentifyEffect(E_CLD_IDENTIFY_EFFECT_BREATHE);
                     break;
                case KEY_11:
                     vAppIdentifyEffect(E_CLD_IDENTIFY_EFFECT_OKAY);
                     break;
                case KEY_12:
                     vAppIdentifyEffect(E_CLD_IDENTIFY_EFFECT_CHANNEL_CHANGE);
                     break;
                case KEY_13: // Shift
                             eShiftLevel = E_SHIFT_0;
                             break;
                case KEY_14: // Unicast/Groupcast Toggle
                             bAddrMode = !bAddrMode;
                             break;
                case KEY_15: // Identify/Next Light
                             bAddrMode=FALSE;
                             vSelectLight();
                             break;
                case KEY_16: // Factory Reset TouchLink Target
                             sEvent.eType = APP_E_COMMISION_START;
                             bSendFactoryResetOverAir = TRUE;
                             OS_ePostMessage(APP_CommissionEvents, &sEvent);
                             break;
                default : //Default
                          u8SelfFR =0;
                          bSendFactoryResetOverAir=FALSE;
                          break;
            }
            break;

        default:
                 u8SelfFR =0;
                 break;
    }

    /* Update LED's to reflect shift level */
     APP_vSetLeds(eShiftLevel);
     APP_vBlinkLeds(eShiftLevel);



}


/****************************************************************************
 *
 * NAME: APP_vHandleKeyRelease
 *
 * DESCRIPTION:
 * handle any actions required if a key is released,
 * eg send a stop move command
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void APP_vHandleKeyRelease(teUserKeyCodes eKeyCode)
{
    switch(eShiftLevel)
    {
        case E_SHIFT_0:
            switch(eKeyCode)
            {
                case KEY_1: vAppLevelStop();
                            break;
                case KEY_2: // Stop Move Hue
                            #ifdef CLD_COLOUR_CONTROL
                                vAppEnhancedMoveHue(E_CLD_COLOURCONTROL_MOVE_MODE_STOP, 0);
                            #endif
                            break;
                case KEY_3: // Stop Move Saturation
                            #ifdef CLD_COLOUR_CONTROL
                                vAppMoveSaturation(E_CLD_COLOURCONTROL_MOVE_MODE_STOP, 0);
                            #endif
                            break;
                case KEY_5: vAppLevelStop();
                            break;
                case KEY_6: // Stop Move Hue
                            #ifdef CLD_COLOUR_CONTROL
                                vAppEnhancedMoveHue(E_CLD_COLOURCONTROL_MOVE_MODE_STOP, 0);
                            #endif
                            break;
                case KEY_7: // Stop Move Saturation
                            #ifdef CLD_COLOUR_CONTROL
                                vAppMoveSaturation(E_CLD_COLOURCONTROL_MOVE_MODE_STOP, 0);
                            #endif
                            break;

                default:
                    break;
            }
            break;
        case E_SHIFT_1:
            switch(eKeyCode)
            {
                case KEY_1: vAppLevelStop();
                            break;
                case KEY_2: // Stop Step Color Temperature
                            #ifdef CLD_COLOUR_CONTROL
                                vAppMoveColourTemperature(E_CLD_COLOURCONTROL_MOVE_MODE_STOP, 0, COL_TEMP_PHY_MIN,COL_TEMP_PHY_MAX);
                            #endif
                            break;
                case KEY_5: vAppLevelStop();
                            break;
                case KEY_6: // Stop Step Color Temperature
                            #ifdef CLD_COLOUR_CONTROL
                                vAppMoveColourTemperature(E_CLD_COLOURCONTROL_MOVE_MODE_STOP, 0, COL_TEMP_PHY_MIN,COL_TEMP_PHY_MAX);
                            #endif
                            break;
                default:
                         break;
            }
            break;
        case E_SHIFT_2: break;
        case E_SHIFT_3: break;
    }
}



/****************************************************************************
 *
 * NAME: APP_AddGroupTask
 *
 * DESCRIPTION:
 * Task that sends a add group command to a light,
 * works through each light in the data base
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_AddGroupTask)
{
    vAppAddGroup(sGroupTable.asGroupRecords[u8GroupId].u16GroupId, u8AddGroupToIndex);
    DBG_vPrintf(TRACE_REMOTE_NODE, "Add G Task: to All %d  index %d\n", bAddGroupToAll, u8AddGroupToIndex);
    if (bAddGroupToAll)
    {
        /* adding group to all lights in the table */
        u8AddGroupToIndex++;
        if (u8AddGroupToIndex < sEndpointTable.u8NumRecords)
        {
            /* more to send, set the timer to come back */
            OS_eStartSWTimer(APP_AddGroupTimer, APP_TIME_MS(500), NULL);
            return;
        }
    }
    /* all done, stop the timer to keep sleep happy */
    OS_eStopSWTimer(APP_AddGroupTimer);
}

/****************************************************************************
 *
 * NAME: vAppOnOff
 *
 * DESCRIPTION: sends an on off command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppOnOff(teCLD_OnOff_Command eCmd) {
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, bAddModeBroadcast);

    if ((eCmd == E_CLD_ONOFF_CMD_ON) ||
         (eCmd == E_CLD_ONOFF_CMD_OFF) ||
         (eCmd == E_CLD_ONOFF_CMD_TOGGLE) ||
         (eCmd == E_CLD_ONOFF_CMD_ON_RECALL_GLOBAL_SCENE))
    {
        eCLD_OnOffCommandSend( sDeviceTable.asDeviceRecords[0].u8Endpoint,
                               sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                               &sAddress,
                               &u8Seq,
                               eCmd);
    } else if (eCmd == E_CLD_ONOFF_CMD_OFF_EFFECT) {
        tsCLD_OnOff_OffWithEffectRequestPayload sPayload;
        sPayload.u8EffectId = 0;
        sPayload.u8EffectVariant = 1;

        eCLD_OnOffCommandOffWithEffectSend(
                sDeviceTable.asDeviceRecords[0].u8Endpoint,
                sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                &sAddress, &u8Seq, &sPayload);
    } else if (E_CLD_ONOFF_CMD_ON_TIMED_OFF) {
        tsCLD_OnOff_OnWithTimedOffRequestPayload sPayload;
        sPayload.u8OnOff = 0x00;  // 0x01 for accept if OnOff = 1
        sPayload.u16OnTime = 0;   // 1/10th of a second
        sPayload.u16OffTime = 20; // 1/10th of a second

        eCLD_OnOffCommandOnWithTimedOffSend(
                sDeviceTable.asDeviceRecords[0].u8Endpoint,
                sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                &sAddress, &u8Seq, &sPayload);
    }

}


/****************************************************************************
 *
 * NAME: vAppIdentify
 *
 * DESCRIPTION: sends an identify command to a light
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppIdentify( uint16 u16Time) {
    uint8 u8Seq;
    tsZCL_Address sAddress;
    tsCLD_Identify_IdentifyRequestPayload sPayload;

    sPayload.u16IdentifyTime = u16Time;


    sAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
    sAddress.uAddress.u16DestinationAddress = sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr;

    eCLD_IdentifyCommandIdentifyRequestSend(sDeviceTable.asDeviceRecords[0].u8Endpoint,
            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                            &sAddress,
                            &u8Seq,
                            &sPayload);
}

/****************************************************************************
 *
 * NAME: vAppIdentifyEffect
 *
 * DESCRIPTION: sends the specified identify effect to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppIdentifyEffect( teCLD_Identify_EffectId eEffect) {
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress,FALSE);

    eCLD_IdentifyCommandTriggerEffectSend(sDeviceTable.asDeviceRecords[0].u8Endpoint,
            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                            &sAddress,
                            &u8Seq,
                            eEffect,
                            0 /* Effect varient */);
}

#ifdef TRACE_UNUSED
/****************************************************************************
 *
 * NAME: vAppOffEffect
 *
 * DESCRIPTION: sends an off with effect command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vAppOffEffect( uint8 u8Effect, uint8 u8Varient)
{
    uint8 u8Seq;
    tsZCL_Address sAddress;
    tsCLD_OnOff_OffWithEffectRequestPayload sPayload;

    sPayload.u8EffectId = u8Effect;
    sPayload.u8EffectVariant = u8Varient;


    sAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
    sAddress.uAddress.u16DestinationAddress = sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr;

    eCLD_OnOffCommandOffWithEffectSend(sDeviceTable.asDeviceRecords[0].u8Endpoint,
            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                            &sAddress,
                            &u8Seq,
                            &sPayload );
}
#endif

/****************************************************************************
 *
 * NAME: vAppIdentifyQuery
 *
 * DESCRIPTION: sends an identify query command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppIdentifyQuery( void) {
    uint8 u8Seq;
    tsZCL_Address sAddress;


    sAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
    sAddress.uAddress.u16DestinationAddress = sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr;

    eCLD_IdentifyCommandIdentifyQueryRequestSend(sDeviceTable.asDeviceRecords[0].u8Endpoint,
            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                            &sAddress,
                            &u8Seq);
}


/****************************************************************************
 *
 * NAME: vAppLevelMove
 *
 * DESCRIPTION: sends a move level command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppLevelMove(teCLD_LevelControl_MoveMode eMode, uint8 u8Rate, bool_t bWithOnOff)
{
    tsCLD_LevelControl_MoveCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.u8Rate = u8Rate;
    sPayload.u8MoveMode = eMode;

    eCLD_LevelControlCommandMoveCommandSend(sDeviceTable.asDeviceRecords[0].u8Endpoint,
                                            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                                            &sAddress,
                                            &u8Seq,
                                            bWithOnOff, /* with on off */
                                            &sPayload);
}


/****************************************************************************
 *
 * NAME: vAppLevelStop
 *
 * DESCRIPTION: sends a move level stop command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppLevelStop(void)
{
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);
    eCLD_LevelControlCommandStopCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                        &sAddress,
                        &u8Seq);
}

/****************************************************************************
 *
 * NAME: vAppLevelStopWithOnOff
 *
 * DESCRIPTION: sends a move level stop with on off command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppLevelStopWithOnOff(void)
{
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);
    eCLD_LevelControlCommandStopWithOnOffCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                        &sAddress,
                        &u8Seq);
}


/****************************************************************************
 *
 * NAME: vAppLevelStepMove
 *
 * DESCRIPTION: sends a move step level command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppLevelStepMove(teCLD_LevelControl_MoveMode eMode, uint8 u8StepSize, uint16 u16TransitionTime, bool_t bWithOnOff)
{
    tsCLD_LevelControl_StepCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.u16TransitionTime = u16TransitionTime;
    sPayload.u8StepMode = eMode;
    sPayload.u8StepSize = u8StepSize;
    eCLD_LevelControlCommandStepCommandSend(
            sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                        &sAddress,
                        &u8Seq,
                        bWithOnOff,
                        &sPayload);
}

#ifdef CLD_COLOUR_CONTROL
/****************************************************************************
 *
 * NAME: vAppMoveHue
 *
 * DESCRIPTION: sends a move hue command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppMoveHue(teCLD_ColourControl_MoveMode eMode, uint8 u8StepsPerSec)
{
    tsCLD_ColourControl_MoveHueCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.eMode = eMode;
    sPayload.u8Rate = u8StepsPerSec;

    eCLD_ColourControlCommandMoveHueCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}


/****************************************************************************
 *
 * NAME: vAppEnhancedMoveHue
 *
 * DESCRIPTION: sends a enhanced move hue command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppEnhancedMoveHue( teCLD_ColourControl_MoveMode eMode, uint16 u16StepsPerSec)
{
    tsCLD_ColourControl_EnhancedMoveHueCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.eMode = eMode;
    sPayload.u16Rate = u16StepsPerSec;

    eCLD_ColourControlCommandEnhancedMoveHueCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}


/****************************************************************************
 *
 * NAME: vAppStepHue
 *
 * DESCRIPTION: sends a move step hue command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppStepHue(teCLD_ColourControl_StepMode eMode, uint8 u8StepSize, uint8 u8TransitionTime)
{
    tsCLD_ColourControl_StepHueCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);
    sPayload.eMode = eMode;
    sPayload.u8StepSize = u8StepSize;
    sPayload.u8TransitionTime = u8TransitionTime;

    eCLD_ColourControlCommandStepHueCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}


/****************************************************************************
 *
 * NAME: vAppEnhancedStepHue
 *
 * DESCRIPTION: sends an enhanced move step hue command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppEnhancedStepHue(teCLD_ColourControl_StepMode eMode, uint16 u16StepSize, uint16 u16TransitionTime)
{
    tsCLD_ColourControl_EnhancedStepHueCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.eMode = eMode;
    sPayload.u16StepSize = u16StepSize;
    sPayload.u16TransitionTime = u16TransitionTime;

    eCLD_ColourControlCommandEnhancedStepHueCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}

/****************************************************************************
 *
 * NAME: vAppMoveToSaturaiton
 *
 * DESCRIPTION: sends a move to saturation command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppMoveToSaturaiton(uint8 u8Saturation, uint16 u16TransitionTime)
{
    tsCLD_ColourControl_MoveToSaturationCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);
    sPayload.u8Saturation = u8Saturation;
    sPayload.u16TransitionTime = u16TransitionTime;

    eCLD_ColourControlCommandMoveToSaturationCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}

/****************************************************************************
 *
 * NAME: vAppMoveToHueAndSaturation
 *
 * DESCRIPTION: sends a move to hue and saturation command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
    PUBLIC void vAppMoveToHueAndSaturation(uint8 u8Hue, uint8 u8Saturation, uint16 u16Time)
{
    tsCLD_ColourControl_MoveToHueAndSaturationCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.u8Hue = u8Hue;
    sPayload.u8Saturation = u8Saturation;
    sPayload.u16TransitionTime = u16Time;

    eCLD_ColourControlCommandMoveToHueAndSaturationCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}

/****************************************************************************
 *
 * NAME: vAppEnhancedMoveToHueAndSaturation
 *
 * DESCRIPTION: sends an enhanced move to jue and saturation command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppEnhancedMoveToHueAndSaturation(uint16 u16Hue, uint8 u8Saturation, uint16 u16Time)
{
    tsCLD_ColourControl_EnhancedMoveToHueAndSaturationCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.u16EnhancedHue = u16Hue;
    sPayload.u8Saturation = u8Saturation;
    sPayload.u16TransitionTime = u16Time;

    eCLD_ColourControlCommandEnhancedMoveToHueAndSaturationCommandSend(
            sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}


/****************************************************************************
 *
 * NAME: vAppGotoColour
 *
 * DESCRIPTION: sends a goto colour XY command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppGotoColour( uint8 u8Colour)
{
    tsCLD_ColourControl_MoveToColourCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    /* transition over 2 seconds */
    sPayload.u16TransitionTime = 20;

    switch (u8Colour)
    {
        case 0:
            sPayload.u16ColourX = (CLD_COLOURCONTROL_RED_X * 65536) - 10;
            sPayload.u16ColourY = (CLD_COLOURCONTROL_RED_Y * 65536);
            break;
        case 1:
            sPayload.u16ColourX = (CLD_COLOURCONTROL_BLUE_X * 65536);
            sPayload.u16ColourY = (CLD_COLOURCONTROL_BLUE_Y * 65536);
            break;
        case 2:
            sPayload.u16ColourX = (CLD_COLOURCONTROL_GREEN_X * 65536);
            sPayload.u16ColourY = (CLD_COLOURCONTROL_GREEN_Y * 65536);
            break;
        case 3:
            sPayload.u16ColourX = (CLD_COLOURCONTROL_WHITE_X * 65536);
            sPayload.u16ColourY = (CLD_COLOURCONTROL_WHITE_Y * 65536);
            break;
        default:
            return;
            break;
    }

    eCLD_ColourControlCommandMoveToColourCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}

/****************************************************************************
 *
 * NAME: vAppGotoHueAndSat
 *
 * DESCRIPTION:
 * send a goto hue and saturatuion command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppGotoHueAndSat( uint8 u8Direction) {
tsCLD_ColourControl_EnhancedMoveToHueAndSaturationCommandPayload sPayload;

uint8 u8Seq;
tsZCL_Address sAddress;
static uint8 u8Step=0;



    vSetAddress(&sAddress, FALSE);

    /* transition over 2 seconds */
    sPayload.u16TransitionTime = 20;

    if (u8Direction!=0) {
        u8Step++;
        if (u8Step > 6){
            u8Step=0;
        }
    } else {
        u8Step--;
        if (u8Step == 255){
            u8Step=6;
        }
    }


    switch(u8Step) {
        case 0:
            sPayload.u16EnhancedHue = 0;
            sPayload.u8Saturation = 0;
        break;
        case 1:
            sPayload.u16EnhancedHue = 0x0000;
            sPayload.u8Saturation = 254;
            break;
        case 2:
            sPayload.u16EnhancedHue = 0x2e00;
            sPayload.u8Saturation = 254;
            break;
        case 3:
            sPayload.u16EnhancedHue = 0x5500;
            sPayload.u8Saturation = 254;
            break;
        case 4:
            sPayload.u16EnhancedHue = 0x7d00;
            sPayload.u8Saturation = 254;
            break;
        case 5:
            sPayload.u16EnhancedHue = 0xaa00;
            sPayload.u8Saturation = 254;
            break;
        case 6:
            sPayload.u16EnhancedHue = 0xdd00;
            sPayload.u8Saturation = 254;
            break;
    }
    eCLD_ColourControlCommandEnhancedMoveToHueAndSaturationCommandSend(
             sDeviceTable.asDeviceRecords[0].u8Endpoint,
                                    sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                                    &sAddress,
                                    &u8Seq,
                                    &sPayload );

}

/****************************************************************************
 *
 * NAME: vAppMoveColour
 *
 * DESCRIPTION: send a move colour XY command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppMoveColour(int16 i16RateX, int16 i16RateY)
{
    tsCLD_ColourControl_MoveColourCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.i16RateX = i16RateX;
    sPayload.i16RateY = i16RateY;

    eCLD_ColourControlCommandMoveColourCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}

/****************************************************************************
 *
 * NAME: vAppStepColour
 *
 * DESCRIPTION: send a move step colour XY command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppStepColour(int16 i16StepX, int16 i16StepY, uint16 u16TransitionTime)
{
    tsCLD_ColourControl_StepColourCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.i16StepX = i16StepX;
    sPayload.i16StepY = i16StepY;
    sPayload.u16TransitionTime = u16TransitionTime;

    eCLD_ColourControlCommandStepColourCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}


/****************************************************************************
 *
 * NAME: vAppMoveToHue
 *
 * DESCRIPTION: send a move to hue command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppMoveToHue(uint8 u8Hue, teCLD_ColourControl_Direction eDirection, uint16 u16TransitionTime)
{

    tsCLD_ColourControl_MoveToHueCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    DBG_vPrintf(TRACE_REMOTE_NODE, "\nMoveTo %02x", u8Hue);

    sPayload.u8Hue = u8Hue;
    sPayload.eDirection = eDirection;
    sPayload.u16TransitionTime = u16TransitionTime;

    eCLD_ColourControlCommandMoveToHueCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}


/****************************************************************************
 *
 * NAME: vAppEnhancedMoveToHue
 *
 * DESCRIPTION: send an enhanced move to hue command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppEnhancedMoveToHue(uint16 u16Hue, teCLD_ColourControl_Direction eDirection)
{

    tsCLD_ColourControl_EnhancedMoveToHueCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    DBG_vPrintf(TRACE_REMOTE_NODE, "\nEnc MoveHueTo %04x", u16Hue);

    sPayload.eDirection = eDirection;
    sPayload.u16TransitionTime = 10;
    sPayload.u16EnhancedHue = u16Hue;

    eCLD_ColourControlCommandEnhancedMoveToHueCommandSend(
            sDeviceTable.asDeviceRecords[0].u8Endpoint,
            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
            &sAddress,
            &u8Seq,
            &sPayload);
}


/****************************************************************************
 *
 * NAME: vAppMoveSaturation
 *
 * DESCRIPTION: send a move saturatuion command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppMoveSaturation( teCLD_ColourControl_MoveMode eMode, uint8 u8StepsPerSec)
{
    tsCLD_ColourControl_MoveSaturationCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.eMode = eMode;
    sPayload.u8Rate = u8StepsPerSec;

    eCLD_ColourControlCommandMoveSaturationCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}


/****************************************************************************
 *
 * NAME: vAppStepSaturation
 *
 * DESCRIPTION: send a move step saturatuion command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppStepSaturation( teCLD_ColourControl_StepMode eMode, uint8 u8StepSize)
{
    tsCLD_ColourControl_StepSaturationCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.eMode = eMode;
    sPayload.u8StepSize = u8StepSize;
    sPayload.u8TransitionTime = 0x0006;

    eCLD_ColourControlCommandStepSaturationCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}

/****************************************************************************
 *
 * NAME: vAppMoveToColourTemperature
 *
 * DESCRIPTION: send a move colour temperature command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppMoveToColourTemperature(uint16 u16ColourTemperatureMired,
                                        uint16 u16TransitionTime)
{
    tsCLD_ColourControl_MoveToColourTemperatureCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.u16ColourTemperatureMired = u16ColourTemperatureMired;
    sPayload.u16TransitionTime = u16TransitionTime;

    eCLD_ColourControlCommandMoveToColourTemperatureCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}

/****************************************************************************
 *
 * NAME: vAppMoveColourTemperature
 *
 * DESCRIPTION: send a move to colour temperature command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppMoveColourTemperature( teCLD_ColourControl_MoveMode eMode,
                                       uint16 u16StepsPerSec,
                                       uint16 u16ColourTemperatureMin,
                                       uint16 u16ColourTemperatureMax)
{
    tsCLD_ColourControl_MoveColourTemperatureCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.eMode = eMode;
    sPayload.u16Rate = u16StepsPerSec;
    sPayload.u16ColourTemperatureMiredMin = u16ColourTemperatureMin;
    sPayload.u16ColourTemperatureMiredMax = u16ColourTemperatureMax;

    eCLD_ColourControlCommandMoveColourTemperatureCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}

/****************************************************************************
 *
 * NAME: vAppStepMoveStop
 *
 * DESCRIPTION: send a move stop command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppStepMoveStop(void)
{
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    eCLD_ColourControlCommandStopMoveStepCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                        &sAddress,
                        &u8Seq);
}


/****************************************************************************
 *
 * NAME: vAppStepColourTemperature
 *
 * DESCRIPTION: send a move step colour temperature command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppStepColourTemperature( teCLD_ColourControl_StepMode eMode, uint16 u16StepSize)
{
    tsCLD_ColourControl_StepColourTemperatureCommandPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.eMode = eMode;
    sPayload.u16StepSize = u16StepSize;
    sPayload.u16TransitionTime = 10;
    sPayload.u16ColourTemperatureMiredMin = 4;
    sPayload.u16ColourTemperatureMiredMax = 1000;

    eCLD_ColourControlCommandStepColourTemperatureCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}

/****************************************************************************
 *
 * NAME: vAPPColourLoopStop
 *
 * DESCRIPTION: send a colour loop stop command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAPPColourLoopStop(void)
{
    vAppColorLoopSet(E_CLD_COLOURCONTROL_FLAGS_UPDATE_ACTION,            // flags
                     E_CLD_COLOURCONTROL_COLOURLOOP_ACTION_DEACTIVATE,        // action
                     0x00,                                                    // direction
                     0x00,                                                    // time
                     0x00);
}

/****************************************************************************
 *
 * NAME: vAppColorLoopSet
 *
 * DESCRIPTION: send a colour loop set command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppColorLoopSet(uint8 u8UpdateFlags, teCLD_ColourControl_LoopAction eAction,
                             teCLD_ColourControl_LoopDirection eDirection,
                             uint16 u16Time, uint16 u16StartHue)
{
    uint8 u8Seq;
    tsZCL_Address sAddress;
    tsCLD_ColourControl_ColourLoopSetCommandPayload sPayload;

    sPayload.u8UpdateFlags = u8UpdateFlags;
    sPayload.eAction       = eAction;
    sPayload.eDirection    = eDirection;
    sPayload.u16Time       = u16Time;
    sPayload.u16StartHue   = u16StartHue;

   vSetAddress(&sAddress, FALSE);

   eCLD_ColourControlCommandColourLoopSetCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}

/****************************************************************************
 *
 * NAME: vAppStopMoveStep
 *
 * DESCRIPTION: send a stop move step command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppStopMoveStep()
{
    uint8 u8Seq;
    tsZCL_Address sAddress;

   vSetAddress(&sAddress, FALSE);

   eCLD_ColourControlCommandStopMoveStepCommandSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                        &sAddress,
                        &u8Seq);
}

#endif

#ifdef CLD_SCENES
/****************************************************************************
 *
 * NAME: vAppAddScene
 *
 * DESCRIPTION:
 * send an add scene command to a light or lights,
 * this command includes all the scene data
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppAddScene(uint8 u8SceneId, uint16 u16GroupId, uint16 u16TransitionTime)
{
    uint8 u8Seq;
    tsZCL_Address sAddress;
    tsCLD_ScenesAddSceneRequestPayload sPayload;
    uint8 name[] = "";

    uint8 au8Extension[] = {0x06, 0x00, 0x01, 0x01,         /* On cluster */
                            0x08, 0x00, 0x01, 0xfe,         /* level cluster */
                            0x00, 0x03, 0x0b,               /* colour cluster */
                            0x00, 0x00, 0x00, 0x00,         /* BigX BigY */
                            0x00, 0xa0,                     /* enhanced Hue */
                            0xfe,                           /* saturation */
                            0x00,                           /* colour loop active */
                            0x00,                           /* colour loop direction */
                            0x00, 0x00};                    /* colour loop time */
    teZCL_Status eStatus;

    vSetAddress(&sAddress, FALSE);
    sPayload.u16GroupId = u16GroupId;
    sPayload.u8SceneId = u8SceneId;
    sPayload.u16TransitionTime = u16TransitionTime;
    sPayload.sSceneName.u8Length = 0;
    sPayload.sSceneName.u8MaxLength = 1;
    sPayload.sSceneName.pu8Data = name;
    sPayload.sExtensionField.pu8Data = au8Extension;
    sPayload.sExtensionField.u16Length = 22;

    eStatus = eCLD_ScenesCommandAddSceneRequestSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                        &sAddress,
                        &u8Seq,
                        &sPayload);
    DBG_vPrintf(TRACE_APP, "\neCLD_ScenesCommandAddSceneRequestSend  eStatus = %d", eStatus);

}

/****************************************************************************
 *
 * NAME: vAppEnhancedAddScene
 *
 * DESCRIPTION: send an enhanced add scene command to a light or lights,
 * this command includes all the scene data
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppEnhancedAddScene( uint8 u8Scene, uint16 u16GroupId)
{
    tsCLD_ScenesEnhancedAddSceneRequestPayload sPayload;

    uint8 au8Extension[] = {0x06, 0x00, 0x01, 0x01,         /* On cluster */
                            0x08, 0x00, 0x01, 0xfe,         /* level cluster */
                            0x00, 0x03, 0x0b,               /* colour cluster */
                            0x00, 0x00, 0x00, 0x00,         /* BigX BigY */
                            0x00, 0xa0,                     /* enhanced Hue */
                            0xfe,                           /* saturation */
                            0x00,                           /* colour loop active */
                            0x00,                           /* colour loop direction */
                            0x00, 0x00};                    /* colour loop time */

    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    DBG_vPrintf(TRACE_REMOTE_NODE, "\nAdd sc %d", u8Scene);

    switch (u8Scene)
    {
        case 0:
            break;
        case 1:
            au8Extension[7] = 127;
            au8Extension[11] = 0x47;    // Big X Blue
            au8Extension[12] = 0x21;
            au8Extension[13] = 0x33;    // Big Y Blue
            au8Extension[14] = 0x13;
            au8Extension[15] = 0;
            au8Extension[16] = 0;
            break;
        case 2:
            au8Extension[7] = 60;
            au8Extension[11] = 0xcf;    // Big X red
            au8Extension[12] = 0xb2;
            au8Extension[13] = 0xcc;    // Big Y red
            au8Extension[14] = 0x4c;
            au8Extension[15] = 0;
            au8Extension[16] = 0;
            break;
        default:
            return;
            break;
    }

    uint8 name[] = "";

    //sPayload.u16GroupId = sGroupTable.asGroupRecords[0].u16GroupId;
    sPayload.u16GroupId = u16GroupId;
    sPayload.u8SceneId = u8Scene;
    sPayload.u16TransitionTime100ms = 0x000a;
    sPayload.sSceneName.u8Length = 0;
    sPayload.sSceneName.u8MaxLength = 0;
    sPayload.sSceneName.pu8Data = name;


    sPayload.sExtensionField.pu8Data = au8Extension;
    sPayload.sExtensionField.u16Length = 22;

    eCLD_ScenesCommandEnhancedAddSceneRequestSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                        &sAddress,
                        &u8Seq,
                        &sPayload );
}

/****************************************************************************
 *
 * NAME: vAppViewScene
 *
 * DESCRIPTION:send a view scene command to a light or lights,
 * this command includes all the scene data
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppViewScene(uint8 u8SceneId, uint16 u16GroupId)
{
    uint8 u8Seq;
    tsZCL_Address sAddress;
    tsCLD_ScenesViewSceneRequestPayload sPayload;

    vSetAddress(&sAddress, FALSE);
    sPayload.u16GroupId = u16GroupId;
    sPayload.u8SceneId = u8SceneId;

    eCLD_ScenesCommandViewSceneRequestSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}

/****************************************************************************
 *
 * NAME: vAppEnhancedViewScene
 *
 * DESCRIPTION: send an enhanced view scene command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppEnhancedViewScene(uint8 u8SceneId, uint16 u16GroupId)
{
    uint8 u8Seq;
    tsZCL_Address sAddress;
    tsCLD_ScenesEnhancedViewSceneRequestPayload sPayload;

    vSetAddress(&sAddress, FALSE);
    sPayload.u16GroupId = u16GroupId;
    sPayload.u8SceneId = u8SceneId;

    eCLD_ScenesCommandEnhancedViewSceneRequestSend(
                        sDeviceTable.asDeviceRecords[0].u8Endpoint,
                        sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                        &sAddress,
                        &u8Seq,
                        &sPayload);
}

/****************************************************************************
 *
 * NAME: vAppRecallSceneById
 *
 * DESCRIPTION: send a recall scene command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppRecallSceneById( uint8 u8SceneId, uint16 u16GroupId)
{

    tsCLD_ScenesRecallSceneRequestPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.u16GroupId = u16GroupId;
    sPayload.u8SceneId = u8SceneId;

    eCLD_ScenesCommandRecallSceneRequestSend(
                            sDeviceTable.asDeviceRecords[0].u8Endpoint,
                            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint   /* don't care group addr */,
                            &sAddress,
                            &u8Seq,
                            &sPayload);

}

/****************************************************************************
 *
 * NAME: vAppRemoveSceneById
 *
 * DESCRIPTION: send a remove scene command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppRemoveSceneById( uint8 u8SceneId, uint16 u16GroupId)
{

    tsCLD_ScenesRemoveSceneRequestPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.u16GroupId = u16GroupId;
    sPayload.u8SceneId = u8SceneId;

    eCLD_ScenesCommandRemoveSceneRequestSend(
                            sDeviceTable.asDeviceRecords[0].u8Endpoint,
                            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint   /* don't care group addr */,
                            &sAddress,
                            &u8Seq,
                            &sPayload);

}

/****************************************************************************
 *
 * NAME: vAppRemoveAllScene
 *
 * DESCRIPTION:
 * send a remove all scenes command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppRemoveAllScene(uint16 u16GroupId)
{

    tsCLD_ScenesRemoveAllScenesRequestPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.u16GroupId = u16GroupId;

    eCLD_ScenesCommandRemoveAllScenesRequestSend(
                            sDeviceTable.asDeviceRecords[0].u8Endpoint,
                            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint   /* don't care group addr */,
                            &sAddress,
                            &u8Seq,
                            &sPayload);

}

/****************************************************************************
 *
 * NAME: vAppGetSceneMembership
 *
 * DESCRIPTION:sends a get scenes membership to a group of lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppGetSceneMembership(uint16 u16GroupId)
{

    tsCLD_ScenesGetSceneMembershipRequestPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.u16GroupId = u16GroupId;

    eCLD_ScenesCommandGetSceneMembershipRequestSend(
                            sDeviceTable.asDeviceRecords[0].u8Endpoint,
                            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint   /* don't care group addr */,
                            &sAddress,
                            &u8Seq,
                            &sPayload);

}
/****************************************************************************
 *
 * NAME: vAppCopyScene
 *
 * DESCRIPTION: sends a copy scene scene command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppCopyScene(uint8 u8Mode, uint8 u8FromSceneId, uint16 u16FromGroupId, uint8 u8ToSceneId, uint16 u16ToGroupId)
{

    tsCLD_ScenesCopySceneRequestPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.u8Mode = u8Mode;
    sPayload.u16FromGroupId = u16FromGroupId;
    sPayload.u8FromSceneId = u8FromSceneId;
    sPayload.u16ToGroupId = u16ToGroupId;
    sPayload.u8ToSceneId = u8ToSceneId;

    eCLD_ScenesCommandCopySceneSceneRequestSend(
                            sDeviceTable.asDeviceRecords[0].u8Endpoint,
                            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint   /* don't care group addr */,
                            &sAddress,
                            &u8Seq,
                            &sPayload);

}

/****************************************************************************
 *
 * NAME: vAppStoreSceneById
 *
 * DESCRIPTION: sends a store scene command to a light or lights
 * the actual scenwe data is the current state of the receiving light
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppStoreSceneById(uint8 u8SceneId, uint16 u16GroupId)
{
    tsCLD_ScenesStoreSceneRequestPayload sPayload;

    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, FALSE);

    sPayload.u16GroupId = u16GroupId;
    sPayload.u8SceneId = u8SceneId;


    eCLD_ScenesCommandStoreSceneRequestSend(
                            sDeviceTable.asDeviceRecords[0].u8Endpoint,
                            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,   // dst ep
                            &sAddress,
                            &u8Seq,
                            &sPayload);

}

#endif
/****************************************************************************
 *
 * NAME: vAppAddGroup
 *
 * DESCRIPTION: send an add group to the currently selected light
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppAddGroup( uint16 u16GroupId, uint8 u8Index)
{

    tsCLD_Groups_AddGroupRequestPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    sAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
    sAddress.uAddress.u16DestinationAddress = sEndpointTable.asEndpointRecords[u8Index].u16NwkAddr;


    sPayload.sGroupName.pu8Data = (uint8*)"";
    sPayload.sGroupName.u8Length = 0;
    sPayload.sGroupName.u8MaxLength = 0;
    sPayload.u16GroupId = u16GroupId;

    eCLD_GroupsCommandAddGroupRequestSend(
                            sDeviceTable.asDeviceRecords[0].u8Endpoint,
                            sEndpointTable.asEndpointRecords[u8Index].u8Endpoint   /* don't care group addr */,
                            &sAddress,
                            &u8Seq,
                            &sPayload);

}

/****************************************************************************
 *
 * NAME: vAppRemoveGroup
 *
 * DESCRIPTION:  send a remove group commanf to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppRemoveGroup( uint16 u16GroupId, bool_t bBroadcast)
{

    tsCLD_Groups_RemoveGroupRequestPayload sPayload;
    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, bBroadcast);

    sPayload.u16GroupId = u16GroupId;

    eCLD_GroupsCommandRemoveGroupRequestSend(
                            sDeviceTable.asDeviceRecords[0].u8Endpoint,
                            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint   /* don't care group addr */,
                            &sAddress,
                            &u8Seq,
                            &sPayload);

}

/****************************************************************************
 *
 * NAME: vAppRemoveAllGroups
 *
 * DESCRIPTION: send a remove all groups command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppRemoveAllGroups(bool_t bBroadcast)
{

    uint8 u8Seq;
    tsZCL_Address sAddress;

    vSetAddress(&sAddress, bBroadcast);

    eCLD_GroupsCommandRemoveAllGroupsRequestSend(
                            sDeviceTable.asDeviceRecords[0].u8Endpoint,
                            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint   /* don't care group addr */,
                            &sAddress,
                            &u8Seq);

}

/****************************************************************************
 *
 * NAME: bAppSendGroupMembershipReq
 *
 * DESCRIPTION: send a get group membership to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void bAppSendGroupMembershipReq(uint8 u8GroupCount, bool_t bBroadcast)
{
    uint8 u8SeqNum;
    tsZCL_Address sAddress;
    tsCLD_Groups_GetGroupMembershipRequestPayload sGroupMembershipReqPayload;
    teZCL_Status eZCL_Status;

    zint16 au16GroupList[] = { 0x0000 };

    sGroupMembershipReqPayload.u8GroupCount = u8GroupCount;
    sGroupMembershipReqPayload.pi16GroupList = &au16GroupList[0];

    vSetAddress(&sAddress, bBroadcast);

    eZCL_Status = eCLD_GroupsCommandGetGroupMembershipRequestSend(
                            sDeviceTable.asDeviceRecords[0].u8Endpoint, /*SrcEP*/
                            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint, /*DstEP*/
                            &sAddress,
                            &u8SeqNum,
                            &sGroupMembershipReqPayload);

    DBG_vPrintf(TRACE_APP,"\neCLD_GroupsCommandGetGroupMembershipRequestSend  eZCL_Status = %d  \n",eZCL_Status);
}

/****************************************************************************
 *
 * NAME: bAppSendViewGroupReq
 *
 * DESCRIPTION: send a view group command to a light or lights
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void bAppSendViewGroupReq(uint16 u16GroupId, bool_t bBroadcast)
{
    uint8 u8SeqNum;
    tsZCL_Address sAddress;
    tsCLD_Groups_ViewGroupRequestPayload sViewGroupReqPayload;
    teZCL_Status eZCL_Status;

    sViewGroupReqPayload.u16GroupId = u16GroupId;

    vSetAddress(&sAddress, bBroadcast);

    eZCL_Status = eCLD_GroupsCommandViewGroupRequestSend(
                            sDeviceTable.asDeviceRecords[0].u8Endpoint,
                            sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint, /*DstEP*/
                            &sAddress,
                            &u8SeqNum,
                            &sViewGroupReqPayload);

    DBG_vPrintf(TRACE_APP,"\neCLD_GroupsCommandViewGroupRequestSend  eZCL_Status = %d  \n",eZCL_Status);
}

/****************************************************************************
 *
 * NAME: vAppChangeChannel
 *
 * DESCRIPTION: This function change the channel randomly to one of the other primaries
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vAppChangeChannel( void)
{
    ZPS_tsAplZdpMgmtNwkUpdateReq sZdpMgmtNwkUpdateReq;
    PDUM_thAPduInstance hAPduInst;
    ZPS_tuAddress uDstAddr;
    uint8 u8Seq;
    uint8 u8Min=4, u8Max=7; // as Primary Channel Set is an array with 11,11,11,11,11,15,20,25
    uint8 u8CurrentChannel, u8RandomNum;

    hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZCL);
    if (hAPduInst != NULL)
    {
        sZdpMgmtNwkUpdateReq.u8ScanDuration = 0xfe;

        u8CurrentChannel = ZPS_u8AplZdoGetRadioChannel();
        u8RandomNum = RND_u32GetRand(u8Min,u8Max);
        if(u8CurrentChannel != au8ZLLChannelSet[u8RandomNum])
        {
            sZdpMgmtNwkUpdateReq.u32ScanChannels = (1<<au8ZLLChannelSet[u8RandomNum]);
        }
        else // Increment the channel by one rather than spending in RND_u32GetRand
        {
            // For roll over situation
            if(u8RandomNum == u8Max)
            {
                sZdpMgmtNwkUpdateReq.u32ScanChannels = (1<<au8ZLLChannelSet[u8Min]);
            }
            else
            {
                sZdpMgmtNwkUpdateReq.u32ScanChannels = (1<<au8ZLLChannelSet[u8RandomNum+1]);
            }
        }

        sZdpMgmtNwkUpdateReq.u8NwkUpdateId = ZPS_psAplZdoGetNib()->sPersist.u8UpdateId + 1;
        uDstAddr.u16Addr = 0xfffd;

        if ( 0 == ZPS_eAplZdpMgmtNwkUpdateRequest( hAPduInst,
                                         uDstAddr,
                                         FALSE,
                                         &u8Seq,
                                         &sZdpMgmtNwkUpdateReq))
        {
            DBG_vPrintf(TRACE_REMOTE_NODE, "update Id\n");
            /* should really be in stack?? */
            ZPS_psAplZdoGetNib()->sPersist.u8UpdateId++;
        }
    }

}

/****************************************************************************
 *
 * NAME: vStopAllTimers
 *
 * DESCRIPTION:
 * Stops all the timers
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vStopAllTimers(void)
{
    OS_eStopSWTimer(APP_LedBlinkTimer);
    OS_eStopSWTimer(APP_PollTimer);
    OS_eStopSWTimer(APP_AddGroupTimer);
    OS_eStopSWTimer(APP_IdTimer);
    OS_eStopSWTimer(APP_CommissionTimer);
}
/****************************************************************************
 *
 * NAME: vStartUpHW
 *
 * DESCRIPTION:
 * sets up the hardware after sleep
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vStartUpHW(void)
{

    uint8 u8Status;


    /* Restart the keyboard scanning timer as we've come up through */
    /* warm start via the Power Manager if we get here              */

    vConfigureScanTimer();

     DBG_vPrintf(TRACE_SLEEP, "\nWoken: start poll timer,");
     u8Status = ZPS_eAplZdoPoll();
     DBG_vPrintf(TRACE_SLEEP, " Wake poll %02x\n", u8Status);
     OS_eStartSWTimer(APP_PollTimer, APP_TIME_MS(200), NULL);


}



/****************************************************************************
 *
 * NAME: vWakeCallBack
 *
 * DESCRIPTION:
 * Wake up call back called upon wake up by the schedule activity event.
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vWakeCallBack(void)
{
    /*Decrement the deepsleep count so that if there is no activity for
     * DEEP_SLEEPTIME then the module is put to deep sleep.
     * */
    if (0 < u8DeepSleepTime) {
#if NEVER_DEEP_SLEEP
        u8DeepSleepTime = DEEP_SLEEPTIME;
#else
        u8DeepSleepTime--;
#endif
    }
    vConfigureScanTimer();
    bAddrMode = TRUE;
}

/****************************************************************************
 *
 * NAME: u8SearchEndpointTable
 *
 * DESCRIPTION: searches the light data base to see if a light is already present,
 * or finds a free location if one exists, sets the point to the location as required
 *
 * RETURNS: 0, not present free slot found pointer valid
 *          1, already present, pointer valid
 *          3, table full, pointer not valid
 * uint8
 *
 ****************************************************************************/
PRIVATE uint8 u8SearchEndpointTable(APP_tsEventTouchLink *psEndpointData, uint8* pu8Index) {
    int i;
    bool bGotFree = FALSE;
    *pu8Index = 0xff;

    for (i=0; i<NUM_ENDPOINT_RECORDS; i++) {
        if ((psEndpointData->u16NwkAddr == sEndpointTable.asEndpointRecords[i].u16NwkAddr) &&
                (psEndpointData->u8Endpoint == sEndpointTable.asEndpointRecords[i].u8Endpoint)) {
            /* same ep on same device already known about */
            *pu8Index = i;
            DBG_vPrintf(TRACE_REMOTE_NODE, "\nPresent");
            return 1;
        }
        if ((sEndpointTable.asEndpointRecords[i].u16NwkAddr == 0) && !bGotFree) {
            *pu8Index = i;
            bGotFree = TRUE;
            DBG_vPrintf(TRACE_REMOTE_NODE, "\nFree slot %d", *pu8Index);
        }

    }

    DBG_vPrintf(TRACE_REMOTE_NODE, "\nNot found");
    return (bGotFree)? 0: 3  ;
}

/****************************************************************************
 *
 * NAME: vRemoveLight
 *
 * DESCRIPTION: remove the light with the specified address from the light data base
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vRemoveLight(uint16 u16Addr)
{
    uint32 i,j;

    for ( i=0; i<sEndpointTable.u8NumRecords; i++ )
    {
        if (sEndpointTable.asEndpointRecords[i].u16NwkAddr == u16Addr )
        {
            if ( i < NUM_ENDPOINT_RECORDS-1 )
            {
                /* not last record */
                for (j=i; j<NUM_ENDPOINT_RECORDS-1; j++ )
                {
                    sEndpointTable.asEndpointRecords[j] = sEndpointTable.asEndpointRecords[j+1];
                    sEndpointTable.au8PingCount[j] = sEndpointTable.au8PingCount[j+1];
                }
            }

            /* clear the last recod */
            sEndpointTable.u8NumRecords--;
            memset( &sEndpointTable.asEndpointRecords[sEndpointTable.u8NumRecords], 0, sizeof( tsZllEndpointInfoRecord) );
            sEndpointTable.au8PingCount[sEndpointTable.u8NumRecords] = 0;
            sEndpointTable.u8CurrentLight = 0;    // reset
            PDM_eSaveRecordData(PDM_ID_APP_END_P_TABLE, &sEndpointTable,
                                sizeof(tsZllEndpointInfoTable));

            DBG_vPrintf(TRACE_LIGHT_AGE, "Num Records %d\n", sEndpointTable.u8NumRecords);
            break;
        }
    }
}

/****************************************************************************
 *
 * NAME: vSelectLight
 *
 * DESCRIPTION: selects the next light in the data base for unicast
 * a read attribute request will be sent to it,
 * it the light responds to that then the identify command will then be sent
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSelectLight(void)
{
    uint32 i;

    if ( sEndpointTable.au8PingCount[sEndpointTable.u8CurrentLight] >= LIGHT_DEVICE_AGE_LIMIT ) {
        DBG_vPrintf(TRACE_LIGHT_AGE, "Remove %04x at idx %d\n",
                sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr,
                sEndpointTable.u8CurrentLight);
        for ( i=sEndpointTable.u8CurrentLight; i<sEndpointTable.u8NumRecords; i++ ) {
            if ( i < NUM_ENDPOINT_RECORDS-1 )
            {
                sEndpointTable.asEndpointRecords[i] = sEndpointTable.asEndpointRecords[i+1];
                sEndpointTable.au8PingCount[i] = sEndpointTable.au8PingCount[i+1];
            }
        }
        /* clear the last recod */
        sEndpointTable.u8NumRecords--;
        memset( &sEndpointTable.asEndpointRecords[sEndpointTable.u8NumRecords], 0, sizeof( tsZllEndpointInfoRecord) );
        sEndpointTable.au8PingCount[sEndpointTable.u8NumRecords] = 0;

        PDM_eSaveRecordData(PDM_ID_APP_END_P_TABLE, &sEndpointTable,
                                    sizeof(tsZllEndpointInfoTable));

        DBG_vPrintf(TRACE_LIGHT_AGE, "Num Records %d\n", sEndpointTable.u8NumRecords);

#if TRACE_LIGHT_AGE
        for (i=0; i<NUM_ENDPOINT_RECORDS; i++) {
            DBG_vPrintf(TRACE_LIGHT_AGE, "Idx %d, Addr %04x Profile %04x\n", i,
                    sEndpointTable.asEndpointRecords[i].u16NwkAddr,
                    sEndpointTable.asEndpointRecords[i].u16ProfileId);
        }
#endif
    }

    if (sEndpointTable.u8NumRecords == 0) {
        /* table is empty, nothing to identify */
        return;
    }


    i = sEndpointTable.u8CurrentLight;

    do {
        /*
         * loop through the table till new device found
         * or full table has been searched
         */
        sEndpointTable.u8CurrentLight++;
        if (sEndpointTable.u8CurrentLight >= sEndpointTable.u8NumRecords) {
            sEndpointTable.u8CurrentLight = 0;
        }
    } while( (sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr == 0) &&
             ( i == sEndpointTable.u8CurrentLight ) );

    sEndpointTable.au8PingCount[sEndpointTable.u8CurrentLight]++;
    DBG_vPrintf(TRACE_LIGHT_AGE, "Ping Count for %04x is now %d\n", sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr,
            sEndpointTable.au8PingCount[sEndpointTable.u8CurrentLight]);


    if (sEndpointTable.au8PingCount[sEndpointTable.u8CurrentLight] == LIGHT_DEVICE_NO_ROUTE) {
        void * thisNet = ZPS_pvAplZdoGetNwkHandle();
        ZPS_tsNwkNib * thisNib = ZPS_psNwkNibGetHandle(thisNet);
        /* send network status of no route
         * will trigger route disovery next time
         */

        ZPS_vNwkSendNwkStatusCommand( ZPS_pvAplZdoGetNwkHandle(),
                                      sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr,
                                      thisNib->sTbl.psNtActv[0].u16NwkAddr,
                                      0,
                                      1);

#if LIGHT_DEVICE_AGING==FALSE
        sEndpointTable.au8PingCount[sEndpointTable.u8CurrentLight] = 0;
#endif

    } else {

        DBG_vPrintf(TRACE_REMOTE_NODE|TRACE_LIGHT_AGE, "\nId %04x", sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr);

        uint8 u8Seq;
        tsZCL_Address sAddress;
        uint16 au16AttribList[] = {0x0000};             /* ZCL version attribute */


        /* ensure we are polling */
        vStartPolling();

        sAddress.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
        sAddress.uAddress.u16DestinationAddress = sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr;

        eZCL_SendReadAttributesRequest(
                                sDeviceTable.asDeviceRecords[0].u8Endpoint,
                                sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u8Endpoint,
                                GENERAL_CLUSTER_ID_BASIC,                     // basic
                                FALSE,                     /* bDirectionIsServerToClient,*/
                                &sAddress,
                                &u8Seq,
                                1,                       /*u8NumberOfAttributesInRequest,*/
                                FALSE,                     /* bIsManufacturerSpecific,*/
                                0,                      /*u16ManufacturerCode,*/
                                au16AttribList);
    }


}


/****************************************************************************
 *
 * NAME: bAddToEndpointTable
 *
 * DESCRIPTION: following completion of touch link with a light
 * the lights details are added to the light data base
 *
 * RETURNS:
 * bool
 *
 ****************************************************************************/
PUBLIC bool bAddToEndpointTable(APP_tsEventTouchLink *psEndpointData) {
    uint8 u8Index = 0xff;
    bool_t bPresent;

    bPresent = u8SearchEndpointTable(psEndpointData, &u8Index);
    if (u8Index < 0xff)
    {
        /* There is space for a new entry
         * or it is already there
         */
        if (!bPresent) {
            /* new entry, increment device count
             *
             */
            sEndpointTable.u8NumRecords++;
        }
        /* Add or update details at the slot indicated
         *
         */
        sEndpointTable.asEndpointRecords[u8Index].u16NwkAddr = psEndpointData->u16NwkAddr;
        sEndpointTable.asEndpointRecords[u8Index].u16ProfileId = psEndpointData->u16ProfileID;
        sEndpointTable.asEndpointRecords[u8Index].u16DeviceId = psEndpointData->u16DeviceID;
        sEndpointTable.asEndpointRecords[u8Index].u8Endpoint = psEndpointData->u8Endpoint;
        sEndpointTable.asEndpointRecords[u8Index].u8Version = psEndpointData->u8Version;
        sEndpointTable.au8PingCount[u8Index] = 0;
        DBG_vPrintf(TRACE_REMOTE_NODE, "\nAdd idx %d Addr %04x Ep %d Dev %04x", u8Index,
                sEndpointTable.asEndpointRecords[u8Index].u16NwkAddr,
                sEndpointTable.asEndpointRecords[u8Index].u8Endpoint,
                sEndpointTable.asEndpointRecords[u8Index].u16DeviceId);

        sEndpointTable.u8CurrentLight = u8Index;
#if TRACE_LIGHT_AGE
        DBG_vPrintf(TRACE_LIGHT_AGE, "Num Records %d\n", sEndpointTable.u8NumRecords);
        for (u8Index=0; u8Index<NUM_ENDPOINT_RECORDS; u8Index++) {
            DBG_vPrintf(TRACE_LIGHT_AGE, "Idx %d, Addr %04x Profile %04x\n", u8Index,
                    sEndpointTable.asEndpointRecords[u8Index].u16NwkAddr,
                    sEndpointTable.asEndpointRecords[u8Index].u16ProfileId);
        }
#endif
        return TRUE;
    }

    /* no room in the table */
    return FALSE;
}


/****************************************************************************
 *
 * NAME: vSetGroupAddress
 *
 * DESCRIPTION: set and save the group addresses for use by the remote control
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSetGroupAddress(uint16 u16GroupStart, uint8 u8NumGroups) {
    int i;

    /* This passes all the required group addresses for the device
     * if the are morethan one sub devices (endpoints) they need
     * sharing amoung the endpoints
     * In this case there is one the 1 Rc endpoint, so all group addresses
     * are for it
     */
    for (i=0; i<NUM_GROUP_RECORDS && i<u8NumGroups; i++) {
        sGroupTable.asGroupRecords[i].u16GroupId = u16GroupStart++;
        sGroupTable.asGroupRecords[i].u8GroupType = 0;
        DBG_vPrintf(TRACE_REMOTE_NODE, "Idx %d Grp %04x\n", i, sGroupTable.asGroupRecords[i].u16GroupId);
    }
    sGroupTable.u8NumRecords = u8NumGroups;

    PDM_eSaveRecordData(PDM_ID_APP_GROUP_TABLE, &sGroupTable, sizeof(tsZllGroupInfoTable));

}


/****************************************************************************
 *
 * NAME: vU16ToString
 *
 * DESCRIPTION: converts a uint16 to an ascii string
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vU16ToString(char * str, uint16 u16Num)
{
    uint8 u8Nybble;
    int i;
    for (i = 12; i >= 0; i -= 4)
    {
        u8Nybble = (uint8)((u16Num >> i) & 0x0f);
        u8Nybble += 0x30;
        if (u8Nybble > 0x39)
            u8Nybble += 7;

        *str = u8Nybble;
        str++;
    }
}

/****************************************************************************
 *
 * NAME: vSetAddress
 *
 * DESCRIPTION: set the appropriate address parameters for the current addressing mode
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSetAddress(tsZCL_Address * psAddress, bool_t bBroadcast) {

    if (bBroadcast) {
        psAddress->eAddressMode = E_ZCL_AM_BROADCAST;
        psAddress->uAddress.eBroadcastMode = ZPS_E_APL_AF_BROADCAST_RX_ON;
    } else if (bAddrMode) {
        psAddress->eAddressMode = E_ZCL_AM_GROUP;
        psAddress->uAddress.u16DestinationAddress = sGroupTable.asGroupRecords[u8GroupId].u16GroupId;
    } else {
        psAddress->eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
        psAddress->uAddress.u16DestinationAddress = sEndpointTable.asEndpointRecords[sEndpointTable.u8CurrentLight].u16NwkAddr;
    }
}

/****************************************************************************
 *
 * NAME: vSetAddressMode
 *
 * DESCRIPTION: toggle between groupcast and unicast
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vSetAddressMode(void)
{
    bAddrMode = !bAddrMode;
#if TRACE_REMOTE_NODE
    if (bAddrMode) {
        DBG_vPrintf(TRACE_REMOTE_NODE, "\nG_CAST\n");
    } else {
        DBG_vPrintf(TRACE_REMOTE_NODE, "\nU_CAST\n");
    }
#endif
}

/****************************************************************************
 *
 * NAME: vSendPermitJoin
 *
 * DESCRIPTION: broadcast a permit join management request, 2 minutes
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendPermitJoin(void) {
    PDUM_thAPduInstance hAPduInst;
    ZPS_tuAddress uDstAddr;
    ZPS_tsAplZdpMgmtPermitJoiningReq sZdpMgmtPermitJoiningReq;

    hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZCL);

    uDstAddr.u16Addr = 0xfffc;                           // a;;p routers

    sZdpMgmtPermitJoiningReq.bTcSignificance = 0;               // doesn't effect trust centre
    sZdpMgmtPermitJoiningReq.u8PermitDuration = 120;            // 2 minutes

    ZPS_eAplZdpMgmtPermitJoiningRequest(
        hAPduInst,
        uDstAddr,
        FALSE,  //bool bExtAddr,
        NULL,   //uint8 *pu8SeqNumber,
        &sZdpMgmtPermitJoiningReq);
}

/****************************************************************************
 *
 * NAME: vFactoryResetRecords
 *
 * DESCRIPTION: reset application and stack to factory new state
 *              preserving the outgoing nwk frame counter
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vFactoryResetRecords( void)
{
    void *pvNwk = ZPS_pvAplZdoGetNwkHandle();
    ZPS_tsNwkNib *psNib = ZPS_psNwkNibGetHandle( pvNwk);
    int i;


    ZPS_vNwkNibClearTables(pvNwk);
    (*(ZPS_tsNwkNibInitialValues *)psNib) = *(psNib->sTbl.psNibDefault);

   /* Clear Security Material Set - will set all 64bit addresses to ZPS_NWK_NULL_EXT_ADDR */
    zps_vNwkSecClearMatSet(pvNwk);

    /* Set default values in Material Set Table */
    for (i = 0; i < psNib->sTblSize.u8SecMatSet; i++)
    {
        psNib->sTbl.psSecMatSet[i].u8KeyType = ZPS_NWK_SEC_KEY_INVALID_KEY;
    }
    psNib->sPersist.u64ExtPanId = ZPS_NWK_NULL_EXT_PAN_ID;
    psNib->sPersist.u16NwkAddr = 0xffff;
    psNib->sPersist.u8ActiveKeySeqNumber = 0xff;
    psNib->sPersist.u16VsParentAddr = 0xffff;

    /* put the old frame counter back */
    psNib->sTbl.u32OutFC = u32OldFrameCtr;

    /* set the aps use pan id */
    ZPS_eAplAibSetApsUseExtendedPanId( 0 );
    ZPS_eAplAibSetApsTrustCenterAddress( 0 );

    sZllState.eNodeState = E_REMOTE_STARTUP;

    sZllState.eState = FACTORY_NEW_REMOTE;
    sZllState.eNodeState = E_REMOTE_STARTUP;
    sZllState.u8MyChannel = ZLL_SKIP_CH1;
    sZllState.u16MyAddr= ZLL_MIN_ADDR;
    sZllState.u16FreeAddrLow = ZLL_MIN_ADDR;
    sZllState.u16FreeAddrHigh = ZLL_MAX_ADDR;
    sZllState.u16FreeGroupLow = ZLL_MIN_GROUP;
    sZllState.u16FreeGroupHigh = ZLL_MAX_GROUP;

    memset(&sEndpointTable, 0 , sizeof(tsZllEndpointInfoTable));
    memset(&sGroupTable, 0, sizeof(tsZllGroupInfoTable));

    ZPS_eAplAibSetApsUseExtendedPanId( 0);

    /* Restore any application data previously saved to flash */
    PDM_eSaveRecordData(PDM_ID_APP_ZLL_CMSSION, &sZllState,sizeof(tsZllRemoteState));
    PDM_eSaveRecordData(PDM_ID_APP_END_P_TABLE, &sEndpointTable, sizeof(tsZllEndpointInfoTable));
    PDM_eSaveRecordData(PDM_ID_APP_GROUP_TABLE, &sGroupTable, sizeof(tsZllGroupInfoTable));
    ZPS_vSaveAllZpsRecords();
}

/****************************************************************************
 *
 * NAME: APP_SleepAndPollTask
 *
 * DESCRIPTION: Manages polling, sleeping and rejoin retries
 *              Acctivated by timer
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_SleepAndPollTask)
{
    uint32 u32Time = APP_TIME_MS(1000);
    if (!bTLinkInProgress)
    {
        DBG_vPrintf(TRACE_REMOTE_NODE|TRACE_REJOIN, "KeepAlive - %d, Deep Sleep - %d fast Poll - %d Rejoin - %d FailedTojoin %d \n",
                u8KeepAliveTime,
                u8DeepSleepTime,
                u16FastPoll,
                u8RejoinAttemptsRemaining,
                bFailedToJoin);
        switch (sZllState.eNodeState)
        {
            case E_REMOTE_WAIT_START:
                DBG_vPrintf(TRACE_REMOTE_NODE|TRACE_REJOIN, "Poll and Sleep: Wait Start \n");
                /* wait to form or to join network, if nothing happens deep sleep */
                u32Time = APP_TIME_MS(1000);
                if (bFailedToJoin)
                {
                    if(u8RejoinAttemptsRemaining == 0)
                    {
                        bFailedToJoin = FALSE;
                        u8RejoinAttemptsRemaining = ZLL_MAX_REJOIN_ATTEMPTS;
                        u8KeepAliveTime = KEEP_ALIVE_FACTORY_NEW;
                        DBG_vPrintf(TRACE_REJOIN, "Wait start: no more retries\n");
                    }
                    else
                    {
                        u8RejoinAttemptsRemaining--;
                        DBG_vPrintf(TRACE_REJOIN, "Wait start: try a rejoin\n");
                        vSetRejoinFilter();
                        ZPS_eAplZdoRejoinNetwork(TRUE);
                    }
                }
                else
                {
                    if(u8KeepAliveTime == 0)
                    {
                        /* waited long enough without forming or joining,
                         * give up and deep sleep to save the  battery
                         */
                        vStopAllTimers();
                        PWRM_vInit(E_AHI_SLEEP_DEEP);
                        bDeepSleep = TRUE;
                        DBG_vPrintf(TRACE_SLEEP, "Wait start: go deep\n");
                        DBG_vPrintf(TRACE_SLEEP,"\n Activity %d\n",PWRM_u16GetActivityCount());
                        return;
                    }
                    else
                    {
                        u8KeepAliveTime--;
                    }
                }
                break;
            case E_REMOTE_NETWORK_DISCOVER:
                /* do nothing, wait for classic discovery and join to finish */
                u32Time = APP_TIME_MS(1000);
                DBG_vPrintf(TRACE_REMOTE_NODE|TRACE_REJOIN, "Poll and Sleep: Network Discovery\n");
                break;
            case E_REMOTE_RUNNING:
                DBG_vPrintf(TRACE_REMOTE_NODE|TRACE_REJOIN, "Poll and Sleep: Running\n");
                if (bFailedToJoin)
                {
                    /* Manage rejoin attempts,then short wait, then deep sleep */
                    u32Time = APP_TIME_MS(1000);
                    if(u8RejoinAttemptsRemaining == 0)
                    {
                        if (u8DeepSleepTime) {
                            u8DeepSleepTime--;
                        }
                        else {
                            vStopAllTimers();
                            DBG_vPrintf(TRACE_REJOIN, "join failed: go deep... %d\n", PWRM_u16GetActivityCount());
                            PWRM_vInit(E_AHI_SLEEP_DEEP);
                            bDeepSleep = TRUE;
                            return;
                        }
                    }
                    else
                    {
                        u8RejoinAttemptsRemaining--;
                        DBG_vPrintf(TRACE_REJOIN, "join failed: try a rejoin\n");
                        vSetRejoinFilter();
                        ZPS_eAplZdoRejoinNetwork(TRUE);
                    }
                }
                else
                {
                    /* Manage polling, then warm sleep, then deep sleep */
                    if(u8KeepAliveTime == 0)
                    {
                        vStopAllTimers();
                        if (u8DeepSleepTime) {
                           PWRM_eScheduleActivity(&sWake, (SLEEP_DURATION_MS * SLEEP_TIMER_TICKS_PER_MS) , vWakeCallBack);
                           DBG_vPrintf(TRACE_SLEEP, "poll task: schedule sleep\n");
                        }
                        else {
                            PWRM_vInit(E_AHI_SLEEP_DEEP);
                            bDeepSleep = TRUE;
                            DBG_vPrintf(TRACE_SLEEP, "poll task: go deep\n");
                        }
                        DBG_vPrintf(TRACE_SLEEP,"Activity %d\n",PWRM_u16GetActivityCount());
                        return;
                    }
                    else
                    {
                       uint8 u8PStatus;
                       u8PStatus = ZPS_eAplZdoPoll();
                       if ( 1 /*u8PStatus*/)
                       {
                           DBG_vPrintf(TRACE_REMOTE_NODE|TRACE_REJOIN, "\nPOLL status %d\n", u8PStatus);
                       }

                       if (u16FastPoll)
                       {
                           u16FastPoll--;
                           u32Time = POLL_TIME_FAST, NULL;
                           if (u16FastPoll == 0)
                           {
                               DBG_vPrintf(TRACE_REMOTE_NODE, "\nStop fast poll");
                           }
                       }
                       else
                       {
                           /* Decrement the keep alive in the normal operation mode
                            * Not in active scann mode or while fast polling
                            */

                           if(0 < u8KeepAliveTime) {
                               u8KeepAliveTime--;
                           }
                           /*Start Poll Timer to continue normal polling */
                           u32Time = APP_TIME_MS(1000);
                       }
                    }
                }
                break;

            default:
                /* shouldb't happen, but... */
                u32Time = APP_TIME_MS(1000);
                break;

        }
    }
    else
    {
        DBG_vPrintf(TRACE_REMOTE_NODE|TRACE_REJOIN, "Sleep and Poll: Touch lnk in progress\n");
    }

    OS_eContinueSWTimer(APP_PollTimer, u32Time, NULL);
}

/****************************************************************************
 *
 * NAME: vHandleIdentifyRequest
 *
 * DESCRIPTION: handle identify request command received by the remote
 * causes the identify blink for the required time
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vHandleIdentifyRequest(uint16 u16Duration)
{
    u16IdTime = u16Duration << 1;
    OS_eStopSWTimer(APP_IdTimer);
    if (u16IdTime == 0)
    {
        APP_vSetLeds(eShiftLevel);
    }
    else
    {
        if (u16Duration == 0xffff)
        {
            u16Duration = 10;
        }
        OS_eStartSWTimer(APP_IdTimer, APP_TIME_MS(500), NULL);
        APP_vSetLeds(E_SHIFT_3);
    }
}

/****************************************************************************
 *
 * NAME: APP_ID_Task
 *
 * DESCRIPTION: Tasks that handles the flashing leds during identfy operation
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_ID_Task)
{
    if (u16IdTime == 0)
    {
        /* interpan id time out, shut down */
        OS_eStopSWTimer(APP_IdTimer);
        APP_vSetLeds(eShiftLevel);
    } else {
        u16IdTime--;
        if (u16IdTime & 0x01)
        {
            APP_vSetLeds(E_SHIFT_0);
        }
        else
        {
            APP_vSetLeds(E_SHIFT_3);
        }
        OS_eContinueSWTimer(APP_IdTimer, APP_TIME_MS(500), NULL);
    }
}

/****************************************************************************
 *
 * NAME: vResetSleepAndJoinCounters
 *
 * DESCRIPTION: helper function to reset counters and flags associated
 * with sleep and join
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vResetSleepAndJoinCounters(void)
{
    u8KeepAliveTime = KEEP_ALIVETIME;
    u8DeepSleepTime = DEEP_SLEEPTIME;
    bFailedToJoin = FALSE;
    u8RejoinAttemptsRemaining = ZLL_MAX_REJOIN_ATTEMPTS;
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
