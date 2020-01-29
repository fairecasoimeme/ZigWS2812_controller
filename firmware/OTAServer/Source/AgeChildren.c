/*****************************************************************************
 *
 * MODULE:             JN-AN-1189
 *
 * COMPONENT:          AgeChildren.c
 *
 * DESCRIPTION:        Child aging code to remove stale child entries
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

/* Stack Includes */
#include <jendefs.h>
#include "dbg.h"
#include "os.h"
#include "os_gen.h"
#include "pdum_apl.h"
#include "pdum_gen.h"
#include "pdm.h"
#include "zps_apl_af.h"
#include "zps_apl_zdp.h"
#include "zps_apl_aib.h"
#include "zps_nwk_nib.h"
#include "zps_nwk_pub.h"
#include "app_timer_driver.h"
#include "AgeChildren.h"
#include "Utilities.h"
#include "appZdpExtraction.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef TRACE_AGE_CHILDREN
#define TRACE_AGE_CHILDREN FALSE
#endif


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/


/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/




/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

PRIVATE uint8	u8ChildOfInterest = 0;

/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/


/****************************************************************************/
/***		Tasks														  ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: APP_AgeOutChildren
 *
 * DESCRIPTION:
 * Cycles through the device's children on a context restore to find out if
 * they now have a different parent
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_TASK(APP_AgeOutChildren)
{
    void * thisNet = ZPS_pvAplZdoGetNwkHandle();
	ZPS_tsNwkNib * thisNib = ZPS_psNwkNibGetHandle(thisNet);
	uint8 i;
	uint8 u8TransactionSequenceNumber;
	ZPS_tuAddress uAddress;

	/* u8ChildOfInterest stored as global so loop starts next location */
	for(i = u8ChildOfInterest ; i < thisNib->sTblSize.u16NtActv ; i++)
	{
		DBG_vPrintf(TRACE_AGE_CHILDREN, "Checking Index: %d \n", i);

		if(thisNib->sTbl.psNtActv[i].u16NwkAddr != 0xFFFE )
		{

			if ( (ZPS_NWK_NT_AP_RELATIONSHIP_CHILD == thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u2Relationship) &&
					(ZPS_NWK_NT_AP_DEVICE_TYPE_ZED == thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1DeviceType)) /* Aging is for the END DEVICE as a child */
			{
				/* Child found in the neighbour table, send out a address request */
				u8ChildOfInterest = i;

				PDUM_thAPduInstance hAPduInst;
				hAPduInst = PDUM_hAPduAllocateAPduInstance(apduZDP);

				if (hAPduInst == PDUM_INVALID_HANDLE)
				{
					DBG_vPrintf(TRACE_AGE_CHILDREN, "IEEE Address Request - PDUM_INVALID_HANDLE\n");
				}
				else
				{
					uAddress.u16Addr = 0xFFFD;

					ZPS_tsAplZdpNwkAddrReq sAplZdpNwkAddrReq;
					sAplZdpNwkAddrReq.u64IeeeAddr = ZPS_u64NwkNibGetMappedIeeeAddr(thisNet,thisNib->sTbl.psNtActv[u8ChildOfInterest].u16Lookup);
					sAplZdpNwkAddrReq.u8RequestType = 0;

					DBG_vPrintf(TRACE_AGE_CHILDREN, "Child found in NT, sending addr request: 0x%04x\n", thisNib->sTbl.psNtActv[u8ChildOfInterest].u16NwkAddr);
					ZPS_teStatus eStatus = ZPS_eAplZdpNwkAddrRequest(	hAPduInst,
																		uAddress,
																		FALSE,
																		&u8TransactionSequenceNumber,
																		&sAplZdpNwkAddrReq
																		);
					if (eStatus)
					{
						DBG_vPrintf(TRACE_AGE_CHILDREN, "Address Request failed: 0x%02x\n", eStatus);
					}
					else
					{
						/* only increment the  counter on success, else try again nect timer expiry */
						u8ChildOfInterest++;
						break;
					}
				}
			}
		}

	}

	/* Table scan complete, stop the timer and clear down */
	if (i >= thisNib->sTblSize.u16NtActv)
	{
		OS_eStopSWTimer(APP_AgeOutChildrenTmr);
		u8ChildOfInterest = 0;
	}
	/* Start the timer for the next scan of the neighbour table */
	else
	{
		OS_eStartSWTimer(APP_AgeOutChildrenTmr, APP_TIME_MS(1600), NULL);
	}
}


/****************************************************************************
 *
 * NAME: vCheckIfMyChild
 *
 * DESCRIPTION:
 * called on ZDP_REQ/RESP packet to see if it a NWK address response for
 * one of its own children, is so, remove
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vCheckIfMyChild( ZPS_tsAfEvent  * psStackEvent )
{
	ZPS_tsAfZdpEvent sAfZdpEvent;
	zps_bAplZdpUnpackResponse(psStackEvent,   //ZPS_tsAfEvent *psZdoServerEvent,
	                          &sAfZdpEvent); //ZPS_tsAfZdpEvent *psReturnStruct

	if (ZPS_ZDP_NWK_ADDR_RSP_CLUSTER_ID == sAfZdpEvent.u16ClusterId)
	{
		if( (!sAfZdpEvent.uZdpData.sNwkAddrRsp.u8Status) &&
				(ZPS_u16AplZdoGetNwkAddr() != psStackEvent->uEvent.sApsDataIndEvent.uSrcAddress.u16Addr ) )
		{
		    void * thisNet = ZPS_pvAplZdoGetNwkHandle();
			ZPS_tsNwkNib * thisNib = ZPS_psNwkNibGetHandle(thisNet);
			uint8 i;

			DBG_vPrintf(TRACE_AGE_CHILDREN, "NWK Lookup Resp: 0x%04x 0x%016llx\n",
					sAfZdpEvent.uZdpData.sNwkAddrRsp.u16NwkAddrRemoteDev,
					sAfZdpEvent.uZdpData.sNwkAddrRsp.u64IeeeAddrRemoteDev);

			for(i = 0 ; i < thisNib->sTblSize.u16NtActv ; i++)
			{

				if ((sAfZdpEvent.uZdpData.sNwkAddrRsp.u64IeeeAddrRemoteDev == ZPS_u64NwkNibGetMappedIeeeAddr(thisNet,thisNib->sTbl.psNtActv[i].u16Lookup)) &&
					( ZPS_NWK_NT_AP_RELATIONSHIP_CHILD == thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u2Relationship ) &&
					/* Only remove ED */
					( ZPS_NWK_NT_AP_DEVICE_TYPE_ZED == thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1DeviceType) &&
					/* The RxOnIdle Child may also reply - C1 and C2 both has Rx On Idle
					 * C1 was associated with R3 now when R3 is off it rejoined R1
					 * C1<---->R1<--->R2<----->R3<---->C2
					 * R3 Going for check for match for C1 as well as C3, both have RxOnIdeal
					 * R1,C2,C1 are sending a response to R3 but only C1 need to be removed.
					 * */
					( 	( TRUE != thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1RxOnWhenIdle)
//							||
//							/**/
//						(
//							( TRUE == thisNib->sTbl.psNtActv[i].uAncAttrs.bfBitfields.u1RxOnWhenIdle) &&
//							( sStackEvent->uEvent.sApsZdpEvent.uZdpData.sNwkAddrRsp.u16NwkAddrRemoteDev != )
//						)
					) )
				{
					DBG_vPrintf(TRACE_AGE_CHILDREN, "\r\n IEEE reponse and NT Match");

					/* Remove child from the neighbour table */
					memset(&thisNib->sTbl.psNtActv[i], 0, sizeof(ZPS_tsNwkActvNtEntry));
					thisNib->sTbl.psNtActv[i].u16NwkAddr = 0xFFFE;
					ZPS_vSaveAllZpsRecords();
				}
			}
		}

	}
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
