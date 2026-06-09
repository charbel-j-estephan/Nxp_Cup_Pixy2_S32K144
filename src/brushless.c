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
#include "brushless.h"
#include "debug_signals.h"

/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/
static volatile Brushless BrushlessInstance;

/*==================================================================================================
*                          FREEMASTER-WATCHED GLOBALS
==================================================================================================*/
volatile sint16 FmstrBrushlessSpeed      = 0;    /* commanded –100..+100              */
volatile uint8  FmstrBrushlessBrake      = 0U;   /* 0 = off, 1 = braking              */
volatile uint8  FmstrBrushlessState      = 2U;   /* 0=Fwd 1=Brk 2=Neu 3=Rev           */
volatile uint16 FmstrBrushlessDutyCycle  = 0U;   /* raw PWM duty ticks                */

/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/
/* Translate a -100..100 speed command into a pulse width and apply it to BOTH ESCs. */
static void SetPwm(int SpeedCommand) {
    uint16 DutyCycle;
    if(SpeedCommand >= 0) {
        DutyCycle = (uint16)(BrushlessInstance.MedDutyCycle
                  + SpeedCommand*(int)(BrushlessInstance.MaxDutyCycle-BrushlessInstance.MedDutyCycle)/100);
    }
    else{
        DutyCycle = (uint16)(BrushlessInstance.MedDutyCycle
                  + SpeedCommand*(int)(BrushlessInstance.MedDutyCycle-BrushlessInstance.MinDutyCycle)/100);
    }
    Pwm_SetDutyCycle(BrushlessInstance.Channel1, DutyCycle);
    Pwm_SetDutyCycle(BrushlessInstance.Channel2, DutyCycle);
    FmstrBrushlessDutyCycle = DutyCycle;   /* FreeMASTER live view */
}

/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/
/* Runs the shared ESC state machine once per PWM period. Both motors share one state because
 * they receive identical speed/brake commands. Register on Channel1's falling-edge notification. */
void Brushless_Period_Finished(void){
    int SpeedCommand;
    /* Mirror state-machine internals to FreeMASTER globals at every PWM edge */
    FmstrBrushlessSpeed = (sint16)BrushlessInstance.Speed;
    FmstrBrushlessBrake = BrushlessInstance.Brake;
    FmstrBrushlessState = (uint8)BrushlessInstance.State;
    /*if your chosen ESCs have braking capabilities, set BRUSHLESS_HAS_BRAKE to STD_ON. If not, STD_OFF.
     * Using the wrong configuration can make the car go full speed backward on braking!*/
#if (BRUSHLESS_HAS_BRAKE == STD_ON)
    /* state machine updates here at every PWM signal edge*/
    switch(BrushlessInstance.State){
    case BrushlessForward:
        if(BrushlessInstance.Brake != 0U || BrushlessInstance.Speed < 0){
            BrushlessInstance.State = BrushlessBraking;
        }
        break;
    case BrushlessBraking:
        if(BrushlessInstance.Brake == 0U && BrushlessInstance.Speed >= 0){
            BrushlessInstance.State = BrushlessForward;
        }
        else if(BrushlessInstance.Brake == 0U && BrushlessInstance.Speed < 0){
            BrushlessInstance.State = BrushlessNeutral;
        }
        break;
#else
    switch(BrushlessInstance.State){
        case BrushlessForward:
            if(BrushlessInstance.Brake != 0U){
                BrushlessInstance.State = BrushlessBraking;
            }
            else if(BrushlessInstance.Speed < 0){
                BrushlessInstance.State = BrushlessReverse;
            }
            break;
        case BrushlessBraking:
            if(BrushlessInstance.Brake != 0U){
                BrushlessInstance.State = BrushlessNeutral;
            }
            else if(BrushlessInstance.Speed < 0){
                BrushlessInstance.State = BrushlessReverse;
            }
            else if(BrushlessInstance.Speed >= 0){
                BrushlessInstance.State = BrushlessForward;
            }
            break;
#endif
        case BrushlessNeutral:
            if(BrushlessInstance.Brake == 0U && BrushlessInstance.Speed < 0){
                BrushlessInstance.State = BrushlessReverse;
            }
            else if(BrushlessInstance.Brake == 0U && BrushlessInstance.Speed >= 0){
                BrushlessInstance.State = BrushlessForward;
            }
            break;
        case BrushlessReverse:
            if(BrushlessInstance.Brake != 0U){
                BrushlessInstance.State = BrushlessBraking;
            }
            else if(BrushlessInstance.Speed >= 0){
                BrushlessInstance.State = BrushlessForward;
            }
            break;
        default:/*invalid case, set safe values for the ESCs*/
            BrushlessInstance.Speed = 0;
    }

    /*update the car's speed*/
    switch(BrushlessInstance.State){
        case BrushlessForward:
        case BrushlessReverse:
            SpeedCommand = BrushlessInstance.Speed;
            break;
        case BrushlessBraking:
#if (BRUSHLESS_HAS_BRAKE == STD_ON)
            SpeedCommand = -100;
#else
            SpeedCommand = -BrushlessInstance.Speed;
#endif
            break;
        case BrushlessNeutral:
        default:
            SpeedCommand = 0;
    }
    SetPwm(SpeedCommand);
}

void BrushlessInit(Pwm_ChannelType Channel1, Pwm_ChannelType Channel2,
                   uint16 MinDutyCycle, uint16 MedDutyCycle, uint16 MaxDutyCycle){
    BrushlessInstance.Channel1 = Channel1;
    BrushlessInstance.Channel2 = Channel2;
    BrushlessInstance.MinDutyCycle = MinDutyCycle;/*1ms*/
    BrushlessInstance.MedDutyCycle = MedDutyCycle;/*1.5ms*/
    BrushlessInstance.MaxDutyCycle = MaxDutyCycle;/*2ms*/
    BrushlessInstance.State = BrushlessNeutral;
    BrushlessInstance.Speed = 0;
    BrushlessInstance.Brake = 0U;
    /*sending the 'Neutral' command to both ESCs arms them*/
    Pwm_SetDutyCycle(Channel1, MedDutyCycle);
    Pwm_SetDutyCycle(Channel2, MedDutyCycle);
    /*one notification on Channel1 drives the shared state machine for both motors*/
    Pwm_EnableNotification(Channel1, PWM_FALLING_EDGE);
}

void BrushlessSetSpeed(int Speed){
    if(Speed > 100){
        BrushlessInstance.Speed = 100;
    }
    else if(Speed < -100){
        BrushlessInstance.Speed = -100;
    }
    else{
        BrushlessInstance.Speed = Speed;
    }
}

void BrushlessSetBrake(uint8 Brake){
    if(Brake != 0U){
        BrushlessInstance.Brake = 1U;
    }
    else{
        BrushlessInstance.Brake = 0U;
    }
}

#ifdef __cplusplus
}
#endif

/** @} */
