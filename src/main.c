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
#include "Switch.h"
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
#define SERVO_CENTER    2450U   /* straight-ahead trim; -157 from 2457 (1.5ms) for a left-leaning neutral */
#define SERVO_MAX_LEFT  1800U   /* full-left  duty (lower pulse)  -- swapped: this servo's polarity is reversed */
#define SERVO_MAX_RIGHT 3100U   /* full-right duty (higher pulse) -- swapped: this servo's polarity is reversed */

/*==================================================================================================
 *                          THROTTLE / SPEED GOVERNOR
 * The ESC has NO feedback path (no RPM/current telemetry into the MCU), so throttle is fully
 * open-loop. A flat kickstart->cruise step was found to cause the ESC to stall (likely BEMF
 * commutation loss at low RPM under load / a hard-step read as a brake command) and then
 * auto-resync, which looks like a "jumpstart" a couple seconds later.
 *
 * Fix: ramp duty up in small steps every control tick instead of jumping straight to cruise,
 * and gate the ramp (and the whole drive command) on Pixy2 frame validity so a bad/garbage
 * camera read cuts throttle back to MIN rather than continuing to ramp blind.
 *
 * NOTE: Pixy2 vectors give LINE POSITION, not vehicle speed -- there's no fixed track spacing
 * to convert pixel movement into velocity. They're used here only as a "camera link is alive
 * and sane" gate, not as an actual speed sensor. For real stall/speed feedback, the MPU6050 on
 * the telemetry ESP32 would be the better source, but that's off-board today.
==================================================================================================*/
#define THROTTLE_MIN         1638U   /* 1.0ms = 0% throttle -> motor off on this unidirectional ESC */
#define THROTTLE_KICKSTART   2100U   /* brief pulse to break static friction only               */
#define THROTTLE_CRUISE_LO   1900U   /* ramp target floor  - re-tune on track                    */
#define THROTTLE_CRUISE_HI   2050U   /* ramp target ceiling - re-tune on track                   */
#define THROTTLE_RAMP_STEP   8U      /* counts added per 20ms tick -> 0->cruise in well under 1s */
#define GARBAGE_FRAME_LIMIT  5U      /* consecutive bad Pixy2 frames before fail-safe throttle cut */
#define PIXY_MAX_VECTORS     4U      /* sanity bound - match pixy2.h if it defines a max          */
#define PIXY_FRAME_X_MAX     79U     /* frame is 0..78, matches LineCenterX = 39.0f below         */

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
    DisplayClear();
    uint16 DbgPack = BatteryGetMilliVolts();
    DisplayText(0U,"PackmV",7U,0U);
    DisplayValue(0U,DbgPack,6U,7U);
    DisplayText(1U,"CellmV",7U,0U);
    DisplayValue(1U, BatteryCellMilliVolts(DbgPack),6U,7U);
    DisplayRefresh();
    DelayMs(5000U);

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

    /* Give the driver 2 s to clear the car */
    Pwm_SetDutyCycle(0U, 1638U);
    DelayMs(2000U);
    boolean start = true;
    uint16 Battery;

    for (;;){
    	if(ButtonIsPressed()){
			if (start == true){
				/* KICKSTART: brief pulse only, just to break static friction. The old code held a
				 * fixed cruise duty (1850) right after this, which was too thin a margin above
				 * THROTTLE_MIN (1638) to sustain rotation under load and caused ESC stall/resync.
				 * Cruise is now reached by ramping in the control loop below instead of stepping. */
				Pwm_SetDutyCycle(0U, THROTTLE_KICKSTART);
				DelayMs(400U);
				start = false;
				DisplayClear();
				DisplayText(0U, "Vector:", 7U, 0U);
				DisplayText(1U, "Angle:", 4U, 0U);
				DisplayText(2U, "Cell:", 5U, 0U);
				DisplayText(3U, "Inters:", 7U, 0U);
				DisplayRefresh();
			}

			/* Fixed-rate steering controller. The loop is paced to a constant period by SysTick
			 * (DelayStartPeriod/DelayWaitPeriodEnd), so the PID's dt is a known constant. */
			const uint32 ControlPeriodUs = 1000000U;   /* 50 Hz control tick */

			/* hold motor off forever */
			Pwm_SetDutyCycle(0U, 1638U);
			DelayMs(100U);

			DetectedVectors PixyVectors;

			DelayStartPeriod(ControlPeriodUs);       /* open the fixed control window */

			/* One camera frame. getMainFeatures returns the tracked main vector (noise-filtered). */
			Pixy2GetVectors(&PixyVectors);

			if (PixyVectors.NumberOfIntersections == 0U){
				DisplayValue(0U, PixyVectors.NumberOfVectors, 6U, 8U);
				DisplayValue(1U, 0U , 6U, 7U);
				DisplayValue(2U, BatteryGetMilliVolts() , 6U, 6U);
				DisplayValue(3U, PixyVectors.NumberOfIntersections, 6U, 8U);
				DisplayRefresh();
			} else {
				DisplayValue(0U, PixyVectors.NumberOfVectors, 6U, 8U);
				DisplayValue(1U, 0U , 6U, 7U);
				DisplayValue(2U, BatteryGetMilliVolts() , 6U, 6U);
				DisplayValue(3U, PixyVectors.NumberOfIntersections, 6U, 8U);
				DisplayRefresh();
			}

			DelayWaitPeriodEnd();                     /* sleep the rest of the period -> constant dt */
		} else {
			/* hold motor off forever */
			Pwm_SetDutyCycle(0U, 1638U);
			DelayMs(100U);
			/*making a startup throttle kick available when the car starts*/
			start = true;
			Battery = BatteryGetMilliVolts();
			/* Check first if the button is released due to low voltage*/
			if (BatteryIsLow(Battery)){
				DisplayClear();
				DisplayText(0U,"Low",7U,0U);
				DisplayText(0U,"Voltage",7U,4U);
				DisplayText(1U,"CellmV",7U,0U);
				DisplayValue(1U, Battery,6U,7U);
				DisplayRefresh();
			} else {
				/*displaying press to start*/
				DisplayClear();
				DisplayText(0U,"Press to start",14U,0U);
				DisplayRefresh();
			}
		}
	}
}

#ifdef __cplusplus
}
#endif

/** @} */

