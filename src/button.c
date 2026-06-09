/*==================================================================================================
*    Start button on PTE14. See button.h for the wiring notes (active-low, pull-up).
==================================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                        INCLUDE FILES
==================================================================================================*/
#include "button.h"
#include "debug_signals.h"

/*==================================================================================================
 *                                       LOCAL MACROS
==================================================================================================*/
/* PTE14 Dio channel id = port E (index 4) * 32 + pin 14 = 142. No symbolic name
 * exists in Dio_Cfg.h for this pin, so the raw channel id is used. */
#define BUTTON_DIO_CHANNEL    ((Dio_ChannelType)142U)

/*==================================================================================================
 *                          FREEMASTER-WATCHED GLOBALS
==================================================================================================*/
volatile uint8 FmstrButtonPressed = 0U;   /* 0 = released, 1 = pressed (active-low, PTE14) */

/*==================================================================================================
 *                                       GLOBAL FUNCTIONS
==================================================================================================*/
boolean ButtonIsPressed(void){
    /* Active-low: a press pulls the pin to GND, so LOW means pressed. */
    boolean Pressed = (boolean)(Dio_ReadChannel(BUTTON_DIO_CHANNEL) == STD_LOW);
    FmstrButtonPressed = (uint8)Pressed;   /* FreeMASTER live view */
    return Pressed;
}

#ifdef __cplusplus
}
#endif

/** @} */
