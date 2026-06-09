/*==================================================================================================
*    Battery voltage monitoring via ADC. See battery.h for the wiring/divider notes.
==================================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                        INCLUDE FILES
==================================================================================================*/
#include "battery.h"
#include "debug_signals.h"

/*==================================================================================================
 *                          FREEMASTER-WATCHED GLOBALS
==================================================================================================*/
volatile uint8 FmstrBatteryIsLow = 0U;   /* 0 = OK, 1 = low-voltage latch triggered */

/*==================================================================================================
 *                                       LOCAL MACROS
==================================================================================================*/
/* EMPIRICAL CALIBRATION (more reliable than nominal resistor values).
 * With a multimeter, at the SAME instant measure:
 *   - the battery you feed into the divider  -> BATTERY_CAL_SOURCE_MV
 *   - the voltage on the ADC pin (PTC16)     -> BATTERY_CAL_PIN_MV
 * The code scales every ADC reading by SOURCE/PIN, so it stays accurate
 * regardless of exact resistor values, tolerances, or which way they're fitted.
 * Measured: 8.11 V at the pack -> 1.36 V on PTC16. */
#define BATTERY_CAL_SOURCE_MV     (8110UL)    /* multimeter on the battery */
#define BATTERY_CAL_PIN_MV        (1360UL)    /* multimeter on PTC16       */

#define ADC_VREF_MV               (3300UL)    /* ADC reference voltage, mV     */
#define ADC_MAX_COUNT             (255UL)     /* 8-bit resolution -> 0..255    */

/* --- Pack / cutoff configuration: SET THESE TO YOUR BATTERY --------------------------------
 * BATTERY_CELL_COUNT     : number of cells in series (1 for a 1S pack, 2 for 2S, etc.)
 * BATTERY_CELL_CUTOFF_MV : per-cell voltage at/below which the car must stop.
 *                          LiPo: ~3300 mV is a safe under-load cutoff (3000 absolute min).
 *                          NiMH: use ~1000 mV/cell instead.
 * BATTERY_LOW_DEBOUNCE   : consecutive low samples required before it trips (anti-sag).
 */
#define BATTERY_CELL_COUNT        (2U)      /* full 2S pack fed into the divider -> per-cell = pack/2 */
#define BATTERY_CELL_CUTOFF_MV    (3700U)   /* lowest usable cell voltage = 3.7 V */
#define BATTERY_LOW_DEBOUNCE      (10U)

/*==================================================================================================
 *                                      LOCAL VARIABLES
==================================================================================================*/
static Adc_GroupType       BatteryAdcGroup;
static Adc_ValueGroupType  BatteryAdcResult;
static uint8               BatteryLowCounter = 0U;

/*==================================================================================================
 *                                       GLOBAL FUNCTIONS
==================================================================================================*/
void BatteryInit(Adc_GroupType AdcGroup){
    BatteryAdcGroup = AdcGroup;
    (void)Adc_SetupResultBuffer(BatteryAdcGroup, &BatteryAdcResult);
}

uint16 BatteryGetMilliVolts(void){
    Adc_StatusType Status;
    uint32 PinMilliVolts;
    uint32 BatteryMilliVolts;

    /* Software-triggered one-shot conversion, then wait for it to finish. */
    Adc_StartGroupConversion(BatteryAdcGroup);
    do{
        Status = Adc_GetGroupStatus(BatteryAdcGroup);
    }while((Status != ADC_COMPLETED) && (Status != ADC_STREAM_COMPLETED));

    (void)Adc_ReadGroup(BatteryAdcGroup, &BatteryAdcResult);

    /* Voltage at the pin, then scale back up to the real pack voltage using the
     * empirical SOURCE/PIN ratio (robust to resistor tolerance / swapped fitting). */
    PinMilliVolts     = (uint32)BatteryAdcResult * ADC_VREF_MV / ADC_MAX_COUNT;
    BatteryMilliVolts = PinMilliVolts * BATTERY_CAL_SOURCE_MV / BATTERY_CAL_PIN_MV;

    return (uint16)BatteryMilliVolts;
}

uint16 BatteryCellMilliVolts(uint16 PackMilliVolts){
    return (uint16)(PackMilliVolts / BATTERY_CELL_COUNT);
}

boolean BatteryIsLow(uint16 PackMilliVolts){
    if(BatteryCellMilliVolts(PackMilliVolts) < BATTERY_CELL_CUTOFF_MV){
        if(BatteryLowCounter < BATTERY_LOW_DEBOUNCE){
            BatteryLowCounter++;
        }
    }
    else{
        BatteryLowCounter = 0U;   /* recovered (e.g. load removed) -> reset */
    }
    FmstrBatteryIsLow = (uint8)(BatteryLowCounter >= BATTERY_LOW_DEBOUNCE);
    return (boolean)(BatteryLowCounter >= BATTERY_LOW_DEBOUNCE);
}

#ifdef __cplusplus
}
#endif

/** @} */
