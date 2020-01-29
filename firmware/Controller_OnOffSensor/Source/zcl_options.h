/*****************************************************************************
 *
 * MODULE:             ZCL Options
 *
 * COMPONENT:          zcl_options.h
 *
 * DESCRIPTION:        Options Header for ZigBee Cluster Library functions
 *                     [Controller On-Off Sensor]
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

#ifndef ZCL_OPTIONS_H
#define ZCL_OPTIONS_H

#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/*
 * Define ONE of the following to set the Primary and Secondary ZLL Channels
 *
 *                      Primaries          Secondaries
 * ZLL_PRIMARY ->       [ 11 15 20 25] and [ 12 13 14 16 17 18 19 21 22 23 24 26]
 * ZLL_PRIMARY_PLUS1 -> [ 12 16 21 26] and [ 11 13 14 15 17 18 19 20 22 23 24 25]
 * ZLL_PRIMARY_PLUS2 -> [ 13 17 22 19] and [ 11 12 14 15 16 18 20 21 23 24 25 26]
 * ZLL_PRIMARY_PLUS3 -> [ 14 18 23 24] and [ 11 12 13 15 16 17 19 20 21 22 25 26]
 *
 */
#define ZLL_PRIMARY
//#define ZLL_PRIMARY_PLUS1
//#define ZLL_PRIMARY_PLUS2
//#define ZLL_PRIMARY_PLUS3




#define ZLL_NO_APS_ACK

// Total number of group addresses required by all sub-device endpoints on the device
#define GROUPS_REQUIRED 4


/* Sets the number of endpoints that will be created by the ZCL library */
#define ZLL_NUMBER_OF_ENDPOINTS                             2
#define ZLL_NUMBER_DEVICES                                  1

#define ZLL_MANUFACTURER_CODE                                0x1037

/* Set this Tue to disable non error default responses from clusters */
#define ZLL_DISABLE_DEFAULT_RESPONSES       (TRUE)

#define ZLL_NUMBER_OF_ZCL_APPLICATION_TIMERS                 0


/* Clusters used by this application */
#define CLD_BASIC
#define BASIC_SERVER
#define BASIC_CLIENT

#define CLD_ZLL_COMMISSION
#define ZLL_COMMISSION_CLIENT

#define CLD_ZLL_UTILITY
#define ZLL_UTILITY_SERVER
#define ZLL_UTILITY_CLIENT

#define CLD_IDENTIFY
#define IDENTIFY_CLIENT
#define CLD_IDENTIFY_TICKS_PER_SECOND   10
#define CLD_IDENTIFY_SUPPORT_ZLL_ENHANCED_COMMANDS

#define CLD_GROUPS
#define GROUPS_CLIENT

#define CLD_ONOFF
#define ONOFF_CLIENT
#define CLD_ONOFF_SUPPORT_ZLL_ENHANCED_COMMANDS

#define CLD_LEVEL_CONTROL
#define LEVEL_CONTROL_CLIENT
#define CLD_LEVELCONTROL_TICKS_PER_SECOND                   10

#define CLD_SCENES
#define SCENES_CLIENT
#define CLD_SCENES_MAX_SCENE_STORAGE_BYTES                  100
#define CLD_SCENES_SUPPORT_ZLL_ENHANCED_COMMANDS

#define CLD_COLOUR_CONTROL
#define COLOUR_CONTROL_CLIENT

#define NUM_ENDPOINT_RECORDS        10
#define NUM_GROUP_RECORDS            4


/****************************************************************************/
/*             Basic Cluster - Optional Attributes                          */
/*                                                                          */
/* Add the following #define's to your zcl_options.h file to add optional   */
/* attributes to the basic cluster.                                         */
/****************************************************************************/

#define ZCL_ATTRIBUTE_READ_SERVER_SUPPORTED
#define ZCL_ATTRIBUTE_READ_CLIENT_SUPPORTED
#define ZCL_ATTRIBUTE_WRITE_SERVER_SUPPORTED

//#define   CLD_BAS_ATTR_LOCATION_DESCRIPTION
//#define   CLD_BAS_ATTR_PHYSICAL_ENVIRONMENT
//#define   CLD_BAS_ATTR_DEVICE_ENBLED
//#define   CLD_BAS_ATTR_ALARM_MASK

#define   CLD_BAS_ATTR_APPLICATION_VERSION
#define   CLD_BAS_ATTR_STACK_VERSION
#define   CLD_BAS_ATTR_HARDWARE_VERSION
#define   CLD_BAS_ATTR_MANUFACTURER_NAME
#define   CLD_BAS_ATTR_MODEL_IDENTIFIER
#define   CLD_BAS_ATTR_DATE_CODE
#define   CLD_BAS_ATTR_SW_BUILD_ID

#define CLD_BAS_APP_VERSION         (1)
#define CLD_BAS_STACK_VERSION       (1)
#define CLD_BAS_HARDWARE_VERSION    (1)
#define CLD_BAS_MANUF_NAME_SIZE     (3)
#define CLD_BAS_MODEL_ID_SIZE       (17)
#define CLD_BAS_DATE_SIZE           (8)
#define CLD_BAS_POWER_SOURCE        E_CLD_BAS_PS_BATTERY
#define CLD_BAS_SW_BUILD_SIZE       (9)

/****************************************************************************/
/*             Colour Control Cluster - Optional Attributes                 */
/*                                                                          */
/* Add the following #define's to your zcl_options.h file to add optional   */
/* attributes to the time cluster.                                          */
/****************************************************************************/

/*
 * Colour attributes, Optional in ZCL spec but Mandatory for ZLL
 */
/* Colour information attribute set attribute ID's (5.2.2.2.1) */
#if 0
#define CLD_COLOURCONTROL_ATTR_CURRENT_HUE
#define CLD_COLOURCONTROL_ATTR_CURRENT_SATURATION
#define CLD_COLOURCONTROL_ATTR_REMAINING_TIME
#define CLD_COLOURCONTROL_ATTR_COLOUR_MODE
#define CLD_COLOURCONTROL_ATTR_COLOUR_TEMPERATURE


/* Defined Primaries Information attribute attribute ID's set (5.2.2.2.2) */
#define CLD_COLOURCONTROL_ATTR_NUMBER_OF_PRIMARIES  3

/* Enable Primary (n) X, Y and Intensity attributes */
#define CLD_COLOURCONTROL_ATTR_PRIMARY_1
#define CLD_COLOURCONTROL_ATTR_PRIMARY_2
#define CLD_COLOURCONTROL_ATTR_PRIMARY_3
#define CLD_COLOURCONTROL_ATTR_PRIMARY_4
#define CLD_COLOURCONTROL_ATTR_PRIMARY_5
#define CLD_COLOURCONTROL_ATTR_PRIMARY_6

/* ZLL enhanced attributes */
#define CLD_COLOURCONTROL_ATTR_ENHANCED_CURRENT_HUE
#define CLD_COLOURCONTROL_ATTR_ENHANCED_COLOUR_MODE
#define CLD_COLOURCONTROL_ATTR_COLOUR_LOOP_ACTIVE
#define CLD_COLOURCONTROL_ATTR_COLOUR_LOOP_DIRECTION
#define CLD_COLOURCONTROL_ATTR_COLOUR_LOOP_TIME
#define CLD_COLOURCONTROL_ATTR_COLOUR_LOOP_START_ENHANCED_HUE
#define CLD_COLOURCONTROL_ATTR_COLOUR_LOOP_STORED_ENHANCED_HUE

#define CLD_COLOURCONTROL_ATTR_COLOUR_CAPABILITIES
#define CLD_COLOURCONTROL_ATTR_COLOUR_TEMPERATURE_PHY_MIN
#define CLD_COLOURCONTROL_ATTR_COLOUR_TEMPERATURE_PHY_MAX

#define CLD_COLOURCONTROL_COLOUR_CAPABILITIES          ( ZLL_CAPABILITY_HUE_SATURATION_SUPPORTED | \
                                                         ZLL_CAPABILITY_ENHANCE_HUE_SUPPORTED | \
                                                         ZLL_CAPABILITY_COLOUR_LOOP_SUPPORTED | \
                                                         ZLL_CAPABILITY_XY_SUPPORTED | \
                                                         ZLL_CAPABILITY_COLOUR_TEMPERATURE_SUPPORTED )

/* Max & Min Limits for colour temperature */
#define CLD_COLOURCONTROL_COLOUR_TEMPERATURE_PHY_MIN    0x0001
#define CLD_COLOURCONTROL_COLOUR_TEMPERATURE_PHY_MAX    0xfef8

#define CLD_COLOURCONTROL_RED_X     (0.68)
#define CLD_COLOURCONTROL_RED_Y     (0.31)
#define CLD_COLOURCONTROL_GREEN_X   (0.11)
#define CLD_COLOURCONTROL_GREEN_Y   (0.82)
#define CLD_COLOURCONTROL_BLUE_X    (0.13)
#define CLD_COLOURCONTROL_BLUE_Y    (0.04)
#define CLD_COLOURCONTROL_WHITE_X   (0.33)
#define CLD_COLOURCONTROL_WHITE_Y   (0.33)


#define CLD_COLOURCONTROL_PRIMARY_1_X           CLD_COLOURCONTROL_RED_X
#define CLD_COLOURCONTROL_PRIMARY_1_Y           CLD_COLOURCONTROL_RED_Y
#define CLD_COLOURCONTROL_PRIMARY_1_INTENSITY   (254 / 3)

#define CLD_COLOURCONTROL_PRIMARY_2_X           CLD_COLOURCONTROL_GREEN_X
#define CLD_COLOURCONTROL_PRIMARY_2_Y           CLD_COLOURCONTROL_GREEN_Y
#define CLD_COLOURCONTROL_PRIMARY_2_INTENSITY   (254 / 3)

#define CLD_COLOURCONTROL_PRIMARY_3_X           CLD_COLOURCONTROL_BLUE_X
#define CLD_COLOURCONTROL_PRIMARY_3_Y           CLD_COLOURCONTROL_BLUE_Y
#define CLD_COLOURCONTROL_PRIMARY_3_INTENSITY   (254 / 3)

#define CLD_COLOURCONTROL_PRIMARY_4_X           (0)
#define CLD_COLOURCONTROL_PRIMARY_4_Y           (0)
#define CLD_COLOURCONTROL_PRIMARY_4_INTENSITY   (0xff)

#define CLD_COLOURCONTROL_PRIMARY_5_X           (0)
#define CLD_COLOURCONTROL_PRIMARY_5_Y           (0)
#define CLD_COLOURCONTROL_PRIMARY_5_INTENSITY   (0xff)

#define CLD_COLOURCONTROL_PRIMARY_6_X           (0)
#define CLD_COLOURCONTROL_PRIMARY_6_Y           (0)
#define CLD_COLOURCONTROL_PRIMARY_6_INTENSITY   (0xff)


#define  CLD_COLOUR_CONTROL_SUPPORT_ZLL_ENHANCED_COMMANDS
#define  CLD_COLOUR_CONTROL_SUPPORT_ZLL_COLOUR_TEMPERATURE_COMMANDS
#endif
/* ZLL attribute in on off cluster */
#define CLD_ONOFF_ATTR_GLOBAL_SCENE_CONTROL
#define CLD_ONOFF_ATTR_ON_TIME
#define CLD_ONOFF_ATTR_OFF_WAIT_TIME


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC void* psGetDeviceTable(void);
/****************************************************************************/
/***        External Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

#endif /* ZCL_OPTIONS_H */
