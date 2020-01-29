#ifndef HH_DOORLOCK_H
#define HH_DOORLOCK_H

#include <jendefs.h>
#include "zcl.h"
#include "zcl_options.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/* Cluster ID's */
#define CLUSTER_ID_HH_DOORLOCK                       0xFC00


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/* hh_doorlock Cluster */
typedef struct
{
	zbool					bOnOff;
#ifdef CLD_DOORLOCK_ATTR_ID_LANG
	zuint8					u8Lang;
#endif
#ifdef CLD_DOORLOCK_ATTR_ID_VOLUME
	zuint8					u8Volume;
#endif
#ifdef CLD_DOORLOCK_ATTR_ID_HEARTBEATCYCLE
	zuint8					u8HeartBeatCycle;
#endif
#ifdef CLD_DOORLOCK_ATTR_ID_POWERINQUIRE
	zuint8					u8PowerInquire;
#endif
} tsCLD_HhDoorLock;

typedef enum {
	/* HH_DoorLock Attribute ID */
	E_CLD_DOORLOCK_ATTR_ID_ONOFF = 0x0000,
	E_CLD_DOORLOCK_ATTR_ID_LANG,
	E_CLD_DOORLOCK_ATTR_ID_VOLUME,
	E_CLD_DOORLOCK_ATTR_ID_HEARTBEATCYCLE,
	E_CLD_DOORLOCK_ATTR_ID_POWERINQUIRE
}teCLD_HhDoorLock_ClusterID;

typedef enum 
{
    E_CLD_HHDOORLOCK_CMD_OFF                      = 0x00,     /* Mandatory */
    E_CLD_HHDOORLOCK_CMD_ON,                                  /* Mandatory */
    E_CLD_HHDOORLOCK_CMD_SETLANGUAGE,
    E_CLD_HHDOORLOCK_CMD_SETVOLUME,
} teCLD_HhDoorLock_Command;

PUBLIC teZCL_Status eCLD_DoorLockCreateHhDoorLock(
                tsZCL_ClusterInstance              *psClusterInstance,
                bool_t                              bIsServer,
                tsZCL_ClusterDefinition            *psClusterDefinition,
                void                               *pvEndPointSharedStructPtr,
                uint8                              *pu8AttributeControlBits);

PUBLIC teZCL_Status eCLD_HhDoorLockCommandSend(
                uint8           u8SourceEndPointId,
                uint8           u8DestinationEndPointId,
                tsZCL_Address   *psDestinationAddress,
                uint8           *pu8TransactionSequenceNumber,
                teCLD_HhDoorLock_Command   eCommand);

extern tsZCL_ClusterDefinition sCLD_HhDoorLockCluster;

#if (defined CLD_HH_DOORLOCK) && (defined HH_DOORLOCK_SERVER)
    extern const tsZCL_AttributeDefinition asCLD_HhDoorLockClusterAttributeDefinitions[];
    extern uint8 au8HhDoorLockServerAttributeControlBits[];
#endif

#endif
