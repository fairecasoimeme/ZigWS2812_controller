/****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5168, JN5179].
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
 * Copyright NXP B.V. 2016. All rights reserved
 ****************************************************************************/


/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/

#include <string.h>

/* SDK includes */
#include <jendefs.h>
#include "AppHardwareApi.h"
#include "MicroSpecific.h"
#include "dbg.h"
#include "dbg_uart.h"
//#include "DeviceDefs.h"

/* Device includes */
#include "DriverBulb.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifndef DEBUG_DRIVER
#define TRACE_DRIVER FALSE
#else
#define TRACE_DRIVER TRUE
#endif

/* Default to using DIO11 for WS2812 Data pin */
#ifndef WS2812_DRIVER_LED_PIN
#define WS2812_DRIVER_LED_PIN 11
#endif /* WS2812_DRIVER_LED_PIN */

/* Default to 8 LEDs in the string */
#ifndef WS2812_DRIVER_LED_COUNT
#define WS2812_DRIVER_LED_COUNT 512
#endif /* WS2812_DRIVER_LED_COUNT */

#ifndef WS2812_DRIVER_TICK_DIVISOR
#define WS2812_DRIVER_TICK_DIVISOR 1
#endif /* WS2812_DRIVER_TICK_DIVISOR */

/* Define the DIO mask for the WS2812 data pin */
#define WS2812_DRIVER_LED_PIN_MASK	(1 << WS2812_DRIVER_LED_PIN)

#define ADC_FULL_SCALE   1023

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef struct
{
	uint8 u8Green;
	uint8 u8Red;
	uint8 u8Blue;
} PACK tsWS2812Led;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern bool WS2812process;
extern uint16_t timeProcessWS2812;
/****************************************************************************/
/***        Global Variables                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
PRIVATE struct
{
	uint8  u8Red;
	uint8  u8Green;
	uint8  u8Blue;
	uint8  u8Level;
	bool_t bOn;
} sWS2812State;

PRIVATE tsWS2812Led asWS2812Data[WS2812_DRIVER_LED_COUNT];


/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

extern void WS2812_vOutputData(tsWS2812Led *psLed, uint32 u32Bits, uint32 u32OutputMask);


PUBLIC void DriverBulb_vInit(void)
{
	static bool_t bFirstCalled = TRUE;

	if (bFirstCalled)
	{
		DBG_vPrintf(TRACE_DRIVER, "\nDriverBulb_vInit");

		vAHI_DioSetDirection(0, WS2812_DRIVER_LED_PIN_MASK);

		bFirstCalled = FALSE;
	}
}

PUBLIC void DriverBulb_vOn(void)
{
	DriverBulb_vSetOnOff(TRUE);
	WS2812process=TRUE;
	timeProcessWS2812=0;
}

PUBLIC void DriverBulb_vOff(void)
{
	DriverBulb_vSetOnOff(FALSE);
	WS2812process=TRUE;
	timeProcessWS2812=0;
}

PUBLIC void DriverBulb_vSetOnOff(bool_t bOn)
{
	DBG_vPrintf(TRACE_DRIVER, "\nS:%s",(bOn ? "ON" : "OFF"));
	sWS2812State.bOn =  bOn;
	WS2812process=TRUE;
	timeProcessWS2812=0;
}

PUBLIC void DriverBulb_vSetLevel(uint32 u32Level)
{
	DBG_vPrintf(TRACE_DRIVER, "\nL%d",u32Level);
	sWS2812State.u8Level = MIN(255, u32Level);
	WS2812process=TRUE;
	timeProcessWS2812=0;
}

PUBLIC void DriverBulb_vSetColour(uint32 u32Red, uint32 u32Green, uint32 u32Blue)
{
	DBG_vPrintf(TRACE_DRIVER, "\nR%d G%d B%d",u32Red, u32Green, u32Blue);
	sWS2812State.u8Red   = MIN(255, u32Red);
	sWS2812State.u8Green = MIN(255, u32Green);
	sWS2812State.u8Blue  = MIN(255, u32Blue);
	WS2812process=TRUE;
	timeProcessWS2812=0;

}

PUBLIC bool_t DriverBulb_bOn(void)
{
	return (sWS2812State.bOn);
}

PUBLIC bool_t DriverBulb_bReady(void)
{
	return (TRUE);
}

PUBLIC bool_t DriverBulb_bFailed(void)
{
	return (FALSE);
}

PUBLIC void DriverBulb_vTick(void)
{
	static int iTickCount = 0;

	if (++iTickCount >= WS2812_DRIVER_TICK_DIVISOR)
	{
		iTickCount = 0;

		// Move all pixels along one
		memmove(&asWS2812Data[1], &asWS2812Data[0], sizeof(asWS2812Data) - sizeof(tsWS2812Led));

		// Update the first pixel's value
		if (!sWS2812State.bOn)
		{
			memset(&asWS2812Data[0], 0, sizeof(tsWS2812Led));
		}
		else
		{

				asWS2812Data[0].u8Red   = sWS2812State.u8Level * sWS2812State.u8Red   / 255;
				asWS2812Data[0].u8Green = sWS2812State.u8Level * sWS2812State.u8Green / 255;
				asWS2812Data[0].u8Blue  = sWS2812State.u8Level * sWS2812State.u8Blue  / 255;

		}

		// WS2812 LEDs have very strict timing requirements, therefore we must disable interrupts during the refresh
		MICRO_DISABLE_INTERRUPTS();
		WS2812_vOutputData(asWS2812Data, sizeof(asWS2812Data) * 8 , WS2812_DRIVER_LED_PIN_MASK);
		MICRO_ENABLE_INTERRUPTS();
	}
}

PUBLIC int16 DriverBulb_i16Analogue(uint8 u8Adc, uint16 u16AdcRead)
{
	if (u8Adc == E_AHI_ADC_SRC_VOLT)
	{
		return ((u16AdcRead * 3600)/ADC_FULL_SCALE);
	}
	else
	{
		return(ADC_FULL_SCALE);
	}
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
