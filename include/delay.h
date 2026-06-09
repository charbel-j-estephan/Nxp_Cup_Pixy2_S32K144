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
#ifndef DELAY_H
#define DELAY_H
/*Blocking delays built on the Cortex-M SysTick timer.*/
#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/
#include "Std_Types.h"

/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/

/* Configures SysTick to run from the core clock. Pass the core clock in Hz (48000000 on this
 * project). Call once after the clock is initialised, before using DelayMs/DelayUs. */
void DelayInit(uint32 CoreClockHz);

/* Blocks for the requested number of milliseconds. */
void DelayMs(uint32 Milliseconds);

/* Blocks for the requested number of microseconds. */
void DelayUs(uint32 Microseconds);

#ifdef __cplusplus
}
#endif

#endif
/** @} */
