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
 * 1) system and project includes
 * 2) needed interfaces from external units
 * 3) internal and external interfaces from this unit
==================================================================================================*/
#include "main_functions.h"
#include "Mcu.h"
#include "Mcl.h"
#include "Platform.h"
#include "Port.h"
#include "CDD_I2c.h"
#include "Pwm.h"
#include "Icu.h"
#include "Gpt.h"
#include "Adc.h"
#include "Mcal.h"

#include "display.h"
#include "receiver.h"
#include "servo.h"
#include "pixy2.h"
#include "esc.h"
#include "linear_camera.h"
/*==================================================================================================
 *                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/

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
void DriversInit(void){
    uint8 Index;
    /* Init system clock */
#if (MCU_PRECOMPILE_SUPPORT == STD_ON)
    Mcu_Init(NULL_PTR);
#elif (MCU_PRECOMPILE_SUPPORT == STD_OFF)
    Mcu_Init(&Mcu_Config_VS_0);
#endif

    /* Initialize the clock tree and apply PLL as system clock */
    Mcu_InitClock(McuClockSettingConfig_0);
    #if (MCU_NO_PLL == STD_OFF)
    while (MCU_PLL_LOCKED != Mcu_GetPllStatus())
        {
            /* Busy wait until the System PLL is locked */
        }
    Mcu_DistributePllClock();
    #endif
    Mcu_SetMode(McuModeSettingConf_0);

    /* Initialize Platform driver */
    Platform_Init(NULL_PTR);

    /* Initialize Port driver */
    Port_Init(NULL_PTR);

    /* Initialize Mcl */
    Mcl_Init(NULL_PTR);

    /* Init i2c instances */
    I2c_Init(NULL_PTR);

    /* Initialize the Icu driver */
    Icu_Init(NULL_PTR);

    /*Init gpt driver*/
    Gpt_Init(NULL_PTR);

    /*Init adc driver*/
    Adc_Init(NULL_PTR);
    Adc_CalibrationStatusType CalibStatus;
    for(Index = 0; Index <= 5; Index++)
    {
        Adc_Calibrate(0U, &CalibStatus);
        if(CalibStatus.AdcUnitSelfTestStatus == E_OK)
        {
            break;
        }
    }

    /*Init pwm driver*/
    Pwm_Init(NULL_PTR);
}

/* ---- Reusable discrete PID controller (see Pid in main_functions.h) ------------------------ */

void PidInit(Pid *Controller, float Kp, float Ki, float Kd, float SetPoint,
             float OutMin, float OutMax, float IntLimit, float DerivAlpha){
    Controller->Kp         = Kp;
    Controller->Ki         = Ki;
    Controller->Kd         = Kd;
    Controller->SetPoint   = SetPoint;
    Controller->OutMin     = OutMin;
    Controller->OutMax     = OutMax;
    Controller->IntLimit   = IntLimit;
    Controller->DerivAlpha = DerivAlpha;
    PidReset(Controller);
}

void PidReset(Pid *Controller){
    Controller->Integral   = 0.0f;
    Controller->PrevMeas   = 0.0f;
    Controller->DerivState = 0.0f;
    Controller->Primed     = FALSE;   /* next update seeds PrevMeas -> no derivative spike */
}

/* One control step. Measurement and SetPoint share the same units; the returned output is in
 * [OutMin, OutMax]. Dt is the elapsed time in seconds (constant when called from a fixed-rate
 * loop). Derivative is taken on the measurement and low-pass filtered; the integral uses
 * back-calculation anti-windup so a saturated output cannot keep charging it. */
float PidUpdate(Pid *Controller, float Measurement, float Dt){
    float Error = Controller->SetPoint - Measurement;
    float Pterm, Dmeas, Draw, Output;

    /* Bumpless first step after Init/Reset: seed history, emit proportional-only. */
    if(Controller->Primed == FALSE){
        Controller->PrevMeas = Measurement;
        Controller->Primed   = TRUE;
    }

    Pterm = Controller->Kp * Error;

    /* Derivative on measurement (note the sign), then first-order low-pass filter. */
    Dmeas = (Measurement - Controller->PrevMeas) / Dt;
    Controller->PrevMeas = Measurement;
    Draw = -Controller->Kd * Dmeas;
    Controller->DerivState += Controller->DerivAlpha * (Draw - Controller->DerivState);

    /* Integrate, then hard-clamp the accumulator. */
    Controller->Integral += Controller->Ki * Error * Dt;
    if(Controller->Integral >  Controller->IntLimit){ Controller->Integral =  Controller->IntLimit; }
    if(Controller->Integral < -Controller->IntLimit){ Controller->Integral = -Controller->IntLimit; }

    Output = Pterm + Controller->Integral + Controller->DerivState;

    /* Output saturation with back-calculation anti-windup: if we clip, push the excess back out
     * of the integral so it doesn't accumulate while saturated. */
    if(Output > Controller->OutMax){
        Controller->Integral -= (Output - Controller->OutMax);
        if(Controller->Integral < -Controller->IntLimit){ Controller->Integral = -Controller->IntLimit; }
        Output = Controller->OutMax;
    }
    else if(Output < Controller->OutMin){
        Controller->Integral -= (Output - Controller->OutMin);   /* (Output-OutMin) < 0 -> raises I */
        if(Controller->Integral >  Controller->IntLimit){ Controller->Integral =  Controller->IntLimit; }
        Output = Controller->OutMin;
    }

    return Output;
}

#ifdef __cplusplus
}
#endif

/** @} */
