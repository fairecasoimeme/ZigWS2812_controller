/****************************************************************************
 *
 * MODULE:             JN-AN-1189 ZHA Demo
 *
 * COMPONENT:          app_mutex.c
 *
 * DESCRIPTION:        ZHA Light Application Initialisation and Startup
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
 * Copyright NXP B.V. 2014. All rights reserved
 *
 ***************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <jendefs.h>
#include "dbg_uart.h"
#include "os.h"
#include "os_gen.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef DEBUG_MUTEX
#define TRACE_MUTEX FALSE
#else
#define TRACE_MUTEX TRUE
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
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vLockZCLMutex
 *
 * DESCRIPTION:
 * Grabs and maintains a counting mutex
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vLockZCLMutex(void)
{
//	OS_eEnterCriticalSection(HA);
/*
	if (u32ZCLMutexCount == 0)
	{
		OS_eEnterCriticalSection(HA);
	}
	u32ZCLMutexCount++;
*/
}


/****************************************************************************
 *
 * NAME: vUnlockZCLMutex
 *
 * DESCRIPTION:
 * Releases and maintains a counting mutex
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void vUnlockZCLMutex(void)
{
//	OS_eExitCriticalSection(HA);
/*
	u32ZCLMutexCount--;
	if (u32ZCLMutexCount == 0)
	{
		OS_eExitCriticalSection(HA);
	}
*/
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
