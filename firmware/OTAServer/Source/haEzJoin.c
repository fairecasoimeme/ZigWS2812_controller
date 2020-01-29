/*****************************************************************************
 *
 * MODULE:             JN-AN-1189
 *
 * COMPONENT:          haEzJoin.c
 *
 * DESCRIPTION:        HA EZ mode commissioning (Implementation)
 *
 *****************************************************************************
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
 * Copyright NXP B.V. 2013. All rights reserved
 *
 ****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "zps_apl.h"
#include "zps_apl_aps.h"
#include "haEzJoin.h"
#include "zps_apl_aib.h"
#include "dbg.h"
#include <string.h>
#include "pwrm.h"
#include "app_timer_driver.h"
#include "app_zbp_utilities.h"
#include "os_gen.h"
#include "appZpsBeaconHandler.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define DEBUG_EZMODE
#ifndef DEBUG_EZMODE
#define TRACE_EZMODE FALSE
#else
#define TRACE_EZMODE TRUE
#endif

#define LQI_CHANGE_UNIT 20
#define LQI_MIN_VALUE   80
#define LQI_MAX_VALUE   180

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vEZ_ClearStateVariables(void);
PRIVATE void vEZ_ClearDiscNT(void);

#ifdef SUPPORT_JOIN_ELSE_FORM
PRIVATE void vEZ_ModeFormOrJoinNwk( void );
PRIVATE void vFormingNetwork(ZPS_tsAfEvent *pZPSevent);
PRIVATE void vHandleFormedNwk(void);
PRIVATE void vHandleFailedToStart(void);
#endif

PRIVATE void vEZ_SortAndSaveNetworks(ZPS_tsAfNwkDiscoveryEvent *pNwkDes);
PRIVATE void vEZ_JoinSavedNetwork(void);
//PRIVATE ZPS_tsNwkDiscNtEntry *sEZ_GetDiscEntry(uint64 *pExtendedPanId);
PRIVATE eChannelsSelected eEZ_SetChannel(void);
PRIVATE bool_t bBackOffTimerExpirred(void);

PRIVATE void vBackOff(bool_t bBackOffTimerExpirredFlag);
PRIVATE void vSetUpStart(void);
PRIVATE void vWaitDiscovery(ZPS_tsAfEvent *pZPSevent);
PRIVATE void vJoiningNetwork(ZPS_tsAfEvent *pZPSevent);
PRIVATE void vDeviceInTheNetwork(ZPS_tsAfEvent *pZPSevent,teEZ_JoinAction eJoinAction);

PRIVATE void vReDiscover(void);
PRIVATE void vHandleJoinedNwk(void);
PRIVATE void vHandleJoinFailed(void);
PRIVATE void vHandleDiscovery(ZPS_tsAfEvent *pZPSevent);

//PRIVATE void vInitAssociationFilter( void);
PRIVATE void vSetAssociationFilter( void);


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

PRIVATE tsEZ_Join sEZModeData = { 0};
PRIVATE uint32 u32DefaultChannelMask=0x07fff800;
PRIVATE bool_t bRejoin;
PRIVATE bool_t bUseLastKnownCh;

PRIVATE tsBeaconFilterType sBeaconFilter;
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vEZ_ReJoinOnLastKnownCh
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS:  Name                Usage
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
PUBLIC void vEZ_ReJoinOnLastKnownCh(void)
{
	vEZ_ClearDiscNT();
	bUseLastKnownCh=TRUE;
	ZPS_eAplZdoRejoinNetwork(TRUE);
}

/****************************************************************************
 *
 * NAME: vEZ_RestoreDefaultAIBChMask
 *
 * DESCRIPTION:
 * The function restores the default channel mask from ZPS config.
 *
 * PARAMETERS:  Name                Usage
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
PUBLIC void vEZ_RestoreDefaultAIBChMask(void)
{
	u32DefaultChannelMask = ZPS_psAplAibGetAib()->u32ApsChannelMask;
}
/****************************************************************************
 *
 * NAME: vEZ_SetDefaultAIBChMask
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS:  Name                Usage
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
PUBLIC void vEZ_SetDefaultAIBChMask(void)
{
	void* pvNwk;
	uint8 u8Ch;
    pvNwk = ZPS_pvAplZdoGetNwkHandle();
    u8Ch = ZPS_psNwkNibGetHandle( pvNwk)->sPersist.u8VsChannel;

    if ( (u8Ch >10) && (u8Ch < 27) )
    {
        ZPS_psAplAibGetAib()->u32ApsChannelMask = 1 << u8Ch;
        DBG_vPrintf(TRACE_EZMODE,"Default mask saved = 0x%08x, Mask Set to = 0x%08x\n",u32DefaultChannelMask,ZPS_psAplAibGetAib()->u32ApsChannelMask );
    } else {
        ZPS_psAplAibGetAib()->u32ApsChannelMask = 1 << 11;
        DBG_vPrintf(TRACE_EZMODE,"Default mask saved = 0x%08x, Mask Set to = 0x%08x\n",u32DefaultChannelMask,ZPS_psAplAibGetAib()->u32ApsChannelMask );
    }
	/*Initialise for the first time*/

}
/****************************************************************************
 *
 * NAME: eEZ_GetJoinState
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS:  Name                            Usage
 * None
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
PUBLIC teEZ_State eEZ_GetJoinState(void)
{
	return ((teEZ_State)sEZModeData.u8EZSetUpState);
}
/****************************************************************************
 *
 * NAME: vEZ_EZModeNWKJoinHandler
 *
 * DESCRIPTION:
 * EZ Mode Handle to be from application.
 *
 * PARAMETERS:  Name                            Usage
 * None
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
PUBLIC void vEZ_EZModeNWKJoinHandler(ZPS_tsAfEvent *pZPSevent,teEZ_JoinAction eJoinAction)
{
	bool_t bBackOffTimerExpirredFlag;
	bBackOffTimerExpirredFlag = bBackOffTimerExpirred();

	if( bBackOffTimerExpirredFlag && (sEZModeData.u8EZSetUpState != E_EZ_BACKOFF) )
	{
		/*Just Back Off with a timer for back off timing don't do anything else*/
		sEZModeData.u8EZSetUpState = E_EZ_BACKOFF;
		DBG_vPrintf(TRACE_EZMODE,"Getting into BackOff State\n");
	}
	else
	{
		/*Run The state machine */
		switch (sEZModeData.u8EZSetUpState)
		{
			case E_EZ_START:
			{
				vSetUpStart();
				break;
			}
			case E_EZ_WAIT_DISCOVERY_TIMEOUT:
			{
				vWaitDiscovery(pZPSevent);
				break;
			}
			case E_EZ_JOINING_NETWORK:
			{
				vJoiningNetwork(pZPSevent);
				break;
			}
			case E_EZ_DEVICE_IN_NETWORK:
			{
				vDeviceInTheNetwork(pZPSevent,eJoinAction);
				break;
			}

			case E_EZ_BACKOFF:
			{
				vBackOff(bBackOffTimerExpirredFlag);
				break;
			}
			default :
				break;
		}
	}
}

/****************************************************************************
 *
 * NAME: eEZ_UpdateEZState
 *
 * DESCRIPTION:
 * should be called from application once reloaded from PDM
 *
 * PARAMETERS:  Name                            Usage
 *				teEZ_State               EZ State
 *
 ****************************************************************************/

PUBLIC ZPS_teStatus eEZ_UpdateEZState(teEZ_State eEZState)
{
	ZPS_teStatus eStatus =ZPS_E_SUCCESS;

	sEZModeData.u8EZSetUpState = eEZState;

	DBG_vPrintf(TRACE_EZMODE, "\r\n vEZ_Update EZState state %d",  sEZModeData.u8EZSetUpState);
	if (sEZModeData.u8EZSetUpState == E_EZ_START )
	{
		/*If the State is start then start the Timers*/
		OS_eStartSWTimer(APP_JoinTimer,APP_TIME_MS(3000),NULL);
		OS_eStartSWTimer(APP_BackOffTimer,APP_TIME_MS(60000),NULL);
	}
	else if (sEZModeData.u8EZSetUpState == E_EZ_DEVICE_IN_NETWORK )
	{
		/*Restart The stack*/
		eStatus= ZPS_eAplZdoStartStack();
	}
	return eStatus;
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vClearDiscNT
 *
 * DESCRIPTION:
 * Resets the discovery NT
 *
 * RETURNS:
 * void
 *
 *
 ****************************************************************************/
PRIVATE void vEZ_ClearDiscNT(void)
{
	ZPS_tsNwkNib * thisNib;

	DBG_vPrintf(TRACE_EZMODE, "vEZ_ClearDiscNT done \r\n");

	thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());

	memset((uint8*)thisNib->sTbl.psNtDisc, 0,
			sizeof(ZPS_tsNwkDiscNtEntry) * thisNib->sTblSize.u8NtDisc);
}

/****************************************************************************
 *
 * NAME: vSetAssociationFilter
 *
 * DESCRIPTION:
 * Sets Association filters
 * The LQi value is set at MAX and on retry the values is reduced gradually
 * upto MIN value.
 * And it loops back from max to min
 *
 * RETURNS:
 * void
 *
 *
 ****************************************************************************/
PRIVATE void vSetAssociationFilter( void)
{

    sBeaconFilter.u8Lqi = 40;
    sBeaconFilter.pu64ExtendPanIdList = NULL;
    sBeaconFilter.u8ListSize = 0;
    sBeaconFilter.u8FilterMap = BF_BITMAP_PERMIT_JOIN | BF_BITMAP_LQI | BF_BITMAP_CAP_ROUTER;
    ZPS_bAppAddBeaconFilter( &sBeaconFilter);

}

/****************************************************************************
 *
 * NAME: vAttemptDiscovery
 *
 * DESCRIPTION:
 * Attempts discovery of the network with a given channel mask
 *
 * RETURNS:
 * void
 *
 *
 ****************************************************************************/
PRIVATE void vAttemptDiscovery(void)
{
	uint8 eStatus;
	teEZ_State EZ_State;
	uint16 u16TimeOut;

	sEZModeData.u8ScanAttempts++;
	vEZ_ClearDiscNT();

    if(bRejoin)
    {
    	if(bUseLastKnownCh)
    	{
    		bUseLastKnownCh= FALSE;
    		vEZ_SetDefaultAIBChMask();
    	}
		eStatus = ZPS_eAplZdoRejoinNetwork(TRUE);
		EZ_State = E_EZ_JOINING_NETWORK;
		u16TimeOut = JOINING_TIMEOUT_IN_MS;
    }
    else
    {
    	//Apply Association Filter
        vSetAssociationFilter();
        ZPS_vNwkNibClearDiscoveryNT(ZPS_pvAplZdoGetNwkHandle());
		eStatus = ZPS_eAplZdoDiscoverNetworks(  ZPS_psAplAibGetAib()->u32ApsChannelMask );
		EZ_State = E_EZ_WAIT_DISCOVERY_TIMEOUT;
		u16TimeOut = DISCOVERY_TIMEOUT_IN_MS;
    }

	DBG_vPrintf(TRACE_EZMODE,"vAttemptDiscovery :- Discovery API Status =%d\n",eStatus);
	vStartStopTimer(APP_JoinTimer,APP_TIME_MS(u16TimeOut),&(sEZModeData.u8EZSetUpState),EZ_State);
}

/****************************************************************************
 *
 * NAME: vSetUpStart
 *
 * DESCRIPTION:
 * Starts the set up
 *
 * RETURNS:
 * void
 *
 *
 ****************************************************************************/
PRIVATE void vSetUpStart(void)
{

	if(OS_eGetSWTimerStatus(APP_JoinTimer) == OS_E_SWTIMER_EXPIRED)
	{
		vEZ_ClearStateVariables(); /*Clear All the variables related to set up*/
		eEZ_SetChannel();		   /*Set Appropriate Channel in APS channel Mask*/
		/*  attempt to discover */
		vAttemptDiscovery();
	}
}
/****************************************************************************
 *
 * NAME: vDeviceInTheNetwork
 *
 * DESCRIPTION:
 * State when the device is already in the NWK
 *
 * RETURNS:
 * void
 *
 *
 ****************************************************************************/
PRIVATE void vDeviceInTheNetwork(ZPS_tsAfEvent *pZPSevent,teEZ_JoinAction eJoinAction)
{
	bRejoin = (bool_t)eJoinAction;
	if(bRejoin)
	{
		vStartStopTimer(APP_JoinTimer,APP_TIME_MS(500),&(sEZModeData.u8EZSetUpState),E_EZ_START);
	}
}
/****************************************************************************
 *
 * NAME: vWaitDiscovery
 *
 * DESCRIPTION:
 * Waits after a discovery was issued, so if the time out happens
 *
 * RETURNS:
 * void
 *
 *
 ****************************************************************************/
PRIVATE void vWaitDiscovery(ZPS_tsAfEvent *pZPSevent)
{

	if(OS_eGetSWTimerStatus(APP_JoinTimer) == OS_E_SWTIMER_EXPIRED)
	{
		vReDiscover();
	}
	else
	{
		if (pZPSevent->eType != ZPS_EVENT_NONE)
		{
		    switch(pZPSevent->eType)
			{
			    case(ZPS_EVENT_NWK_DISCOVERY_COMPLETE):
				    vHandleDiscovery(pZPSevent);
				    break;

			    case(ZPS_EVENT_NWK_JOINED_AS_ROUTER):     /* fall through*/
				case(ZPS_EVENT_NWK_JOINED_AS_ENDDEVICE):
			        vHandleJoinedNwk();
					break;

				case(ZPS_EVENT_NWK_FAILED_TO_JOIN):
					vReDiscover();
					break;

				default:
					break;
			}
		}
	}
}
/****************************************************************************
 *
 * NAME: vBackOff
 *
 * DESCRIPTION:
 * BackOff state, where the device will not do anything but wait before it starts
 * to attempt joining
 *
 * RETURNS:
 * void
 *
 *
 ****************************************************************************/
PRIVATE void vBackOff(bool_t bBackOffTimerExpirredFlag )
{
	if(bBackOffTimerExpirredFlag)
	{
		/*Come back to Start Up state*/
		vStartStopTimer( APP_JoinTimer, APP_TIME_MS(RESTART_TIME_IN_MS),&(sEZModeData.u8EZSetUpState),E_EZ_START );
		DBG_vPrintf(TRACE_EZMODE,"BackOff State -> Start State\n");
	}
}

/****************************************************************************
 *
 * NAME: bBackOffTimerExpirred
 *
 * DESCRIPTION:
 * Returns if the back up time is over and maintains the back off timing
 *
 * RETURNS:
 * TRUE if back off time is over, else FALSE
 *
 *
 ****************************************************************************/
PRIVATE bool_t bBackOffTimerExpirred(void)
{
	static uint32 u32BackOffTime;
	if(OS_eGetSWTimerStatus(APP_BackOffTimer) == OS_E_SWTIMER_EXPIRED)
	{
		OS_eStopSWTimer(APP_BackOffTimer);
		OS_eStartSWTimer(APP_BackOffTimer, APP_TIME_MS(BACKOFF_TIMEOUT_IN_MS), NULL );
		/*Increament by 1 minute */
		u32BackOffTime++;
		DBG_vPrintf(TRACE_EZMODE,"Time in Minute  = %d\n",u32BackOffTime);

		/*15 Minutes past - Change state*/
		if(u32BackOffTime >= BACKOFF_TIME_IN_MINUTES)
		{
			u32BackOffTime=0;
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	return FALSE;
}
/****************************************************************************
 *
 * NAME: vJoiningNetwork
 *
 * DESCRIPTION:
 * Joining the NWK state.
 *
 * RETURNS:
 * void
 *
 *
 ****************************************************************************/
PRIVATE void vJoiningNetwork(ZPS_tsAfEvent *pZPSevent)
{

	if(OS_eGetSWTimerStatus(APP_JoinTimer) == OS_E_SWTIMER_EXPIRED)
	{
		DBG_vPrintf(TRACE_EZMODE, "Joining TimeOut -- \n\n\n\n");
		vHandleJoinFailed();
	}
	else
	{
		if (pZPSevent->eType != ZPS_EVENT_NONE)
		{
		    switch(pZPSevent->eType)
			{
				case(ZPS_EVENT_NWK_JOINED_AS_ROUTER):     /* fall through*/
				case(ZPS_EVENT_NWK_JOINED_AS_ENDDEVICE):
		            vHandleJoinedNwk();
					break;

				case(ZPS_EVENT_NWK_FAILED_TO_JOIN):
				    vHandleJoinFailed();
					break;

				default:
					break;
			}
		}
	}
}

/****************************************************************************
 *
 * NAME: vReDiscover
 *
 * DESCRIPTION:
 * Initiates re-discovery, if all the attempts are over switches the state
 *
 * RETURNS:
 * void
 *
 *
 ****************************************************************************/
PRIVATE void vReDiscover(void)
{
	DBG_vPrintf(TRACE_EZMODE, "Discovery FailedtoJoin.....%d\n",sEZModeData.u8ScanAttempts);

	DBG_vPrintf(TRACE_EZMODE, "\n\n\n");
	vStartStopTimer(APP_JoinTimer,APP_TIME_MS(DISCOVERY_TIMEOUT_IN_MS),&(sEZModeData.u8EZSetUpState),E_EZ_START);

}
/****************************************************************************
 *
 * NAME: vEZ_SortAndSaveNetworks
 *
 * DESCRIPTION:
 * This function sorts discovered networks based on LQI and saves it
 *
 * PARAMETERS:  Name                            Usage
 *	None
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
PRIVATE void vEZ_SortAndSaveNetworks(ZPS_tsAfNwkDiscoveryEvent *pNwkDes)
{
	uint8 i=0;
	//uint8 j=0;

	//ZPS_tsNwkNetworkDescr sTempDesc;
	//ZPS_tsNwkDiscNtEntry *pTempDiscEntry1;
	//ZPS_tsNwkDiscNtEntry *pTempDiscEntry2;

	sEZModeData.u8JoinIndex = 0;
//	sEZModeData.u8DiscoveredNwkCount = 0;
	sEZModeData.u8DiscoveredNwkCount = pNwkDes->u8NetworkCount;
	DBG_vPrintf(TRACE_EZMODE, "vEZ_SortAndSaveNetworks \n");

	for(i=0; (i < EZ_MAX_NETWORK_DESCRIPTOR ) && ( i < pNwkDes->u8NetworkCount); i++)
	{
		DBG_vPrintf(TRACE_EZMODE,"*");
		/* copy only if permit join of network is set - Capacity is available */

			/* copy descriptors */
			memcpy( &sEZModeData.asSavedNetworks[i],
					&pNwkDes->psNwkDescriptors[i],
					sizeof(ZPS_tsNwkNetworkDescr));

			DBG_vPrintf(TRACE_EZMODE, "Saving Open NWK ExPANID = 0x%16llx\n",sEZModeData.asSavedNetworks[i].u64ExtPanId);
	}
#if 0
	DBG_vPrintf(TRACE_EZMODE, "vEZ_SortAndSaveNetworks sEZModeData.u8DiscoveredNwkCount = %d\n",sEZModeData.u8DiscoveredNwkCount);
	for(i=0; i< sEZModeData.u8DiscoveredNwkCount; i++)
	{
		for(j=i+1; j< sEZModeData.u8DiscoveredNwkCount; j++)
		{
			pTempDiscEntry1 = sEZ_GetDiscEntry(&sEZModeData.asSavedNetworks[i].u64ExtPanId);
			pTempDiscEntry2 = sEZ_GetDiscEntry(&sEZModeData.asSavedNetworks[j].u64ExtPanId);

			DBG_vPrintf(TRACE_EZMODE,"\n\n\n");
			DBG_vPrintf(TRACE_EZMODE, "EPID 0x%16llx has LQI %d \n",sEZModeData.asSavedNetworks[i].u64ExtPanId,pTempDiscEntry1->u8LinkQuality);
			DBG_vPrintf(TRACE_EZMODE, "EPID 0x%16llx has LQI %d \n",sEZModeData.asSavedNetworks[j].u64ExtPanId,pTempDiscEntry2->u8LinkQuality);
			if((NULL != pTempDiscEntry1 ) &&(NULL != pTempDiscEntry2))
			{

				if(pTempDiscEntry1->u8LinkQuality < pTempDiscEntry2->u8LinkQuality)
				{
					/* swap*/
					memcpy(&sTempDesc, 						&(sEZModeData.asSavedNetworks[i]),	sizeof(ZPS_tsNwkNetworkDescr) );
					memcpy(&sEZModeData.asSavedNetworks[i],	&(sEZModeData.asSavedNetworks[j]), 	sizeof(ZPS_tsNwkNetworkDescr) );
					memcpy(&sEZModeData.asSavedNetworks[j],	&sTempDesc,							sizeof(ZPS_tsNwkNetworkDescr) );

					DBG_vPrintf(TRACE_EZMODE, "vEZ_SortAndSaveNetworks  sorted pan id %16llx with panid%16llx at indexs %d with index %d\n",
							sEZModeData.asSavedNetworks[j].u64ExtPanId,sEZModeData.asSavedNetworks[i].u64ExtPanId, j,i);
				}
			}
		}
	}
#endif
}
/****************************************************************************
 *
 * NAME:vEZ_JoinSavedNetwork
 *
 * DESCRIPTION:
 * This function joins to network stored at index sEZModeData.u8JoinIndex
 *
 * PARAMETERS:  Name                            Usage
 *	None
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
PRIVATE void vEZ_JoinSavedNetwork(void)
{
	uint8 u8Status;

	if( sEZModeData.u8DiscoveredNwkCount > 0)
	{
		while( sEZModeData.u8JoinIndex < sEZModeData.u8DiscoveredNwkCount  )
		{
			u8Status = ZPS_eAplZdoJoinNetwork(&(sEZModeData.asSavedNetworks[sEZModeData.u8JoinIndex]));
			sEZModeData.u8JoinIndex++;
			if(ZPS_E_SUCCESS != u8Status )
			{
				/* This should not happen */
				DBG_vPrintf(TRACE_EZMODE, "API failed in vEZ_JoinSavedNetwork %d\n",u8Status);
			}
			else
			{
				DBG_vPrintf(TRACE_EZMODE, "Join Network API Success \n");
				vStartStopTimer( APP_JoinTimer, APP_TIME_MS(JOINING_TIMEOUT_IN_MS), &(sEZModeData.u8EZSetUpState),E_EZ_JOINING_NETWORK );
				return ;
			}
		}
		if (sEZModeData.u8JoinIndex >= sEZModeData.u8DiscoveredNwkCount)
		{
			DBG_vPrintf(TRACE_EZMODE, "Exhausted NWK Join\n\n\n");
			vStartStopTimer( APP_JoinTimer, APP_TIME_MS(RESTART_TIME_IN_MS),&(sEZModeData.u8EZSetUpState),E_EZ_START );
		}
	}
	else
	{
		DBG_vPrintf(TRACE_EZMODE, "NO Open NWK\n\n\n");
		vStartStopTimer( APP_JoinTimer, APP_TIME_MS(RESTART_TIME_IN_MS),&(sEZModeData.u8EZSetUpState),E_EZ_START );
	}
}
#if 0
/****************************************************************************
 *
 * NAME: sEZ_GetDiscEntry
 *
 * DESCRIPTION:
 * This function retrieves discovery table entry with given extended pan id
 *
 * PARAMETERS:  Name                            Usage
 *	None
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
PRIVATE ZPS_tsNwkDiscNtEntry *sEZ_GetDiscEntry(uint64 *pExtendedPanId)
{
	uint8 i;
	ZPS_tsNwkNib *thisNib;
//	ZPS_tsNwkDiscNtEntry *pDiscEntry;
	thisNib = ZPS_psNwkNibGetHandle(ZPS_pvAplZdoGetNwkHandle());
//	pDiscEntry =thisNib->sTbl.psNtDisc;
	for( i = 0; i < thisNib->sTblSize.u8NtDisc; i++)
	{
		if(thisNib->sTbl.psNtDisc[i].u64ExtPanId == *pExtendedPanId )
		{
			return(&thisNib->sTbl.psNtDisc[i]);
		}
	}
	return NULL;
}
#endif

/****************************************************************************
 *
 * NAME: eEZ_SetChannel
 *
 * DESCRIPTION:
 * This function sets the primary channels and other channels alternatively
 *
 * PARAMETERS:  Name                            Usage
 *	None
 *
 * RETURNS:
 * None
 *
 ****************************************************************************/
PRIVATE eChannelsSelected eEZ_SetChannel(void)
{
	/* Full mask in  primary->secondary order */
	const uint8 u8DiscChannels[] = { 11, 15, 20, 25, 12, 13, 14, 16, 17, 18, 19, 21, 22, 23, 24, 26 };
	static uint8 u8ChannelIndex = 0;
	bool bChannelSetOK = FALSE;

	DBG_vPrintf(TRACE_EZMODE, "eEZ_SetChannel\n");

	/* Loop until channel is set */
	/* ASSERT, at least 1 channel must be set in the user defined channel mask */
	while( !bChannelSetOK )
	{

		/* set a single channel from the primary/secondary selected by user */
		if( (u32DefaultChannelMask) & (1 << u8DiscChannels[u8ChannelIndex]))
		{
			ZPS_psAplAibGetAib()->u32ApsChannelMask = 1 << u8DiscChannels[u8ChannelIndex];

			bChannelSetOK = TRUE;
			DBG_vPrintf(TRACE_EZMODE, "\n\nSet Channel to %d\n",u8DiscChannels[u8ChannelIndex]);
		}
		if( u8ChannelIndex < sizeof(u8DiscChannels))
		{
			u8ChannelIndex++;
		}
		else
		{
			u8ChannelIndex = 0;
		}
	}

	return E_EZ_SCAN_HALF_CHANNEL_COMPLETED;
}

/****************************************************************************
 *
 * NAME: vHandleDiscovery
 *
 * DESCRIPTION:
 * Handles discovery during the joining
 *
 * PARAMETERS:  Name                            Usage
 *				ZPS_tsAfEvent *pZPSevent        Stack event
 *
 * RETURNS:
 *
 *
 ****************************************************************************/
PRIVATE void vHandleDiscovery(ZPS_tsAfEvent *pZPSevent)
{
	DBG_vPrintf(TRACE_EZMODE, "ZPS_EVENT_NWK_DISCOVERY_COMPLETE......");
	if( ( ZPS_E_SUCCESS == pZPSevent->uEvent.sNwkDiscoveryEvent.eStatus) ||
	        (ZPS_NWK_ENUM_NEIGHBOR_TABLE_FULL == pZPSevent->uEvent.sNwkDiscoveryEvent.eStatus) )
	{
	    DBG_vPrintf(TRACE_EZMODE, "NWK Found, Count = %d  \n",pZPSevent->uEvent.sNwkDiscoveryEvent.u8NetworkCount);
	    vEZ_SortAndSaveNetworks(&(pZPSevent->uEvent.sNwkDiscoveryEvent));
	    vEZ_JoinSavedNetwork();
	}
	else
	{
		DBG_vPrintf(TRACE_EZMODE, "Discovery ERR or No NWK\n");
	    vStartStopTimer(APP_JoinTimer,
	                    APP_TIME_MS(DISCOVERY_TIMEOUT_IN_MS),
	                    &(sEZModeData.u8EZSetUpState),
	                    E_EZ_WAIT_DISCOVERY_TIMEOUT);
	}
}

/****************************************************************************
 *
 * NAME: vHandleJoinedNwk
 *
 * DESCRIPTION:
 * Handles the Device joined state, the state needs to be persisted here
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 *
 ****************************************************************************/
PRIVATE void vHandleJoinedNwk(void)
{

	OS_eStopSWTimer(APP_JoinTimer);
	OS_eStopSWTimer(APP_BackOffTimer);
	/*Rejoin is cleared now */
	bRejoin=FALSE;
	/* Now device part of network. Update state variable in persistent storage */
	sEZModeData.u8EZSetUpState = E_EZ_DEVICE_IN_NETWORK;

	/* Store EPID for future rejoin */
	ZPS_eAplAibSetApsUseExtendedPanId(ZPS_u64NwkNibGetEpid( ZPS_pvAplZdoGetNwkHandle()));
#if (defined OLD_TOOLCHAIN)
	ZPS_bNwkNibAddrMapAddEntry(ZPS_pvAplZdoGetNwkHandle(),
			0x0000, ZPS_psAplAibGetAib()->u64ApsTrustCenterAddress);
#else
	 ZPS_u16AplZdoLookupAddr(ZPS_psAplAibGetAib()->u64ApsTrustCenterAddress);
#endif
}

/****************************************************************************
 *
 * NAME: vHandleJoinFailed
 *
 * DESCRIPTION:
 * Handles the Device join failure state
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
 *
 ****************************************************************************/
PRIVATE void vHandleJoinFailed(void)
{
	if(bRejoin)
	{
		vStartStopTimer( APP_JoinTimer, APP_TIME_MS(1000),&(sEZModeData.u8EZSetUpState),E_EZ_START );
	}
	else
		vEZ_JoinSavedNetwork();
}

/****************************************************************************
 **
 ** NAME:       vEZ_ClearStateVariables
 **
 ** DESCRIPTION:
 ** Function to reset variables
 **
 ** PARAMETERS:                 Name               Usage
 ** tsZCL_CallBackEvent        *psCallBackEvent    Timer Server Callback
 **
 ** RETURN:
 ** None
 **
 ****************************************************************************/
PRIVATE void vEZ_ClearStateVariables(void)
{
	DBG_vPrintf(TRACE_EZMODE, "vEZ_ClearStateVariables\r\n");
	sEZModeData.u8ScanAttempts = 0;
	sEZModeData.u8ScanDurationInSec = 0;
	sEZModeData.u8IsPrimaryChannelsScanned = 0;
	sEZModeData.u8SetUpTime = 0;
	sEZModeData.u8JoinIndex = 0;
	sEZModeData.u8DiscoveredNwkCount = 0;
	memset(sEZModeData.asSavedNetworks,0,sizeof(ZPS_tsNwkNetworkDescr)*EZ_MAX_NETWORK_DESCRIPTOR);
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

