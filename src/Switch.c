/*==================================================================================================
*    Start button on PTE14. See button.h for the wiring notes (active-low, pull-up).
==================================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                        INCLUDE FILES
==================================================================================================*/
#include "Switch.h"
#include "battery.h"
#include "delay.h"

/*==================================================================================================
 *                                       LOCAL MACROS
==================================================================================================*/
/* PTE14 Dio channel id = port E (index 4) * 32 + pin 14 = 142. No symbolic name
 * exists in Dio_Cfg.h for this pin, so the raw channel id is used. */
#define SWITCH_DIO_CHANNEL    ((Dio_ChannelType)142U)

/*==================================================================================================
 *                                       GLOBAL FUNCTIONS
==================================================================================================*/
boolean ButtonIsPressed(void){
    /* Active-low: switch closed pulls the pin to GND, so LOW means the switch is ON/closed. */
    uint8 counter = 0U;
    boolean SwitchClosed = false;

    for (uint8 i = 0U; i <= 9U; i++){
        if (Dio_ReadChannel(SWITCH_DIO_CHANNEL) == STD_LOW){
            counter++;
        }
        DelayMs(1U);
    }

    if ((counter >= 7U) && !BatteryIsLow(BatteryGetMilliVolts())){
        SwitchClosed = true;
    }

    return SwitchClosed;
}


#ifdef __cplusplus
}
#endif

/** @} */
