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
#ifndef BRUSHLESS_H
#define BRUSHLESS_H
/*Drives two brushless motors, each on its own ESC, with a single shared speed command.
 *These functions assume the relevant drivers are already initialised.*/
#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "Pwm.h"

/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/
/*if your chosen ESCs have braking capabilities, set this on STD_ON. If not, set it on STD_OFF.
 * Using the wrong configuration can make the car go full speed backward on braking!*/
#define BRUSHLESS_HAS_BRAKE     STD_ON

enum BrushlessStates{
    BrushlessForward,
    BrushlessBraking,
    BrushlessNeutral,
    BrushlessReverse
};

typedef struct{
    enum BrushlessStates State;
    Pwm_ChannelType Channel1;   /*PWM channel driving ESC 1*/
    Pwm_ChannelType Channel2;   /*PWM channel driving ESC 2*/
    uint16 MinDutyCycle;        /*1ms  -> full reverse*/
    uint16 MaxDutyCycle;        /*2ms  -> full forward*/
    uint16 MedDutyCycle;        /*1.5ms-> neutral (arms the ESCs)*/
    int Speed;    /*values between -100 and 100: val >= 0 means Forward, val < 0 means Reverse*/
    uint8 Brake;  /*value is zero or non zero: val != 0 means brake, val == 0 means no brake*/
}Brushless;

/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/
/*Channel1/Channel2: the two PWM channels (configured ~50Hz) wired to the two ESCs.
 *MinDutyCycle/MedDutyCycle/MaxDutyCycle: PWM ticks for 1ms / 1.5ms / 2ms signals.*/
void BrushlessInit(Pwm_ChannelType Channel1, Pwm_ChannelType Channel2,
                   uint16 MinDutyCycle, uint16 MedDutyCycle, uint16 MaxDutyCycle);
void BrushlessSetSpeed(int Speed);   /*-100..100, applied to both motors*/
void BrushlessSetBrake(uint8 Brake); /*0 = release, non-zero = brake both motors*/

/*PWM falling-edge notification callback. Register this on Channel1 in the Peripherals tool.*/
void Brushless_Period_Finished(void);

#ifdef __cplusplus
}
#endif

#endif
/** @} */
