#include <jendefs.h>
#include "zcl.h"
#include "zcl_options.h"
#include "zcl_customcommand.h"
#include "string.h"
#include "HhDoorLock_internal.h"
#include "dbg.h"
#include "hh_doorlock.h"

#ifdef DEBUG_HH_DOORLOCK
	#define TRACE_HHDOORLOCK TRUE
#else
	#define TRACE_HHDOORLOCK FALSE
#endif
#ifdef HH_DOORLOCK_SERVER
const tsZCL_AttributeDefinition asCLD_HhDoorLockClusterAttributeDefinitions[] = {
    {E_CLD_DOORLOCK_ATTR_ID_ONOFF,	(E_ZCL_AF_RD|E_ZCL_AF_RP|E_ZCL_AF_WR),	E_ZCL_BOOL,	(uint32)(&((tsCLD_HhDoorLock*)(0))->bOnOff),0},     /* Mandatory */
    
#ifdef CLD_DOORLOCK_ATTR_ID_LANG
    {E_CLD_DOORLOCK_ATTR_ID_LANG,	(E_ZCL_AF_RD|E_ZCL_AF_RP|E_ZCL_AF_WR),	E_ZCL_UINT8,	(uint32)(&((tsCLD_HhDoorLock*)(0))->u8Lang),0},
#endif

#ifdef CLD_DOORLOCK_ATTR_ID_VOLUME
    {E_CLD_DOORLOCK_ATTR_ID_VOLUME,	(E_ZCL_AF_RD|E_ZCL_AF_RP|E_ZCL_AF_WR),	E_ZCL_UINT8,	(uint32)(&((tsCLD_HhDoorLock*)(0))->u8Volume),0},
#endif

#ifdef CLD_DOORLOCK_ATTR_ID_HEARTBEATCYCLE
    {E_CLD_DOORLOCK_ATTR_ID_HEARTBEATCYCLE,	(E_ZCL_AF_RD|E_ZCL_AF_RP|E_ZCL_AF_WR),	E_ZCL_UINT8,	(uint32)(&((tsCLD_HhDoorLock*)(0))->u8HeartBeatCycle),0},
#endif
    
#ifdef CLD_DOORLOCK_ATTR_ID_POWERINQUIRE
    {E_CLD_DOORLOCK_ATTR_ID_POWERINQUIRE,	(E_ZCL_AF_RD|E_ZCL_AF_RP|E_ZCL_AF_WR),	E_ZCL_UINT8,	(uint32)(&((tsCLD_HhDoorLock*)(0))->u8PowerInquire),0},
#endif
};
#endif

#ifdef HH_DOORLOCK_SERVER
tsZCL_ClusterDefinition sCLD_HhDoorLockCluster = {
        CLUSTER_ID_HH_DOORLOCK,
        FALSE,
        E_ZCL_SECURITY_NETWORK,
        (sizeof(asCLD_HhDoorLockClusterAttributeDefinitions) / sizeof(tsZCL_AttributeDefinition)),
        (tsZCL_AttributeDefinition*)asCLD_HhDoorLockClusterAttributeDefinitions,
        NULL 
};
#else
tsZCL_ClusterDefinition sCLD_HhDoorLockCluster = {
        CLUSTER_ID_HH_DOORLOCK,
        FALSE,
        E_ZCL_SECURITY_NETWORK,
        0,
        NULL,
        NULL  
};
#endif

#if (defined CLD_HH_DOORLOCK) && (defined HH_DOORLOCK_SERVER)
    uint8 au8HhDoorLockServerAttributeControlBits[(sizeof(asCLD_HhDoorLockClusterAttributeDefinitions) / sizeof(tsZCL_AttributeDefinition))];
#endif

PUBLIC teZCL_Status eCLD_DoorLockCreateHhDoorLock(
                tsZCL_ClusterInstance              *psClusterInstance,
                bool_t                              bIsServer,
                tsZCL_ClusterDefinition            *psClusterDefinition,
                void                               *pvEndPointSharedStructPtr,
                uint8                              *pu8AttributeControlBits)
{
	if (NULL == psClusterInstance) {
		return E_ZCL_ERR_PARAMETER_NULL;
	} 
    // cluster data
    vZCL_InitializeClusterInstance(
           psClusterInstance, 
           bIsServer,
           psClusterDefinition,
           pvEndPointSharedStructPtr,
           pu8AttributeControlBits,
           NULL,
           eCLD_HhDoorLockCommandHandler);
    
    if(psClusterInstance->pvEndPointSharedStructPtr != NULL)
	{
		// set default state
		((tsCLD_HhDoorLock *)(psClusterInstance->pvEndPointSharedStructPtr))->bOnOff = FALSE;

	#ifdef CLD_DOORLOCK_ATTR_ID_LANG
		((tsCLD_HhDoorLock *)(psClusterInstance->pvEndPointSharedStructPtr))->u8Lang = 0x00;
	#endif

	#ifdef CLD_DOORLOCK_ATTR_ID_VOLUME
		((tsCLD_HhDoorLock *)(psClusterInstance->pvEndPointSharedStructPtr))->u8Volume = 0x00;
	#endif

	#ifdef CLD_DOORLOCK_ATTR_ID_HEARTBEATCYCLE
		((tsCLD_HhDoorLock *)(psClusterInstance->pvEndPointSharedStructPtr))->u8HeartBeatCycle = 0x00;
	#endif

	#ifdef CLD_DOORLOCK_ATTR_ID_POWERINQUIRE
		((tsCLD_HhDoorLock *)(psClusterInstance->pvEndPointSharedStructPtr))->u8PowerInquire = 0x00;
	#endif
	}

    return E_ZCL_SUCCESS;	
}


