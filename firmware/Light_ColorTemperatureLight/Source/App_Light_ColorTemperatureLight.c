/*****************************************************************************
 *
 * MODULE:             JN-AN-1171
 *
 * COMPONENT:          App_Light_ColorTemperatureLight.c
 *
 * DESCRIPTION:        ZLL Demo: Color Temperature Light Implementation
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
#include "AppHardwareApi.h"

/* RTOS includes */
#include "os.h"
#include "dbg.h"

/* Stack includes */
#include "zps_gen.h"

/* Application includes */
#include "App_Light_ColorTemperatureLight.h"
#include "app_common.h"
#include "app_zcl_light_task.h"
#include "app_light_interpolation.h"
#include "DriverBulb_Shim.h"
#include "DriverBulb.h"



/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#ifdef DEBUG_LIGHT_TASK
#define TRACE_LIGHT_TASK  TRUE
#else
#define TRACE_LIGHT_TASK FALSE
#endif

#ifdef DEBUG_PATH
#define TRACE_PATH  TRUE
#else
#define TRACE_PATH  FALSE
#endif

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
tsZLL_ColourTemperatureLightDevice sLight;
tsIdentifyWhite sIdEffect;
tsCLD_ZllDeviceTable sDeviceTable = { ZLL_NUMBER_DEVICES,
                                      {
                                          { 0,
                                            ZLL_PROFILE_ID,
                                            COLOUR_TEMPERATURE_LIGHT_DEVICE_ID,
                                            LIGHT_COLORTEMPERATURELIGHT_LIGHT_ENDPOINT,
                                            2,
                                            0,
                                            0}
                                      }
};


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PRIVATE void vOverideProfileId(uint16* pu16Profile, uint8 u8Ep);
/****************************************************************************
 *
 * NAME: eApp_ZLL_RegisterEndpoint
 *
 * DESCRIPTION:
 * Register ZLL endpoints
 *
 * PARAMETER
 * Type                        Name                 Descirption
 * tfpZCL_ZCLCallBackFunction  fptr                 Pointer to ZCL Callback function
 * tsZLL_CommissionEndpoint    psCommissionEndpoint Pointer to Commission Endpoint
 *
 * RETURNS:
 * teZCL_Status
 *
 ****************************************************************************/
PUBLIC teZCL_Status eApp_ZLL_RegisterEndpoint(tfpZCL_ZCLCallBackFunction fptr,
                                       tsZLL_CommissionEndpoint* psCommissionEndpoint)
{

	ZPS_vAplZdoRegisterProfileCallback(vOverideProfileId);
	zps_vSetIgnoreProfileCheck();

	eZLL_RegisterCommissionEndPoint(LIGHT_COLORTEMPERATURELIGHT_COMMISSION_ENDPOINT,
                                    fptr,
                                    psCommissionEndpoint);

    return eZLL_RegisterColourTemperatureLightEndPoint(LIGHT_COLORTEMPERATURELIGHT_LIGHT_ENDPOINT,
                                                      fptr,
                                                      &sLight);
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
    if (u8Ep == LIGHT_COLORTEMPERATURELIGHT_LIGHT_ENDPOINT)
    {
        *pu16Profile = 0x0104;
    }
}

#if(!defined DR1221) && (!defined DR1221_Dimic)
/****************************************************************************
 *
 * NAME: vApp_eCLD_ColourControl_GetRGB
 *
 * DESCRIPTION:
 * To get RGB value
 *
 * PARAMETER
 * Type         Name               Descirption
 * uint8 *      pu8Red             Pointer to Red in RGB value
 * uint8 *      pu8Green           Pointer to Green in RGB value
 * uint8 *      pu8Blue            Pointer to Blue in RGB value
 *
 * RETURNS:
 * teZCL_Status
 *
 ****************************************************************************/
PUBLIC void vApp_eCLD_ColourControl_GetRGB(uint8 *pu8Red,uint8 *pu8Green,uint8 *pu8Blue)
{
    eCLD_ColourControl_GetRGB(LIGHT_COLORTEMPERATURELIGHT_LIGHT_ENDPOINT,
                              pu8Red,
                               pu8Green,
                              pu8Blue);
}
#endif

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
PUBLIC void vAPP_ZCL_DeviceSpecific_Init(void)
{
    /* Initialise the strings in Basic */
    memcpy(sLight.sBasicServerCluster.au8ManufacturerName, "NXP", CLD_BAS_MANUF_NAME_SIZE);
    memcpy(sLight.sBasicServerCluster.au8ModelIdentifier, "ZLL-ColorTemperature", CLD_BAS_MODEL_ID_SIZE);
    memcpy(sLight.sBasicServerCluster.au8DateCode, "20150212", CLD_BAS_DATE_SIZE);
    memcpy(sLight.sBasicServerCluster.au8SWBuildID, "1000-0003", CLD_BAS_SW_BUILD_SIZE);

    sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_STOP_EFFECT;
    sIdEffect.u8Tick = 0;

    /* Set the initial value to 4000 Kelvin */
    sLight.sColourControlServerCluster.u16ColourTemperatureMired = (1E6/4000);

#if (defined DR1221) || (defined DR1221_Dimic)
    {
    	uint16 u16PhyMax;
    	uint16 u16PhyMin;

    	DriverBulb_vGetColourTempPhyMinMax(&u16PhyMin,&u16PhyMax);
    	sLight.sColourControlServerCluster.u16ColourTemperatureMiredPhyMin = u16PhyMin;
    	sLight.sColourControlServerCluster.u16ColourTemperatureMiredPhyMax = u16PhyMax;
    }
#endif


}

/****************************************************************************
 *
 * NAME: APP_vHandleIdentify
 *
 * DESCRIPTION:
 * ZLL Device Specific identify
 *
 * PARAMETER: the identify time
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void APP_vHandleIdentify(uint16 u16Time)
{
    static bool bActive = FALSE;

#if(!defined DR1221) && (!defined DR1221_Dimic)
    uint8 u8Red, u8Green, u8Blue;
    vApp_eCLD_ColourControl_GetRGB(&u8Red, &u8Green, &u8Blue);
    #endif

    if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT) {
        /* do nothing */
        //DBG_vPrintf(TRACE_LIGHT_TASK, "Efect do nothing\n");
    }
    else if (u16Time == 0)
    {
        /*
         * Restore to off/off state
         */
        DBG_vPrintf(TRACE_PATH, "\nPath 3");
        #if (defined DR1221) || (defined DR1221_Dimic) /* Restore the CCT TW bulb to pre-identify state */


        vTunableWhiteLightSetLevels(sLight.sOnOffServerCluster.bOnOff,
                                    sLight.sLevelControlServerCluster.u8CurrentLevel,
                                    sLight.sColourControlServerCluster.u16ColourTemperatureMired);

        #else /* Restore CCT RGB bulb to pre-identify State */
        vRGBLight_SetLevels(sLight.sOnOffServerCluster.bOnOff,
                            sLight.sLevelControlServerCluster.u8CurrentLevel,
                            u8Red,
                            u8Green,
                            u8Blue);
        #endif
        bActive = FALSE;
    }
    else
    {
        /* Set the Identify levels */
        if (!bActive) {
            bActive = TRUE;
            sIdEffect.u8Level = 250;
            sIdEffect.u8Count = 5;
            #if (defined DR1221) || (defined DR1221_Dimic) /* Restore the CCT TW bulb to pre-identify state */
            vTunableWhiteLightSetLevels(TRUE,
                                        CLD_LEVELCONTROL_MAX_LEVEL,
                                        sLight.sColourControlServerCluster.u16ColourTemperatureMired);

            #else /* Restore CCT RGB bulb to pre-identify State */
            vRGBLight_SetLevels(TRUE,
                                CLD_LEVELCONTROL_MAX_LEVEL,
                                u8Red,
                                u8Green,
                                u8Blue);
            #endif
        }
    }
}

/****************************************************************************
 *
 * NAME: vIdEffectTick
 *
 * DESCRIPTION:
 * ZLL Device Specific identify tick
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/
PUBLIC void vIdEffectTick(uint8 u8Endpoint) {

#if(!defined DR1221) && (!defined DR1221_Dimic)
    uint8 u8Red, u8Green, u8Blue;
    vApp_eCLD_ColourControl_GetRGB(&u8Red, &u8Green, &u8Blue);
    #endif

    if (u8Endpoint != LIGHT_DIMMABLELIGHT_LIGHT_ENDPOINT) {
        return;
    }

    if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT)
    {
        if (sIdEffect.u8Tick > 0)
        {
            //DBG_vPrintf(TRACE_PATH, "\nPath 5");

            sIdEffect.u8Tick--;
            /* Set the light parameters */
            #if (defined DR1221) || (defined DR1221_Dimic)
            vTunableWhiteLightSetLevels(TRUE,sIdEffect.u8Level,sLight.sColourControlServerCluster.u16ColourTemperatureMired);
            #else
            vRGBLight_SetLevels(TRUE, sIdEffect.u8Level, u8Red, u8Green, u8Blue);
            #endif

            /* Now adjust parameters ready for for next round */
            switch (sIdEffect.u8Effect) {
                case E_CLD_IDENTIFY_EFFECT_BLINK:
                    break;

                case E_CLD_IDENTIFY_EFFECT_BREATHE:
                    if (sIdEffect.bDirection) {
                        if (sIdEffect.u8Level >= 250) {
                            sIdEffect.u8Level -= 50;
                            sIdEffect.bDirection = 0;
                        } else {
                            sIdEffect.u8Level += 50;
                        }
                    } else {
                        if (sIdEffect.u8Level == 0) {
                            // go back up, check for stop
                            sIdEffect.u8Count--;
                            if ((sIdEffect.u8Count) && ( !sIdEffect.bFinish)) {
                                sIdEffect.u8Level += 50;
                                sIdEffect.bDirection = 1;
                            } else {
                                //DBG_vPrintf(TRACE_LIGHT_TASK, "\n>>set tick 0<<");
                                /* lpsw2773 - stop the effect on the next tick */
                                sIdEffect.u8Tick = 0;
                            }
                        } else {
                            sIdEffect.u8Level -= 50;
                        }
                    }
                    break;
                case E_CLD_IDENTIFY_EFFECT_OKAY:
                    if ((sIdEffect.u8Tick == 10) || (sIdEffect.u8Tick == 5)) {
                        sIdEffect.u8Level ^= 254;
                        if (sIdEffect.bFinish) {
                            sIdEffect.u8Tick = 0;
                        }
                    }
                    break;
                case E_CLD_IDENTIFY_EFFECT_CHANNEL_CHANGE:
                    if ( sIdEffect.u8Tick == 74) {
                        sIdEffect.u8Level = 1;
                        if (sIdEffect.bFinish) {
                            sIdEffect.u8Tick = 0;
                        }
                    }
                    break;
                default:
                    if ( sIdEffect.bFinish ) {
                        sIdEffect.u8Tick = 0;
                    }
                }
        } else {
            /*
             * Effect finished, restore the light
             */
            DBG_vPrintf(TRACE_PATH, "\nEffect End");
            sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_STOP_EFFECT;
            sIdEffect.bDirection = FALSE;
            APP_ZCL_vSetIdentifyTime(0);
            #if (defined DR1221) || (defined DR1221_Dimic)
            vTunableWhiteLightSetLevels( sLight.sOnOffServerCluster.bOnOff, sLight.sLevelControlServerCluster.u8CurrentLevel,sLight.sColourControlServerCluster.u16ColourTemperatureMired);
            #else
            vRGBLight_SetLevels( sLight.sOnOffServerCluster.bOnOff, sLight.sLevelControlServerCluster.u8CurrentLevel, u8Red, u8Green, u8Blue);
            #endif
        }
    } else {
        if (sLight.sIdentifyServerCluster.u16IdentifyTime) {
            sIdEffect.u8Count--;
            if (0 == sIdEffect.u8Count) {
                sIdEffect.u8Count = 5;
                if (sIdEffect.u8Level) {
                    sIdEffect.u8Level = 0;

                    #if (defined DR1221) || (defined DR1221_Dimic)
                    vTunableWhiteLightSetLevels( FALSE, 0,sLight.sColourControlServerCluster.u16ColourTemperatureMired);
                    #else
                    vRGBLight_SetLevels( FALSE, 0, u8Red, u8Green, u8Blue);
                    #endif
                }
                else
                {
                    sIdEffect.u8Level = 250;
                    #if (defined DR1221) || (defined DR1221_Dimic)
                    vTunableWhiteLightSetLevels( TRUE, CLD_LEVELCONTROL_MAX_LEVEL,sLight.sColourControlServerCluster.u16ColourTemperatureMired);
                    #else
                    vRGBLight_SetLevels( TRUE, CLD_LEVELCONTROL_MAX_LEVEL, u8Red, u8Green, u8Blue);
                    #endif
                }
            }
        }
    }

}

/****************************************************************************
 *
 * NAME: vStartEffect
 *
 * DESCRIPTION:
 * ZLL Device Specific identify effect set up
 *
 * PARAMETER: void
 *
 * RETURNS: void
 *
 ****************************************************************************/

PUBLIC void vStartEffect(uint8 u8Effect)
{
    switch (u8Effect) {
		case E_CLD_IDENTIFY_EFFECT_BLINK:
			sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_BLINK;
			if (sLight.sOnOffServerCluster.bOnOff) {
				sIdEffect.u8Level = 0;
			} else {
				sIdEffect.u8Level = 250;
			}
			sIdEffect.bFinish = FALSE;
			APP_ZCL_vSetIdentifyTime(2);
			sIdEffect.u8Tick = 10;
			break;
		case E_CLD_IDENTIFY_EFFECT_BREATHE:
			sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_BREATHE;
			sIdEffect.bDirection = 1;
			sIdEffect.bFinish = FALSE;
			sIdEffect.u8Level = 0;
			sIdEffect.u8Count = 15;
			APP_ZCL_vSetIdentifyTime(17);
			sIdEffect.u8Tick = 200;
			break;
		case E_CLD_IDENTIFY_EFFECT_OKAY:
			sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_OKAY;
			sIdEffect.bFinish = FALSE;
			if (sLight.sOnOffServerCluster.bOnOff) {
				sIdEffect.u8Level = 0;
			} else {
				sIdEffect.u8Level = 254;
			}
			APP_ZCL_vSetIdentifyTime(3);
			sIdEffect.u8Tick = 15;
			break;
		case E_CLD_IDENTIFY_EFFECT_CHANNEL_CHANGE:
			sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_CHANNEL_CHANGE;
			sIdEffect.u8Level = 254;
			sIdEffect.bFinish = FALSE;
			APP_ZCL_vSetIdentifyTime(9);
			sIdEffect.u8Tick = 80;
			break;

		case E_CLD_IDENTIFY_EFFECT_FINISH_EFFECT:
			if (sIdEffect.u8Effect < E_CLD_IDENTIFY_EFFECT_STOP_EFFECT)
			{
				DBG_vPrintf(TRACE_LIGHT_TASK, "\n<FINISH>");
				sIdEffect.bFinish = TRUE;
			}
			break;
		case E_CLD_IDENTIFY_EFFECT_STOP_EFFECT:
			sIdEffect.u8Effect = E_CLD_IDENTIFY_EFFECT_STOP_EFFECT;
			APP_ZCL_vSetIdentifyTime(1);
			break;
	}
}

/****************************************************************************
 *
 * NAME: vRGBLight_SetLevels
 *
 * DESCRIPTION:
 * Set the RGB and levels
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vRGBLight_SetLevels(bool_t bOn, uint8 u8Level, uint8 u8Red, uint8 u8Green, uint8 u8Blue)
{
	 if (bOn == TRUE)
	 {
		 vLI_Start(u8Level, u8Red, u8Green, u8Blue,0);
	 } 
     else 
     {
        vLI_Stop();  
    }
	 vBULB_SetOnOff(bOn);
}

/****************************************************************************
 *
 * NAME: vTunableWhiteLightSetLevels
 *
 * DESCRIPTION:
 * Device Specific build driver interface
 *
 * PARAMETER: the on/off state, the level, and colour temperature
 *
 ****************************************************************************/


PUBLIC void vTunableWhiteLightSetLevels(bool_t bOn, uint32 u32Level, int32 i32ColourTemperature)
{
	if (bOn) /* only move the colour temperature if lamp is on */
	{
		vLI_Start(u32Level, 0,0,0,(1E6/i32ColourTemperature));
	}
    else
    {
        vLI_Stop();
    }
	vBULB_SetOnOff(bOn);
}

/*****************************************************************************/
/* OS config sheet has extra ISR for real bulbs so include stubs on DK4      */
/*  bulbs to allow builds                                                    */
/*****************************************************************************/
#if (defined DR1175) || (defined DR1173)
OS_ISR(vISR_Timer3)
{
	(void) u8AHI_TimerFired(E_AHI_TIMER_3);
}
OS_ISR(vISR_Timer4)
{
	(void) u8AHI_TimerFired(E_AHI_TIMER_4);
}

#endif

/*****************************************************************************/
/* Real bulb DR1221 or DR1221_Dimic re-purposes system controller interrupt which is used    */
/* for button press detection on DK4. This is OK as real bulbs dont have     */
/* buttons (DR1221 uses is for Rebroadcast flicker prevention & mains synch  */
/*****************************************************************************/
#if (defined DR1221) || (defined DR1221_Dimic)
OS_TASK(APP_ButtonsScanTask)
{
	(void) u32AHI_DioWakeStatus();
}
#endif

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

