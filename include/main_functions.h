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
#ifndef MAIN_FUNCTIONS_H
#define MAIN_FUNCTIONS_H
/*These functions assume the relevant drivers are already initialised*/
#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "main_types.h"
/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
/* Reusable discrete PID controller. State lives in the struct, so it is reentrant and can be
 * reset cleanly (e.g. when a line follower loses then re-acquires the line). Features:
 *   - derivative on the MEASUREMENT (not the error) -> no kick if the set-point ever changes,
 *   - first-order low-pass on the derivative (DerivAlpha, 0..1) to tame noise amplification,
 *   - back-calculation anti-windup + a hard integral clamp,
 *   - output saturation, and a bumpless first step after Init/Reset. */
typedef struct{
    float Kp, Ki, Kd;        /* gains                                                   */
    float SetPoint;          /* target measurement                                      */
    float OutMin, OutMax;    /* output saturation limits                                */
    float IntLimit;          /* |integral| clamp (anti-windup)                          */
    float DerivAlpha;        /* derivative LPF coefficient: 1 = none, smaller = smoother */
    float Integral;          /* running integral term                                   */
    float PrevMeas;          /* previous measurement (for derivative-on-measurement)    */
    float DerivState;        /* filtered derivative term                                */
    boolean Primed;          /* FALSE until the first update seeds PrevMeas             */
}Pid;

/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/

/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/
void DriversInit(void);
Vector NormalizePixyVector(Vector PixyVector);
uint8 SmoothLineX(uint8 RawX);

/* PID lifecycle. PidInit sets gains/limits and clears state; PidReset clears just the running
 * state (integral, derivative, prime flag) keeping the gains; PidUpdate advances one step with
 * the latest measurement and the elapsed time Dt (seconds) and returns the clamped output. */
void  PidInit(Pid *Controller, float Kp, float Ki, float Kd, float SetPoint,
              float OutMin, float OutMax, float IntLimit, float DerivAlpha);
void  PidReset(Pid *Controller);
float PidUpdate(Pid *Controller, float Measurement, float Dt);

void DisplayTest(void);
void ReceiverTest(void);
void ServoTest(void);
void LinearCameraTest(void);
void Pixy2Test(void);
void EscTest(void);

#ifdef __cplusplus
}
#endif

#endif
/** @} */
