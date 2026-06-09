/*==================================================================================================
*    Battery voltage monitoring via ADC.
*
*    HARDWARE: the WHOLE 2S pack is fed through a resistor divider into the ADC pin.
*      The MCU ADC only reads 0..3.3 V, and a 2S LiPo is up to 8.4 V, so the pack
*      MUST go through a divider into the ADC pin:
*
*          PACK + ---[ R1 ]---+---[ R2 ]--- GND (common with board GND)
*                             |
*                      ADC pin (PTC16 / ADC0_SE14, the old linear-camera input)
*                             |
*                          [100nF] --- GND   (stabilises the high-impedance divider)
*
*      Keep the pin under 3.3 V at full charge (8.4 V). Exact resistor values do
*      NOT need to be known: the firmware is calibrated EMPIRICALLY instead.
*
*    CALIBRATION (in battery.c): with a multimeter, at the SAME instant measure the
*      pack voltage and the voltage on PTC16, and set BATTERY_CAL_SOURCE_MV /
*      BATTERY_CAL_PIN_MV. Every reading is then scaled by SOURCE/PIN, so it stays
*      accurate regardless of resistor tolerance or which way they're fitted.
*      Measured reference: 8.11 V pack -> 1.36 V on PTC16.
*
*    These functions assume DriversInit() (which calls Adc_Init) has already run.
==================================================================================================*/
#ifndef BATTERY_H
#define BATTERY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Adc.h"

/* Call once, passing the ADC group wired to the battery divider (e.g. AdcGroup_0). */
void BatteryInit(Adc_GroupType AdcGroup);

/* Blocking single conversion. Returns the WHOLE-PACK voltage in millivolts. */
uint16 BatteryGetMilliVolts(void);

/* Estimated per-cell voltage = pack / BATTERY_CELL_COUNT (assumes a balanced pack).
 * Pass the pack reading you already took with BatteryGetMilliVolts(). */
uint16 BatteryCellMilliVolts(uint16 PackMilliVolts);

/* TRUE once the per-cell voltage has stayed below the cutoff for several samples
 * in a row (debounced, so a brief sag under load doesn't trip it).
 * Pass the pack reading you already took with BatteryGetMilliVolts(). */
boolean BatteryIsLow(uint16 PackMilliVolts);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_H */
