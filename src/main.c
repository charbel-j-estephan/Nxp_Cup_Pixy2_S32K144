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
 *                          SERVO STEERING CALIBRATION
 * FTM3 ch7 runs a 20 ms (50 Hz) frame; the duty arg is scaled 0..0x8000 (32768 = 100%),
 * so 1 count = 20ms/32768 = 0.61 us of pulse width, and 1.5 ms = 0.075*32768 = 2457 counts.
 *
 * Convention: Steer(-100)=full left, Steer(+100)=full right, Steer(0)=straight.
 *   SERVO_MAX_LEFT  = full-left  duty (higher pulse), SteerLeft()
 *   SERVO_MAX_RIGHT = full-right duty (lower pulse),  SteerRight()
 *   SERVO_CENTER    = straight-ahead trim,            SteerStraight()
 *
 * CENTERING PROCEDURE (do once per servo / after any remount):
 *   1. Coarse: with power on and SERVO_CENTER at 2457, pull the horn off its spline
 *      and re-seat it on the tooth that puts the wheels closest to straight.
 *   2. Fine: nudge SERVO_CENTER until the wheels are dead straight. 1 count ~= 0.61 us;
 *      1.5 ms tolerance between servos is ~+/-100 counts. Lower => wheels move RIGHT,
 *      higher => wheels move LEFT.
 *   3. Confirm: roll the car forward slowly on a flat floor; trim out any drift.
 *
 * END-STOPS: set MAX_LEFT/MAX_RIGHT just short of mechanical bind. If the servo
 * buzzes/grinds at an extreme it is jammed on its stop -> back that value off.
 *
 * DIRECTION CHECK: the boot sweep calls SteerLeft() (=> SERVO_MAX_LEFT) first. If the
 * wheels physically go RIGHT there, this servo's polarity is reversed for this car --
 * simply swap the SERVO_MAX_LEFT and SERVO_MAX_RIGHT values below and rebuild.
==================================================================================================*/
#define SERVO_CENTER    2345U   /* straight-ahead trim; -157 from 2457 (1.5ms) for a left-leaning neutral */
#define SERVO_MAX_LEFT  3100U   /* full-left  duty (higher pulse) */
#define SERVO_MAX_RIGHT 1800U   /* full-right duty (lower pulse)  */


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
    Pixy2SetLamp(1U, 0U);      /* upper white lamp ON -> shorter exposure, less motion blur */
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

    /* Now wait for a clean, debounced press, then for its release. Motor stays
     * off (1638 = 0% throttle) the whole time. */
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
    ServoInit(Servo_Pwm, SERVO_MAX_LEFT, SERVO_MAX_RIGHT, SERVO_CENTER);
    SteerLeft();
    DelayMs(1000U);
    SteerStraight();
    DelayMs(1000U);
    SteerRight();
    DelayMs(1000U);
    SteerStraight();
    DelayMs(1000U);
    SteerLeft();
    DelayMs(1000U);
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

    /* Started: hold a gentle constant forward speed; the PID drives the steering from the
     * Pixy2 line vector ("steering only" -- throttle stays fixed). */
    Pwm_SetDutyCycle(0U, 1750);   /* steady, gentle forward throttle */
    DisplayClear();
    DisplayText(0U, "PID Line Follow", 15U, 0U);
    DisplayText(1U, "Dir:", 4U, 0U);
    DisplayText(2U, "Cell:", 5U, 0U);
    DisplayText(2U, "mV", 2U, 13U);
    DisplayRefresh();

    /* Fixed-rate steering controller. The loop is paced to a constant period by SysTick
     * (DelayStartPeriod/DelayWaitPeriodEnd), so the PID's dt is a known constant. */
    const uint32 ControlPeriodUs = 20000U;   /* 50 Hz control tick                           */
    const float  ControlDtS      = 0.02f;    /* seconds, must match ControlPeriodUs           */
    const float  LineCenterX     = 39.0f;    /* center of the 0..78 Pixy2 line frame          */
    const uint16 LineLostLimit   = 10U;      /* frames without a line before reset + recenter */

    Pid    SteerPid;
    DetectedVectors PixyVectors;
    int    LastSteer  = 0;       /* held during brief dropouts, and shown on the OLED */
    uint32 FrameCount = 0U;
    uint16 LostFrames = 0U;

    /* PidInit(Kp, Ki, Kd, SetPoint=0, OutMin, OutMax, IntLimit, DerivAlpha).
     * Kp/Kd are TUNING starting points; Ki=0 (steering seldom needs integral). DerivAlpha 0.4
     * lightly filters the derivative. Output is the -100..+100 steering command. */
    PidInit(&SteerPid, 2.0f, 0.0f, 0.25f, 0.0f, -100.0f, 100.0f, 60.0f, 0.4f);

    while(1){
        DelayStartPeriod(ControlPeriodUs);       /* open the fixed control window */

        /* One camera frame. getMainFeatures returns the tracked main vector (noise-filtered). */
        Pixy2GetVectors(&PixyVectors);

        if(PixyVectors.NumberOfVectors > 0U){
            uint8 LineX = SmoothLineX((uint8)PixyVectors.Vectors[0].x1);  /* smoothed far-end X (0..78) */
            /* Centered + sign-corrected line position: steering right makes the line move LEFT in
             * frame, so we feed (center - X) against a zero set-point -> a positive PID output
             * steers toward the line with positive gains. If it steers the WRONG way, negate this. */
            float CenteredX = LineCenterX - (float)LineX;
            int   SteerCmd  = (int)PidUpdate(&SteerPid, CenteredX, ControlDtS);
            Steer(SteerCmd);
            LastSteer  = SteerCmd;
            LostFrames = 0U;
        }
        else if(++LostFrames >= LineLostLimit){
            PidReset(&SteerPid);                 /* clear stale I/D -> bumpless re-acquire */
            Steer(0);                            /* recenter after a sustained line loss   */
            LastSteer = 0;
        }
        /* else: brief dropout -> hold the last steering command (servo stays put). */

        /* Slow OLED refresh + blocking battery read every ~20 ticks so they don't stall steering. */
        if((++FrameCount % 20U) == 0U){
            uint16 PackMv = BatteryGetMilliVolts();
            DisplayValue(1U, LastSteer, 4U, 5U);                       /* live steering command   */
            DisplayValue(2U, BatteryCellMilliVolts(PackMv), 6U, 6U);   /* live per-cell voltage   */
            DisplayRefresh();
            if(BatteryIsLow(PackMv)){
                StopCar();   /* never returns */
            }
        }

        DelayWaitPeriodEnd();                     /* sleep the rest of the period -> constant dt */
    }
}

#ifdef __cplusplus
}
#endif

/** @} */
