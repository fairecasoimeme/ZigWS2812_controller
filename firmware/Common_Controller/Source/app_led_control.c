/*****************************************************************************
 *
 * MODULE:             JN-AN-1171
 *
 * COMPONENT:          app_led_control.c
 *
 * DESCRIPTION:        ZLL demo - Led Control Implementation
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
#include "os.h"
#include "os_gen.h"
#include "app_led_control.h"
#include "AppHardwareApi.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define DIO_BASE_ADDR 0x02002000UL
#define DIO_LED_MASK  0x03UL
#define LED_BLINK_TIME 8E5                /*  16E6/8E5 = 0.05sec blink time */

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef struct
{
    uint32 u32DioDirection;
    uint32 u32DioOutput;
    const volatile uint32 u32DioInput;
} tsDio;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vAppSetLedState(uint32 u32LedMask, bool_t bOn );

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/

tsDio * const psDio = (tsDio *)DIO_BASE_ADDR;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: APP_vInitLeds
 *
 * DESCRIPTION:
 * Initialise the led hardware
 *
 * PARAMETERS:      none            RW  Usage
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vInitLeds(void)
{
    vAHI_DioSetPullup( DIO_LED_MASK,0);        /* pull up on */
    psDio->u32DioDirection &= ~DIO_LED_MASK;  /* input */
}

/****************************************************************************
 *
 * NAME: APP_vBlinkLeds
 *
 * DESCRIPTION:
 * flash the leds, use the inverse pf the shift level of the menu indications
 * set the blink timer running so they can be restored
 *
 * PARAMETERS:      Name            RW  Usage
 * teShiftLevel    eShiftLevel      R - the current state of the leds
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vBlinkLeds(teShiftLevel eShiftLevel)
{
    APP_vSetLeds((~eShiftLevel) & DIO_LED_MASK);
    OS_eStartSWTimer(APP_LedBlinkTimer,LED_BLINK_TIME,(void*)eShiftLevel);
}

/****************************************************************************
 *
 * NAME: APP_vSetLeds
 *
 * DESCRIPTION:
 *
 * Sets the leds to the state given in eShiftLeve, usually menu level
 * matches this to the actual io pins
 *
 * PARAMETERS:      Name            RW  Usage
 * teShiftLevel     eShiftLeve      R the value to write tothe led output pins
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void APP_vSetLeds(teShiftLevel eShiftLevel)
{
    (eShiftLevel & 0x01) ? vAppSetLedState(1<<0, 1): vAppSetLedState(1<<0, 0);
    (eShiftLevel & 0x02) ? vAppSetLedState(1<<1, 1): vAppSetLedState(1<<1, 0);
}

/****************************************************************************
 *
 * NAME: vAppSetLedState
 *
 * DESCRIPTION:
 *
 * set or clear the io pins in the mask to the given value
 *
 * PARAMETERS:      Name            RW  Usage
 * uint32           u32LedMask      R  the register mask to adjust
 * bool_t           bOn             R  the value reqiuired to be copied to the io registers
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vAppSetLedState(uint32 u32LedMask, bool_t bOn )
{
     if (bOn)
     {
           vAHI_DioSetPullup(0,u32LedMask);        /* pull up off */
           psDio->u32DioDirection |= u32LedMask;   /* pin an output */
           psDio->u32DioOutput &= ~u32LedMask;      /* output low */
     }
     else
     {
           vAHI_DioSetPullup(u32LedMask,0);        /* pull up on */
           psDio->u32DioDirection &= ~u32LedMask;  /* pin an input */
     }
}

/****************************************************************************
 *
 * NAME: Wakeup
 *
 * DESCRIPTION:
 *
 * Callback from the blibnk timer, sets the leds to the value given in
 * the void pointer
 *
 * PARAMETERS:      Name            RW  Usage
 * void*            pvDummy         R - void pointer to the data to be written to the leds
 *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
OS_SWTIMER_CALLBACK(App_cbBlinkLed, pvDummy)
{
    APP_vSetLeds( (teShiftLevel)pvDummy );

}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
