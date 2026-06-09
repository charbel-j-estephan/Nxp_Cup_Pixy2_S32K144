/*==================================================================================================
*    Copyright 2021-2024 NXP
*
*    NXP Confidential and Proprietary. This software is owned or controlled by NXP and may only be
*    used strictly in accordance with the applicable license terms. By expressly
*    accepting such terms or by downloading, installing, activating and/or otherwise
*    using the software, you are agreeing that you have read, and that you agree to
*    comply with and are bound by, such license terms. If you do not agree to be
*    bound by the applicable license terms, then you may not retain, install,
*    activate or otherwise use the software.
==================================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
 *                                        INCLUDE FILES
==================================================================================================*/
#include "Pwm.h"
#include "main_functions.h"
#include "delay.h"
#include "servo.h"
#include "display.h"
#include "battery.h"
#include "button.h"
#include "pixy2.h"

/* Pixy2 fills this global with the raw I2C reply; a valid Pixy2 response starts with
 * the sync bytes 0xAF (or 0xAE) then 0xC1. Defined in pixy2.c. */
extern I2c_DataType PixyReceivedLinesBuffer[];

/*==================================================================================================
 *                          FREEMASTER-WATCHED GLOBALS
 * Read live by FreeMASTER over the PEmicro debug probe (run-mode memory access).
 * They MUST be global + volatile so they get a fixed RAM address in the .elf and the
 * compiler always writes the current value to memory. In FreeMASTER: connect with the
 * PEmicro plug-in, load this project's .elf for symbols, then add these to a
 * Scope (live) or Recorder (buffered) to graph them.
==================================================================================================*/
volatile sint16 FmstrDir      = 0;   /* steering command, -100 (left) .. +100 (right) */
volatile uint16 FmstrPackMv   = 0;   /* whole 2S pack voltage, mV                     */
volatile uint16 FmstrCellMv   = 0;   /* estimated per-cell voltage, mV                */
volatile uint16 FmstrThrottle = 0;   /* PWM duty currently commanded to the ESC       */

/*==================================================================================================
 *                                       LOCAL FUNCTIONS
==================================================================================================*/
/* Boot-time Pixy2 connectivity diagnostic. Cycles the Pixy's own RGB LED (a visible
 * write-path check), reads vectors, and validates the I2C reply's sync bytes. Shows
 * PIXY OK / PIXY FAIL plus the vector count and raw sync bytes on the OLED for a few
 * seconds, then returns so the normal ESC/servo flow continues.
 *   - LED cycles red/green/blue + "PIXY OK"  -> camera wired and talking on I2C.
 *   - LED dead and/or "PIXY FAIL"            -> still in SPI mode, wrong address,
 *                                               or SDA/SCL/GND wiring problem. */
static void PixyCheck(void)
{
    DetectedVectors PixyVectors;
    boolean Ok;
    uint8 Pass;

    Pixy2Init(0x54, I2cConf_I2cChannel_Display_Channel);  /* address 0x54, shared I2C bus 0 */

    for(Pass = 0U; Pass < 10U; Pass++){                   /* ~5 s diagnostic window */
        /* Visible heartbeat on the Pixy's RGB LED (proves the I2C write path). */
        Pixy2SetLed(255U, 0U, 0U); DelayMs(150U);
        Pixy2SetLed(0U, 255U, 0U); DelayMs(150U);
        Pixy2SetLed(0U, 0U, 255U); DelayMs(150U);

        Pixy2GetVectors(&PixyVectors);                    /* triggers an I2C read */

        /* A valid Pixy2 reply begins with sync 0xAF/0xAE (175/174) then 0xC1 (193). */
        Ok = (boolean)(((PixyReceivedLinesBuffer[0] == 175U) ||
                        (PixyReceivedLinesBuffer[0] == 174U)) &&
                        (PixyReceivedLinesBuffer[1] == 193U));

        DisplayClear();
        DisplayText(0U, Ok ? "PIXY OK" : "PIXY FAIL", Ok ? 7U : 9U, 0U);
        DisplayText(1U, "Vectors:", 8U, 0U);
        DisplayValue(1U, PixyVectors.NumberOfVectors, 3U, 9U);
        DisplayText(2U, "Sync:", 5U, 0U);
        DisplayValue(2U, PixyReceivedLinesBuffer[0], 3U, 6U);   /* raw byte 0 */
        DisplayValue(2U, PixyReceivedLinesBuffer[1], 3U, 10U);  /* raw byte 1 */
        DisplayRefresh();
        DelayMs(50U);
    }

    Pixy2SetLed(0U, 0U, 0U);   /* LED off before moving on */
}

/* Battery too low: cut the motor, center the wheels, warn, and stay stopped.
 * This ESC is unidirectional: 1.0ms (1638) = 0% throttle = OFF; 1.5ms (2457) = ~50%.
 * So we must command MIN (1638), not "neutral", to actually stop the motor. */
static void StopCar(void)
{
    Pwm_SetDutyCycle(0U, 1638U);   /* MIN pulse = 0% throttle -> motor off */
    SteerStraight();
    DisplayClear();
    DisplayText(0U, "LOW BATTERY!", 12U, 0U);
    DisplayText(1U, "MOTOR STOPPED", 13U, 0U);
    DisplayRefresh();
    while(1){
        Pwm_SetDutyCycle(0U, 1638U);   /* hold motor off forever */
        DelayMs(100U);
    }
}

/* Hold the motor OFF (1638 = 0% throttle on this unidirectional ESC) and block until
 * the driver presses the start button on PTE14. Active-low, debounced, press-once.
 * Battery is still watched here so a low pack while waiting still stops the car. */
static void WaitForStartButton(void)
{
    DisplayClear();
    DisplayText(0U, "Press to START", 14U, 0U);
    DisplayRefresh();

    /* Ignore a button that is already held when we arrive, so a stuck/held button
     * can't auto-start the car. Wait for it to be released first. */
    while(ButtonIsPressed()){
        Pwm_SetDutyCycle(0U, 1638U);
        if(BatteryIsLow(BatteryGetMilliVolts())){
            StopCar();   /* never returns */
        }
        DelayMs(10U);
    }

    /* Now wait for a clean, debounced press, then for its release. */
    for(;;){
        Pwm_SetDutyCycle(0U, 1638U);   /* motor stays off while waiting */
        if(BatteryIsLow(BatteryGetMilliVolts())){
            StopCar();   /* never returns */
        }
        if(ButtonIsPressed()){
            DelayMs(30U);              /* debounce the contact bounce */
            if(ButtonIsPressed()){     /* still pressed -> a real press */
                while(ButtonIsPressed()){
                    DelayMs(10U);      /* wait for release so we start exactly once */
                }
                return;
            }
        }
        DelayMs(10U);
    }
}

/*==================================================================================================
 *                                       GLOBAL FUNCTIONS
==================================================================================================*/
/**
 * @brief   ESC THROTTLE CALIBRATION on channel 0 (PTE2) + servo sweep + live display.
 *          KEEP WHEELS OFF THE GROUND.
 *
 *          2457 = 1.5ms neutral, 3276 = 2.0ms full forward, 1638 = 1.0ms full reverse/min.
 */
int main(void)
{
    DriversInit();
    DelayInit(48000000U);

    /* OLED: I2C is already up from DriversInit(); just configure the display */
    DisplayInit(I2cConf_I2cChannel_Display_Channel, STD_ON);

    /* Boot-time camera connectivity check (LED cycle + PASS/FAIL on the OLED). */
    PixyCheck();

    DisplayClear();
    DisplayText(0U, "Calibrating ESC", 15U, 0U);
    DisplayRefresh();

    /* Battery sense reuses the (unused) linear-camera ADC: AdcGroup_0 = PTC16/SE14.
     * Wire the battery through a divider into PTC16 (see battery.h). */
    BatteryInit(AdcGroup_0);

    /* Servo on channel 1 (PTE6). 2457 = 1.5ms center; Max=left, Min=right.
     * Hold the wheels straight. Tighten Max/Min if the steering binds at the ends. */
    ServoInit(Servo_Pwm, 3100U, 1800U, 2457U);
    SteerStraight();

    /* MAX point - connect the ESC battery NOW while this is held */
    Pwm_SetDutyCycle(0U, 3276U);
    DelayMs(6000U);

    /* NEUTRAL point */
    Pwm_SetDutyCycle(0U, 2457U);
    DelayMs(4000U);

    /* MIN point */
    Pwm_SetDutyCycle(0U, 1638U);
    DelayMs(4000U);

    /* Arm the ESC at MIN throttle (1638 = 1.0ms = 0% on this unidirectional ESC).
     * The motor stays OFF here, which is also the correct arming point. */
    Pwm_SetDutyCycle(0U, 1638U);
    DelayMs(3000U);

    /* Wait for the driver to press the start button before applying any throttle. */
    WaitForStartButton();          /* holds motor off; returns on a debounced press */

    /* Give the driver 2 s to clear the car after pressing, motor still OFF. */
    Pwm_SetDutyCycle(0U, 1638U);
    DelayMs(2000U);

    /* Started: hold a gentle forward speed while the servo sweeps left<->right */
    Pwm_SetDutyCycle(0U, 2650U);   /* steady, gentle forward throttle */
    FmstrThrottle = 2650U;         /* mirror for FreeMASTER */
    DisplayClear();
    DisplayText(0U, "Servo + ESC", 11U, 0U);
    DisplayText(1U, "Dir:", 4U, 0U);
    DisplayText(2U, "Cell:", 5U, 0U);
    DisplayText(2U, "mV", 2U, 13U);
    DisplayRefresh();

    while(1){
        /* The servo steps every 2 ms for a fast sweep. The slow OLED refresh and the
         * blocking battery read only run every 20 steps, so they don't throttle it. */
        for(int Dir = -100; Dir <= 100; Dir++){   /* full left -> full right */
            Steer(Dir);
            FmstrDir = (sint16)Dir;                                     /* live steering for FreeMASTER */
            if((Dir % 20) == 0){
                uint16 PackMv = BatteryGetMilliVolts();                  /* whole 2S pack */
                FmstrPackMv = PackMv;                                    /* mirrors for FreeMASTER */
                FmstrCellMv = BatteryCellMilliVolts(PackMv);
                DisplayValue(1U, Dir, 4U, 5U);                          /* live steering value */
                DisplayValue(2U, BatteryCellMilliVolts(PackMv), 6U, 6U);/* live per-cell voltage */
                DisplayRefresh();
                if(BatteryIsLow(PackMv)){
                    StopCar();   /* never returns */
                }
            }
            DelayMs(2U);
        }
        for(int Dir = 100; Dir >= -100; Dir--){   /* full right -> full left */
            Steer(Dir);
            FmstrDir = (sint16)Dir;
            if((Dir % 20) == 0){
                uint16 PackMv = BatteryGetMilliVolts();
                FmstrPackMv = PackMv;
                FmstrCellMv = BatteryCellMilliVolts(PackMv);
                DisplayValue(1U, Dir, 4U, 5U);
                DisplayValue(2U, BatteryCellMilliVolts(PackMv), 6U, 6U);
                DisplayRefresh();
                if(BatteryIsLow(PackMv)){
                    StopCar();
                }
            }
            DelayMs(2U);
        }
    }
}

#ifdef __cplusplus
}
#endif

/** @} */
