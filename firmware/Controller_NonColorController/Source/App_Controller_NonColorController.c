/*****************************************************************************
 *
 * MODULE:             JN-AN-1171
 *
 * COMPONENT:          App_Controller_NonColorController.c
 *
 * DESCRIPTION:       ZLL DEMO: Non-Color Controller Implementation
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
 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include "zps_gen.h"
#include "App_Controller_NonColorController.h"
#include <string.h>

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

tsZLL_NonColourRemoteDevice sRemote;
tsCLD_ZllDeviceTable sDeviceTable = { ZLL_NUMBER_DEVICES,
                                       {
                                          { 0,
                                            ZLL_PROFILE_ID,
                                            NON_COLOUR_REMOTE_DEVICE_ID,
                                            CONTROLLER_NONCOLORCONTROLLER_REMOTE_ENDPOINT,
                                            2,
                                            GROUPS_REQUIRED,
                                            0
                                          }
                                       }
};

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PRIVATE void vOverideProfileId(uint16* pu16Profile, uint8 u8Ep);
PRIVATE bool bAllowInterPanEp(uint8 u8Ep);

/****************************************************************************
 **
 ** NAME: eApp_ZLL_RegisterEndpoint
 **
 ** DESCRIPTION:
 ** Register ZLL endpoints
 **
 ** PARAMETER
 ** Type                                Name                    Descirption
 ** tfpZCL_ZCLCallBackFunction          fptr                    Pointer to ZCL Callback function
 ** tsZLL_CommissionEndpoint            psCommissionEndpoint    Pointer to Commission Endpoint
 **
 **
 ** RETURNS:
 ** teZCL_Status
 *
 ****************************************************************************/
teZCL_Status eApp_ZLL_RegisterEndpoint(tfpZCL_ZCLCallBackFunction fptr,
                                       tsZLL_CommissionEndpoint* psCommissionEndpoint)
{

	ZPS_vAplZdoRegisterProfileCallback(vOverideProfileId);
	ZPS_vAplZdoRegisterInterPanFilter( bAllowInterPanEp);
	zps_vSetIgnoreProfileCheck();

	eZLL_RegisterCommissionEndPoint(CONTROLLER_NONCOLORCONTROLLER_COMMISSION_ENDPOINT,
                                    fptr,
                                    psCommissionEndpoint);

    return eZLL_RegisterNonColourRemoteEndPoint(CONTROLLER_NONCOLORCONTROLLER_REMOTE_ENDPOINT,
                                                fptr,
                                                &sRemote);
}


/****************************************************************************
*
* NAME: vOverideProfileId
*
* DESCRIPTION: Allows the application to over ride the profile in the
* simple descriptor (0xc05e) with the ZHA profile id (0x0104)
* required for on air packets
*
*
* PARAMETER: pointer to the profile  to be used, the end point sending the data
*
* RETURNS: void
*
****************************************************************************/
PRIVATE void vOverideProfileId(uint16* pu16Profile, uint8 u8Ep)
{
    if (u8Ep == CONTROLLER_NONCOLORCONTROLLER_REMOTE_ENDPOINT)
    {
        *pu16Profile = 0x0104;
    }
}


/****************************************************************************
*
* NAME: bAllowInterPanEp
*
* DESCRIPTION: Allows the application to decide which end point receive
* inter pan messages
*
*
* PARAMETER: the end point receiving the inter pan
*
* RETURNS: True to allow reception, False otherwise
*
****************************************************************************/
PRIVATE bool bAllowInterPanEp(uint8 u8Ep) {

    return (u8Ep == CONTROLLER_NONCOLORCONTROLLER_COMMISSION_ENDPOINT) ? TRUE: FALSE;
}

/****************************************************************************
 *
 * NAME: vAPP_ZCL_DeviceSpecific_Init
 *
 * DESCRIPTION:
 * ZLL Device Specific initialization
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
void vAPP_ZCL_DeviceSpecific_Init()
{
    /* Initialise the strings in Basic */
    memcpy(sRemote.sBasicServerCluster.au8ManufacturerName, "NXP", CLD_BAS_MANUF_NAME_SIZE);
    memcpy(sRemote.sBasicServerCluster.au8ModelIdentifier, "ZLL-NonColorController", CLD_BAS_MODEL_ID_SIZE);
    memcpy(sRemote.sBasicServerCluster.au8DateCode, "20150212", CLD_BAS_DATE_SIZE);
    memcpy(sRemote.sBasicServerCluster.au8SWBuildID, "2000-0003", CLD_BAS_SW_BUILD_SIZE);
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
