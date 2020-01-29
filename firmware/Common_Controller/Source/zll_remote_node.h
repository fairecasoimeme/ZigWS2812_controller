/*****************************************************************************
 *
 * MODULE:             JN-AN-1171
 *
 * COMPONENT:          zll_remote_node.c
 *
 * DESCRIPTION:        ZLL Demo: Remote Control Functionality -Interface
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

#ifndef APP_SENSOR_NODE_H_
#define APP_SENSOR_NODE_H_

#include "zps_nwk_sap.h"
#include "zps_nwk_nib.h"
#include "zps_nwk_pub.h"
#include "zps_apl.h"
#include "zps_apl_zdo.h"

#include "app_common.h"

#include "zcl_customcommand.h"
#include "zll_utility.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum {
    E_REMOTE_STARTUP,
    E_REMOTE_NFN_START,
    E_REMOTE_WAIT_START,
    E_REMOTE_NETWORK_DISCOVER,
    E_REMOTE_RUNNING
} teRemoteNode_States;

typedef enum {
        FACTORY_NEW_REMOTE = 0,
        NOT_FACTORY_NEW_REMOTE = 0xff
} teRemoteState;



typedef struct {
    teRemoteState eState;
    teRemoteNode_States eNodeState;
    uint8 u8MyChannel;
    uint16 u16MyAddr;
    uint16 u16FreeAddrLow;
    uint16 u16FreeAddrHigh;
    uint16 u16FreeGroupLow;
    uint16 u16FreeGroupHigh;
}tsZllRemoteState;

/*
 * Set TRUE to allow lights that fail to respond to read attribute requests
 * to be aged out of the lighting database
 */
#define LIGHT_DEVICE_AGING          TRUE
#define LIGHT_DEVICE_AGE_LIMIT      (8)
#define LIGHT_DEVICE_NO_ROUTE       (4)

/* set TRUE to produce debug output to show the generated network key
 */
#define SHOW_KEY            FALSE

/* set TRUE to generate a random network key
 * set TRUE for release builds
 */
#define RAND_KEY            TRUE

/* set TRUE to fix the channel the network will operate on
 * set FALSE for release builds
 */
#define FIX_CHANNEL         FALSE

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

PUBLIC void APP_vInitialiseNode(void);

PUBLIC void vInitCommission();
PUBLIC void vSetGroupAddress(uint16 u16GroupStart, uint8 u8NumGroups);

PUBLIC void vStartPolling(void);
void vStartUpHW(void);

PUBLIC void vAppOnOff(teCLD_OnOff_Command eCmd);
PUBLIC void vAppLevelStepMove(teCLD_LevelControl_MoveMode eMode, uint8 u8StepSize, uint16 u16TransitionTime, bool_t bWithOnOff);
PUBLIC void vAppLevelMove(teCLD_LevelControl_MoveMode eMode, uint8 u8Rate, bool_t bWithOnOff);
PUBLIC void vAppLevelStop(void);
PUBLIC void vAppLevelStopWithOnOff(void);

PUBLIC void vAppIdentify(uint16 u16Time);
PUBLIC void vAppIdentifyEffect(teCLD_Identify_EffectId eEffect);

PUBLIC void vSelectLight(void);

PUBLIC void vSetAddressMode(void);

PUBLIC void vAppAddGroup(uint16 u16GroupId, uint8 u8Index);
PUBLIC void vAppRemoveGroup(uint16 u16GroupId, bool_t bBroadcast);
PUBLIC void vAppRemoveAllGroups(bool_t bBroadcast);
PUBLIC void bAppSendGroupMembershipReq(uint8 u8GroupCount, bool_t bBroadcast);
PUBLIC void bAppSendViewGroupReq(uint16 u16GroupId, bool_t bBroadcast);


#ifdef CLD_COLOUR_CONTROL
PUBLIC void vAppMoveSaturation(teCLD_ColourControl_MoveMode eMode, uint8 u8StepsPerSec);
PUBLIC void vAppStepSaturation(teCLD_ColourControl_StepMode eMode, uint8 u8StepSize);
PUBLIC void vAppMoveToHue(uint8 u8Hue, teCLD_ColourControl_Direction eDirection, uint16 u16TransitionTime);
PUBLIC void vAppEnhancedMoveToHue(uint16 u16Hue, teCLD_ColourControl_Direction eDirection);
PUBLIC void vAppMoveToHueAndSaturation(uint8 u8Hue, uint8 u8Saturation, uint16 u16Time);
PUBLIC void vAppEnhancedMoveToHueAndSaturation(uint16 u16Hue, uint8 u8Saturation, uint16 u16Time);
PUBLIC void vAppGotoColour(uint8 u8Colour);
PUBLIC void vAppMoveColour(int16 i16RateX, int16 i16RateY);
PUBLIC void vAppStepColour(int16 i16StepX, int16 i16StepY, uint16 u16TransitionTime);
PUBLIC void vAppMoveHue(teCLD_ColourControl_MoveMode eMode, uint8 u8StepsPerSec);
PUBLIC void vAppEnhancedMoveHue(teCLD_ColourControl_MoveMode eMode, uint16 u16StepsPerSec);
PUBLIC void vAppStepHue(teCLD_ColourControl_StepMode eMode, uint8 u8StepSize, uint8 u8TransitionTime);
PUBLIC void vAppEnhancedStepHue(teCLD_ColourControl_StepMode eMode, uint16 u16StepSize, uint16 u16TransitionTime);
PUBLIC void vAppMoveToSaturaiton(uint8 u8Saturation, uint16 u16TransitionTime);
PUBLIC void vAppStepColourTemperature(teCLD_ColourControl_StepMode eMode, uint16 u16StepSize);
PUBLIC void vAppStepMoveStop(void);
//PUBLIC void vAppMoveToColourTemperature(uint16 u16ColourTemperature, uint16 u16TransitionTime);
PUBLIC void vAppMoveColourTemperature(teCLD_ColourControl_MoveMode eMode, uint16 u16StepsPerSec, uint16 u16ColourTemperatureMin, uint16 u16ColourTemperatureMax);
PUBLIC void vAppColorLoopSet(uint8 u8UpdateFlags, teCLD_ColourControl_LoopAction eAction,teCLD_ColourControl_LoopDirection eDirection, uint16 u16Time, uint16 u16StartHue);
PUBLIC void vAppStopMoveStep();
PUBLIC void vAppGotoHueAndSat( uint8 u8Direction);
PUBLIC void vAPPColourLoopStop(void);
#endif


#ifdef CLD_SCENES
PUBLIC void vAppRemoveSceneById(uint8 u8SceneId, uint16 u16GroupId);
PUBLIC void vAppRemoveAllScene(uint16 u16GroupId);
PUBLIC void vAppGetSceneMembership(uint16 u16GroupId);
PUBLIC void vAppRecallSceneById(uint8 u8SceneId, uint16 u16GroupId);
PUBLIC void vAppAddScene(uint8 u8SceneId, uint16 u16GroupId, uint16 u16TransitionTime);
PUBLIC void vAppEnhancedAddScene(uint8 u8Scene, uint16 u16GroupId);
PUBLIC void vAppViewScene(uint8 u8SceneId, uint16 u16GroupId);
PUBLIC void vAppEnhancedViewScene(uint8 u8SceneId, uint16 u16GroupId);
PUBLIC void vAppCopyScene(uint8 u8Mode, uint8 u8FromSceneId, uint16 u16FromGroupId, uint8 u8ToSceneId, uint16 u16ToGroupId);
PUBLIC void vAppStoreSceneById(uint8 u8SceneId, uint16 u16GroupId);
#endif

PUBLIC void vAppChangeChannel(void);
PUBLIC void vAppIdentifyQuery(void);

PUBLIC void vSetRejoinFilter( void);
PUBLIC void vHandleIdentifyRequest(uint16 u16Duration);


/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/
extern tsZllRemoteState sZllState;
extern PDM_tsRecordDescriptor sZllPDDesc;

extern PUBLIC bool_t bTLinkInProgress;
extern uint16 au16AttribList[];
extern tsZllEndpointInfoTable sEndpointTable;
extern uint32 u32OldFrameCtr;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#endif /*APP_SENSOR_NDOE_H_*/
