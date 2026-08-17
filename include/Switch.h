/*==================================================================================================
*    Start button on PTE14.
*
*    HARDWARE: the button is wired between PTE14 and GND, with a pull-up holding the
*      pin HIGH while idle. So:
*          idle (not pressed) -> pin HIGH
*          pressed            -> pin pulled to GND -> pin LOW      (ACTIVE-LOW)
*
*      The pull-up can be the MCU's internal one (enabled in the Port config for
*      PTE14 = PORT_INTERNAL_PULL_UP_ENABLED) or an external ~10k to 3.3 V. Without
*      ANY pull-up the input floats and reads garbage, so make sure one is present.
*
*    PTE14 is the only spare pin already configured as a GPIO digital input by the
*      Port driver. Its AUTOSAR Dio channel id is 142 (= port E(4) * 32 + pin 14);
*      there is no symbolic name in Dio_Cfg.h, so the raw id is used directly.
*
*    These functions assume DriversInit() (Port_Init / Dio) has already run.
==================================================================================================*/
#ifndef BUTTON_H
#define BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Dio.h"

/* TRUE while the button is held down (active-low: pin reads LOW when pressed).
 * This is a raw, un-debounced read; debounce in the caller if needed. */
boolean ButtonIsPressed(void);
boolean ButtonIsReleased(void);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_H */
